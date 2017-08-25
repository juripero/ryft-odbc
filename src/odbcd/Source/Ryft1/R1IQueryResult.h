// =================================================================================================
///  @file R1IQueryResult.h
///
///  Implements the query result interface class
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================
#ifndef _R1IQUERYRESULT_H_
#define _R1IQUERYRESULT_H_

namespace Simba
{
namespace Support
{
    class ILogger;
}
}

#include "RyftOne.h"
using namespace RyftOne;

#include "libmeta.h"
#include "sqlite3.h"

#include <glob.h>
#include <dirent.h>
#include <stdio.h>
#include <libconfig.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <string.h>
#include <curl/curl.h>
#include <cctype>
#include <pwd.h>
#include <json/json.h>

#include "R1Util.h"
#include "TypeDefines.h"
#include "NumberConverter.h"

__inline void CopyEscapedIdentifier(string& dest, string& src)
{
    string::iterator itr; 
    char pchar = 0;

    try {
        // remove special characters from identifiers
        for(itr = src.begin(); itr != src.end(); ++itr) {
            switch (*itr) {
            case '\'':
            case '\"':
            case '\\':
            case '[':
            case ']':
            case '.':
            case '!':
            case '`':
                if(pchar != '_') 
                    dest += '_';
                pchar = '_';
                break;
            default:
                dest += *itr;
                pchar = *itr;
            }
        }
    }
    catch(exception &e) {
        ;
    }
}

__inline void CopyEscapedLiteral(string& dest, string& src)
{
    string::iterator itr; 
    try {
        for(itr = src.begin(); itr != src.end(); ++itr) {
            if(*itr == '\'') {
                dest += "''";
            }
            else
                dest += *itr;
        }
    }
    catch(exception& e) {
        ;
    }
}
#define SC_NUMERIC  0
#define SC_TEXT     1
#define SC_REAL     2
struct resultCol {
    int colNum;
    int storageClass;
    int storageBytes;
    int charCols;
    union {
        char *text;
        long long integer;
        double real;
    } colResult;

};
typedef vector<resultCol> resultCols;

enum {
    typeNative = 0,
    typeInserted
};

typedef struct _Cursor {
    int __type;
    int __colIdx;
    resultCols __row;
} Cursor;

typedef struct _RyftOne_Column {
    unsigned m_ordinal;
    string m_tableName;
    string m_colName;
    string m_colAlias;
    string m_description;
    unsigned m_dataType;
    string m_typeName;
    unsigned m_charCols;
    unsigned m_bufLength;
    unsigned m_dtType;
    string m_formatSpec;
} RyftOne_Column;
typedef vector<RyftOne_Column> RyftOne_Columns;

static int xCreate(sqlite3* in_sqlite, void *in_qresult, int in_argc, const char * const * in_argv, sqlite3_vtab **out_ppVTab, char**out_pzErr);
static int xConnect(sqlite3* in_sqlite, void *in_qresult, int in_argc, const char * const * in_argv, sqlite3_vtab **out_ppVTab, char**out_pzErr);
static int xDisconnect(sqlite3_vtab *in_pVTab);
static int xDestroy(sqlite3_vtab *in_pVTab);
static int xOpen(sqlite3_vtab *in_pVTab, sqlite3_vtab_cursor **out_ppCursor);
static int xClose(sqlite3_vtab_cursor *in_pVTabCursor);
static int xBestIndex(sqlite3_vtab *pVTab, sqlite3_index_info*);
static int xFilter(sqlite3_vtab_cursor*, int idxNum, const char *idxStr, int argc, sqlite3_value **argv);
static int xNext(sqlite3_vtab_cursor *in_pVTabCursor);
static int xEof(sqlite3_vtab_cursor *in_pVTabCursor);
static int xColumn(sqlite3_vtab_cursor *in_pVTabCursor, sqlite3_context*, int);
static int xRowid(sqlite3_vtab_cursor *in_pVTabCursor, sqlite3_int64 *pRowid);
static int xRename(sqlite3_vtab *pVtab, const char *zNew);

class IQueryResult {
public:
    IQueryResult(ILogger *log) : __log(log), __sqlite(0), __stmt(0) { ; }
   ~IQueryResult()
    {
        if(__stmt) 
            sqlite3_finalize(__stmt);
        __stmt = NULL;

        if(__sqlite)
            sqlite3_close(__sqlite);
        __sqlite = NULL;
   }

    void OpenQuery(string& in_name, vector<__catalog_entry__>::iterator in_catentry, 
        string in_server, string in_token, string in_restPath, string in_rootPath)
    {
        __query.clear();

        __restServer = in_server;
        __restToken = in_token;
        __restPath = in_restPath;
        __rootPath = in_rootPath;

        // JSON, XML, Unstructured
        __dataType = in_catentry->meta_config.data_type;

        __loadTable(in_name, in_catentry);

        vector<__meta_config__::__meta_view__>::iterator viewItr;
        for(viewItr = in_catentry->meta_config.views.begin(); viewItr != in_catentry->meta_config.views.end(); viewItr++) {
            if(in_name == viewItr->view_name) {
                __query = viewItr->restriction;
                break;
            }
        }
        
        __cols = __getColumns(in_catentry->meta_config);

        resultCol rcol;
        for(int idx = 0; idx < __metaTags.size(); idx++) {
            rcol.colNum = idx;
            rcol.storageBytes = __cols[idx].m_bufLength;
            rcol.charCols = __cols[idx].m_charCols;
            switch (__cols[idx].m_dataType) {
            default:
                rcol.storageClass = SC_TEXT;
                rcol.colResult.text = NULL;
                break;
            case SQL_INTEGER:
            case SQL_BIGINT:
                rcol.storageClass = SC_NUMERIC;
                rcol.colResult.integer = 0;
                break;
            case SQL_DOUBLE:
                rcol.storageClass = SC_REAL;
                rcol.colResult.real = 0.0f;
                break;
            }
            __cursor.__row.push_back(rcol);
        }

        string dbpath = __path;
        dbpath += s_R1Caches;
        mkdir(dbpath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
        struct passwd *pwd = getpwnam(s_RyftUser);
        if(pwd != NULL)
            chown(dbpath.c_str(), pwd->pw_uid, pwd->pw_gid);

        __metafile_stat = in_catentry->meta_config.metafile_stat;

        dbpath += "/caches_v2.bin";
        int sqlret = sqlite3_open(dbpath.c_str(), &__sqlite);

        char *errp = NULL;
        sqlret = sqlite3_exec(__sqlite, "CREATE TABLE \"__DIRECTORY__\" (ID TEXT, QUERY TEXT, IDX_FILE TEXT, NUM_ROWS INTEGER)", NULL, NULL, &errp);
        if(errp)
            sqlite3_free(errp);

        memset(&__vtab_mod, 0, sizeof(sqlite3_module));
        __vtab_mod.xCreate = xCreate;
        __vtab_mod.xConnect = xConnect;
        __vtab_mod.xDisconnect = xDisconnect;
        __vtab_mod.xDestroy = xDestroy;
        __vtab_mod.xOpen = xOpen;
        __vtab_mod.xClose = xClose;
        __vtab_mod.xBestIndex = xBestIndex;
        __vtab_mod.xFilter = xFilter;
        __vtab_mod.xNext = xNext;
        __vtab_mod.xEof = xEof;
        __vtab_mod.xColumn = xColumn;
        __vtab_mod.xRowid = xRowid;
        __vtab_mod.xRename = xRename;
        sqlite3_create_module(__sqlite, "query_vtab", &__vtab_mod, this);

        __queryFinished = false;
    }

    virtual void AppendFilter(string in_filter)
    {
        if(!__query.empty())
            __query += " AND ";

        __query += in_filter;
    }
    
    void PrepareUpdate()
    {
        int idx;
        char * errp;
        string coldefs;
        vector<string>::iterator itr;
        for(idx = 0, itr = __metaTags.begin(); itr != __metaTags.end(); itr++, idx++) {
            if(!coldefs.empty())
                coldefs += ",";

            coldefs += "\"";
            CopyEscapedIdentifier(coldefs, *itr);
            coldefs += "\" ";

            switch(__cursor.__row[idx].storageClass) {
            case SC_NUMERIC:
                coldefs += "BIGINT";
                break;
            case SC_TEXT:
                coldefs += "TEXT";
                break;
            case SC_REAL:
                coldefs += "DOUBLE";
                break;
            }
        }

        char *sql = sqlite3_mprintf("CREATE TEMP TABLE \"__APPENDED__\" (%s)", coldefs.c_str());
        int sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);

        string values;
        for(idx = 0; idx < __metaTags.size(); idx++) {
            if(!values.empty())
                values += ",";
            values += "?";
        }

        sql = sqlite3_mprintf("INSERT INTO \"__APPENDED__\" VALUES (%s)", values.c_str() );
        sqlret = sqlite3_prepare_v2(__sqlite, sql, -1, &__stmt, NULL);
        sqlite3_free(sql);

        resultCols::iterator itrCols;
        for(itrCols = __cursor.__row.begin(); itrCols != __cursor.__row.end(); itrCols++) 
            (*itrCols).colResult.text = (char *)sqlite3_malloc((*itrCols).charCols + 1);
    }

    void FinishUpdate()
    {
        int sqlret;
        resultCols::iterator itr;
        if(__stmt) {
            for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
                // bind as text, sqlite will do the conversion into the right storage class
                sqlret = sqlite3_bind_text(__stmt, (*itr).colNum + 1, (const char *)(*itr).colResult.text, -1, SQLITE_STATIC);
            }
            sqlret = sqlite3_step(__stmt);
        }

        for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) 
            sqlite3_free((*itr).colResult.text);

        sqlite3_finalize(__stmt);
        __stmt = NULL;
    }

    void FlushUpdate()
    {
        time_t now = ::time(NULL);
        tm *lt = localtime(&now);
        char file_path[PATH_MAX];
        char temp_path[PATH_MAX];
        IFile *formattedFile = NULL;
        sprintf(temp_path, "%s/%04d%02d%02d.%s%s", __path.c_str(), 1900 + lt->tm_year, lt->tm_mon + 1, lt->tm_mday, "journal_", __extension.c_str());
        sprintf(file_path, "%s/%04d%02d%02d.%s", __path.c_str(), 1900 + lt->tm_year, lt->tm_mon + 1, lt->tm_mday, __extension.c_str());
        formattedFile = __createFormattedFile(temp_path);
        if(formattedFile == NULL)
            return;
        formattedFile->prolog();
        formattedFile->copyFile(file_path);
        char **prows;
        int nrows, ncols;
        char *errp;
        int sqlret = sqlite3_get_table(__sqlite, "SELECT * FROM \"__APPENDED__\"", &prows, &nrows, &ncols, &errp);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) {
            delete formattedFile;
            return;
        }
        int idx;
        int colIdx;
        for(idx = 1; idx <= nrows; idx++) {
            std::vector<resultCol>::iterator itr;
            formattedFile->startRecord();
            for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) 
                formattedFile->outputField(__metaTags[(*itr).colNum].c_str(), prows[idx * ncols + (*itr).colNum]);
            formattedFile->endRecord();
        }
        formattedFile->epilog();
        delete formattedFile;
        char cpcmd[PATH_MAX];
        sprintf(cpcmd, "cp %s %s", temp_path, file_path);
        system(cpcmd);
        struct passwd *pwd = getpwnam(s_RyftUser);
        if(pwd != NULL)
            chown(file_path, pwd->pw_uid, pwd->pw_gid);
        unlink(temp_path);
    }

    int UnloadResults()
    {
        char unloadPath[PATH_MAX];
        __odbcRoot(unloadPath);
        strcat(unloadPath, s_R1Unload);
        mkdir(unloadPath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH); 
        struct passwd *pwd = getpwnam(s_RyftUser);
        if(pwd != NULL)
            chown(unloadPath, pwd->pw_uid, pwd->pw_gid);

        time_t now = ::time(NULL);
        tm *lt = localtime(&now);
        char file_path[PATH_MAX];
        IFile *formattedFile = NULL;
        sprintf(file_path, "%s/%s_%04d%02d%02d.%s", 
            unloadPath, __name.c_str(), 1900 + lt->tm_year, lt->tm_mon + 1, lt->tm_mday, __extension.c_str());
        formattedFile = __createFormattedFile(file_path);
        if(formattedFile == NULL)
            return 0;
        formattedFile->prolog();
        int numrows = 0;
        if(FetchFirst()) {
            do {
                formattedFile->startRecord();
                for(int idx = 0; idx < __cols.size(); idx++) {
                    formattedFile->outputField(__metaTags[idx].c_str(), GetStringValue(idx));
                }
                formattedFile->endRecord();
                numrows++;
            } while (FetchNext());
        }
        formattedFile->epilog();
        delete formattedFile;
        return numrows;
    }

    bool FetchFirst() 
    {
        if(!__queryFinished)
            __queryFinished = __execute();

        int sqlret = SQLITE_DONE;
        if(__stmt) 
            sqlite3_reset(__stmt);

        return FetchNext();
    };

    bool FetchNext()
    {
        int sqlret = SQLITE_DONE;
        if(__stmt) 
            sqlret = sqlite3_step(__stmt);

        return(sqlret == SQLITE_ROW);
    }

    void CloseCursor()
    {
        if(__stmt) 
            sqlite3_finalize(__stmt);
        __stmt = NULL;
    }

    const char * GetStringValue(int colIdx)
    {
        switch(__cols[colIdx].m_dtType) {
        case TYPE_META_FILE:
            return __idxFilename;
        default:
            return (const char *)sqlite3_column_text(__stmt, colIdx);
        }
    }

    int GetIntValue(int colIdx)
    {
        return sqlite3_column_int(__stmt, colIdx);
    }

    long long GetInt64Value(int colIdx)
    {
        switch(__cols[colIdx].m_dtType) {
        case TYPE_META_OFFSET:
            return __offset;
        case TYPE_META_LENGTH:
            return __length;
        default:
            return sqlite3_column_int64(__stmt, colIdx);
        }
    }

    double GetDoubleValue(int colIdx)
    {
        return sqlite3_column_double(__stmt, colIdx);
    }

    struct tm GetDateValue(int colIdx)
    {
        struct tm date = {0};
        __date((const char *)sqlite3_column_text(__stmt, colIdx), colIdx, &date);
        return date;
    }

    struct tm GetTimeValue(int colIdx)
    {
        struct tm time = {0};
        __time((const char *)sqlite3_column_text(__stmt, colIdx), colIdx, &time);
        return time;
    }
        
    struct tm GetDateTimeValue(int colIdx)
    {
        struct tm ts = {0};
        __datetime((const char *)sqlite3_column_text(__stmt, colIdx), colIdx, &ts);
        return ts;
    }

    void PutStringValue(int colIdx, string colValue)
    {
        strcpy(__cursor.__row[colIdx].colResult.text, colValue.c_str());
    }
        
    void PutDateValue(int colIdx, tm *date)
    {
        __date(colIdx, date);
    }

    void PutTimeValue(int colIdx, tm *time)
    {
        __time(colIdx, time);
    }

    void PutDateTimeValue(int colIdx, tm *datetime)
    {
        __datetime(colIdx, datetime);
    }

    void GetTypeFormatSpecifier(int colIdx, unsigned *dtType, string& formatSpec)
    {
        *dtType = __cols[colIdx].m_dtType;
        formatSpec = __cols[colIdx].m_formatSpec;
    }

    bool IsStructuredType() 
    {
        return __isStructuredType();
    }

    bool OpenIndexedResult()
    {
        __idxFD = fopen(__idxFile.c_str(), "r");
        if(__idxFD == NULL) 
            return false;

        __idxCurRow = 0;

        resultCols::iterator itr;
        for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
            (*itr).colResult.text = (char *)sqlite3_malloc((*itr).charCols + 1);
        }

        __idxLine = new char[FILENAME_MAX];
        __idxFilename = new char[FILENAME_MAX];
        __cachedFile = new char[FILENAME_MAX];
        __idxLine[0] = '\0';
        __idxFilename[0] = '\0';
        __cachedFile[0] = '\0';
        __cachedFD = -1;

        return FetchNextIndexedResult();
    }

    bool CloseIndexedResult()
    {
        fclose(__idxFD);

        resultCols::iterator itr;
        for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
            sqlite3_free((*itr).colResult.text);
        }

        delete[] __idxLine;
        delete[] __idxFilename;
        delete[] __cachedFile;
        close(__cachedFD);

        return true;
    }

    bool FetchNextIndexedResult()
    {
        __idxFilename[0] = '\0';
        char * offset = NULL;
        char * length = NULL;
        char * surrounding = NULL;
        char * saveptr = NULL;
        __offset = __length = __surrounding = 0;

        if(fgets(__idxLine, FILENAME_MAX, __idxFD)) {
            saveptr = __idxLine;
            strcpy(__idxFilename, strtok_r(__idxLine, ",", &saveptr));
            if(offset = strtok_r(NULL, ",", &saveptr))
                __offset = strtoll(offset, NULL, 10);
            if(length = strtok_r(NULL, ",", &saveptr))
                __length = strtoll(length, NULL, 10);
            if(surrounding = strtok_r(NULL, "\n", &saveptr))
                __surrounding = strtoll(surrounding, NULL, 10);
        }

        if(strcmp(__cachedFile, __idxFilename)) {
            close(__cachedFD);
            // WORKWORK PATH IN INDEX FILE ASSUMED TO BE THE SAME ON BOTH REST SERVER AND ODBC SERVER
            __cachedFD = open(__idxFilename, O_RDONLY);
            strcpy(__cachedFile, __idxFilename);
        }

        lseek(__cachedFD, __offset, SEEK_SET);
        
        __parse(__cachedFD, __length, true, "");

        __idxCurRow++;
        return true;
    }

    bool IndexedResultEof() 
    {
        if(__idxCurRow > __idxNumRows)
            return true;

        return false;
    }

    const char * GetIndexedResultColumn(int col)
    {
        return __cursor.__row[col].colResult.text;
    }

    long GetResultCount()
    {
        return __idxNumRows;
    }

protected:

    NodeAction __startRow()
    {
        __cursor.__colIdx = -1;
        resultCols::iterator itr;
        for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
            *((*itr).colResult.text) = '\0';
        }
        return ProcessNode;
    }

    NodeAction __addElement(std::string sName, const char **psAttribs, XMLElement **ppElement)
    {
        __cursor.__colIdx = __getColumnNum(sName);
        return ProcessNode;
    }

    void __addText(std::string sText, std::string sDelimiter) 
    {
        if(__cursor.__colIdx != -1) {
            if(!sDelimiter.empty() && strlen(__cursor.__row[__cursor.__colIdx].colResult.text))
                strncat(__cursor.__row[__cursor.__colIdx].colResult.text, sDelimiter.c_str(), 
                    __cursor.__row[__cursor.__colIdx].charCols - strlen(__cursor.__row[__cursor.__colIdx].colResult.text));
            strncat(__cursor.__row[__cursor.__colIdx].colResult.text, sText.c_str(), 
                __cursor.__row[__cursor.__colIdx].charCols - strlen(__cursor.__row[__cursor.__colIdx].colResult.text));
        }
    }

    void __exitRow()
    {

    }

    virtual RyftOne_Columns __getColumns(__meta_config__ meta_config) 
    { 
        return __cols;
    }

    virtual IFile * __createFormattedFile(const char *filename)
    {
        return NULL;
    }

    virtual string __getFormatString()
    {
        return "";
    }

    virtual bool __isStructuredType()
    {
        return false;
    }

    virtual void __loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr)
    {
        ;
    }

    virtual bool __execute()
    {
        char results[PATH_MAX];
        string query(__query);
        bool ret = false;
        string tableName;

        if(query.empty()) 
            query = "( RECORD CONTAINS ? )";

        INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__execute", "Executing query = %s", query.c_str());

        if(!(ret = __loadFromSqlite(query))) {

            vector<string>::iterator globItr;

            uuid_t id;
            uuid_generate(id);
            char stringId[100];
            uuid_unparse(id, stringId);
            for(char *pstr = stringId; *pstr; pstr++)
                *pstr = toupper(*pstr);
            tableName = stringId;

            if (!__initTable(query, tableName)) {
                __dropTable(query);
                return false;
            }

            curl_global_init(CURL_GLOBAL_DEFAULT);
            CURL *curl = curl_easy_init();

            if(!__restToken.empty()) {
                struct curl_slist *header = NULL;
                string auth = "Authorization: Basic " + __restToken;
                header = curl_slist_append(header, auth.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
            }

            string url = __restServer + "/count?query=" + RyftOne_Util::UrlEncode(query);
            string relPath = RyftOne_Util::UrlEncode(__path.c_str() + __restPath.length());

            url += "&files=" + relPath + RyftOne_Util::UrlEncode("/") + "*." + __extension; 
            url += "&index=" + relPath + RyftOne_Util::UrlEncode("/.caches/") + tableName + ".txt";
            url += "&local=true";

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            char resultsPath[PATH_MAX];
            __odbcRoot(resultsPath);
            strcat(resultsPath, s_R1Results);

            mkdir(resultsPath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
            struct passwd *pwd = getpwnam(s_RyftUser);
            if(pwd != NULL)
                chown(resultsPath, pwd->pw_uid, pwd->pw_gid);

            sprintf(results, "%s/results.%08x", resultsPath, pthread_self());

            FILE *f = fopen(results, "w+");

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
            CURLcode code = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            curl_easy_cleanup(curl);
            curl_global_cleanup();
            fclose(f);

            if(code != CURLE_OK || http_code != 200) {
                string perr = __getErrorMessage(results);
                if(http_code == 401) {
                    // special case unauthorized 
                    perr = "HTTP Response - 401 Unauthorized";
                }
                INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__execute", "REST returned an error = %s", perr.c_str());
                simba_wstring errorMsg(perr);
                R1THROWGEN1(L"RolException", errorMsg.GetAsPlatformWString());
                unlink(results);
                return false;
            }
            ret = __storeToSqlite(tableName, results);
            unlink(results);
            if(!ret) {
                __dropTable(query);
                return false;
            }
            return __loadFromSqlite(query);
        }
        return ret;
    }

    virtual void __parse(int fd, size_t st_size, bool no_top, string top_object)
    {

    }

    vector<string> __glob;

    string __path;
    string __name;
    string __extension;
    vector<string> __metaTags;

    bool __no_top;
    string __delimiter;

    string __query;

private:
    ILogger* __log;

    string __restServer;
    string __restToken;
    string __restPath;
    string __rootPath;

    inline void __odbcRoot(char *path) {
        strcpy(path, __rootPath.c_str());
        strcat(path, s_R1Catalog);
    }

    bool __queryFinished;

    RyftOne_Columns __cols;

    DataType __dataType;

    sqlite3 *__sqlite;
    sqlite3_stmt *__stmt;
    sqlite3_module __vtab_mod;
    struct stat __metafile_stat;

    Cursor __cursor;

    string __idxFile;
    long __idxNumRows;
    long __idxCurRow;
    FILE * __idxFD;

    int __cachedFD;
    char * __cachedFile;
    char * __idxLine;
    char * __idxFilename;
    long long __offset;
    long long __length;
    long long __surrounding;

    bool __initTable(string& query, string& tableName)
    {
        char * errp;

        int idx;
        string coldefs;
        vector<string>::iterator itr;
        for(idx = 0, itr = __metaTags.begin(); itr != __metaTags.end(); itr++, idx++) {
            if(!coldefs.empty())
                coldefs += ",";

            coldefs += "\"";
            CopyEscapedIdentifier(coldefs, *itr);
            coldefs += "\" ";

            switch(__cursor.__row[idx].storageClass) {
            case SC_NUMERIC:
                coldefs += "BIGINT";
                break;
            case SC_TEXT:
                coldefs += "TEXT";
                break;
            case SC_REAL:
                coldefs += "DOUBLE";
                break;
            }
        }

        char *sql = sqlite3_mprintf("CREATE VIRTUAL TABLE \"%s\" USING query_vtab (%s)", tableName.c_str(), coldefs.c_str());
        int sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) 
            return false;

        sql = sqlite3_mprintf("CREATE TABLE \"__FGLOB_STAT_%s__\" (\"__ST_DEV\" INTEGER, \"__ST_INO\" INTEGER, \"__ST_MTIME\" INTEGER, \"__METAFILE_STAT\" INTEGER )", 
            tableName.c_str());
        sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) 
            return false;

        // store fstat for .meta.table
        sql = sqlite3_mprintf("INSERT INTO \"__FGLOB_STAT_%s__\" VALUES (%d,%d,%d,1)", tableName.c_str(), __metafile_stat.st_dev, __metafile_stat.st_ino, __metafile_stat.st_mtime);
        sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) 
            return false;

        int fd;
        struct stat sb;
        vector<string>::iterator globItr;
        for(globItr = __glob.begin(); globItr != __glob.end(); globItr++) {
            fd = ::open((*globItr).c_str(), O_RDONLY);
            if(fd == -1)
                return false;
            fstat(fd, &sb);
            close(fd);
            sql = sqlite3_mprintf("INSERT INTO \"__FGLOB_STAT_%s__\" VALUES (%d,%d,%d,0)", tableName.c_str(), sb.st_dev, sb.st_ino, sb.st_mtime);
            sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
            sqlite3_free(sql);
            if(errp)
                sqlite3_free(errp);
            if(sqlret != SQLITE_OK) 
                return false;
        }

        string escapedQuery;
        CopyEscapedLiteral(escapedQuery, query);
        sql = sqlite3_mprintf("INSERT INTO \"__DIRECTORY__\" VALUES ('%s','%s', NULL, NULL)", tableName.c_str(), escapedQuery.c_str());
        sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) 
            return false;

        return true;
    }

    void __dropTable(string& query)
    {
        char *sql;
        int sqlret;
        char *errp;
        char **prows;
        int nrows, ncols;

        string escapedQuery;
        CopyEscapedLiteral(escapedQuery, query);
        sql = sqlite3_mprintf("SELECT * FROM \"__DIRECTORY__\" WHERE QUERY = '%s'", escapedQuery.c_str());
        sqlret = sqlite3_get_table(__sqlite, sql, &prows, &nrows, &ncols, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) 
            return;
        if(nrows == 0) 
            return;

        string tableName = prows[ncols+0];
        sql = sqlite3_mprintf("DROP TABLE \"__FGLOB_STAT_%s__\"", tableName.c_str());
        sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);

        sql = sqlite3_mprintf("DROP TABLE \"%s\"", tableName.c_str());
        sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);

        sql = sqlite3_mprintf("DELETE FROM \"__DIRECTORY__\" WHERE QUERY = '%s'", escapedQuery.c_str());
        sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);

        // delete the stored index file
        unlink(prows[ncols+2]);

        sqlite3_free_table(prows);
    }

    bool __loadFromSqlite(string& query)
    {
        char *sql;
        int sqlret;
        char **prows;
        int nrows, ncols;
        char *errp;
        int fd;
        int idx;
        struct stat sb;
        vector<string>::iterator globItr;

        string escapedQuery;
        CopyEscapedLiteral(escapedQuery, query);
        sql = sqlite3_mprintf("SELECT * FROM \"__DIRECTORY__\" WHERE QUERY = '%s'", escapedQuery.c_str());
        sqlret = sqlite3_get_table(__sqlite, sql, &prows, &nrows, &ncols, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) 
            return false;
        if(nrows == 0) 
            return false;

        string tableName;
        if(prows[ncols+0])
            tableName = prows[ncols+0];
        __idxFile.clear();
        if(prows[ncols+2])
            __idxFile = prows[ncols+2];
        __idxNumRows = 0;
        if(prows[ncols+3])
            __idxNumRows = atoi(prows[ncols+3]);
        if(tableName.empty() || __idxFile.empty() || __idxNumRows == 0) {
            __dropTable(query);
            return false;
        }
        sqlite3_free_table(prows);

        // check stat against .meta.table
        sql = sqlite3_mprintf("SELECT * FROM \"__FGLOB_STAT_%s__\" WHERE \"__METAFILE_STAT\" = 1", tableName.c_str());
        sqlret = sqlite3_get_table(__sqlite, sql, &prows, &nrows, &ncols, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) {
            __dropTable(query);
            return false;
        }
        if((nrows) != 1) {
            sqlite3_free_table(prows);
            __dropTable(query);
            return false;
        }
        if( (atoi((const char *)prows[(ncols) + 0]) != __metafile_stat.st_dev) || 
            (atoi((const char *)prows[(ncols) + 1]) != __metafile_stat.st_ino) ||
            (atoi((const char *)prows[(ncols) + 2]) != __metafile_stat.st_mtime)) {

            sqlite3_free_table(prows);
            __dropTable(query);
            return false;
        }
        sqlite3_free_table(prows);

        sql = sqlite3_mprintf("SELECT * FROM \"__FGLOB_STAT_%s__\" WHERE \"__METAFILE_STAT\" = 0", tableName.c_str());
        sqlret = sqlite3_get_table(__sqlite, sql, &prows, &nrows, &ncols, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) {
            __dropTable(query);
            return false;
        }
        if((nrows) != __glob.size()) {
            sqlite3_free_table(prows);
            __dropTable(query);
            return false;
        }

        for(globItr = __glob.begin(); globItr != __glob.end(); globItr++) {
            fd = ::open((*globItr).c_str(), O_RDONLY);
            if(fd == -1) {
                sqlite3_free_table(prows);
                __dropTable(query);
                return false;
            }
            fstat(fd, &sb);
            close(fd);

            for(idx = 1; idx <= nrows; idx++) {
                if( (atoi((const char *)prows[(idx*ncols) + 0]) == sb.st_dev) && 
                    (atoi((const char *)prows[(idx*ncols) + 1]) == sb.st_ino) &&
                    (atoi((const char *)prows[(idx*ncols) + 2]) == sb.st_mtime))
                        break;
            }
            if(idx == nrows + 1) {
                sqlite3_free_table(prows);
                __dropTable(query);
                return false;
            }
        }

        sqlite3_free_table(prows);

        sql = sqlite3_mprintf("SELECT * FROM \"%s\"", tableName.c_str());
        sqlret = sqlite3_prepare_v2(__sqlite, sql, -1, &__stmt, NULL);
        sqlite3_free(sql);
        if(sqlret == SQLITE_OK) {
            INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__loadFromSqlite", "Query \"%s\" satisfied by cached table %s", 
                query.c_str(), tableName.c_str());
            return true;
        }
        return false;
    }

    bool __storeToSqlite(string& tableName, const char *count_file)
    {
        int fd;
        struct stat sb;
        if((fd = ::open(count_file, O_RDONLY)) == -1)
            return false;
        fstat(fd, &sb);

        int matches = __getRowCount(fd, sb.st_size);
        string idxPath = __path + s_R1Caches + "/" + tableName + ".txt";

        char * errp;
        char * sql = sqlite3_mprintf("UPDATE \"__DIRECTORY__\" SET IDX_FILE = '%s', NUM_ROWS = %d WHERE ID = '%s'", idxPath.c_str(), matches, tableName.c_str());
        int sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);
        if(sqlret != SQLITE_OK) 
            return false;

        close(fd);
        return true;
    }

    int __getColumnNum(string colTag) 
    {
        int idx;
        vector<string>::iterator itr;
        for(idx = 0, itr = __metaTags.begin(); itr != __metaTags.end(); itr++, idx++) {
            if((*itr) == colTag) 
                return idx;
        }
        return -1;
    }

    string __getErrorMessage(char *jsonFile)
    {
        string message;

        int fd;
        struct stat sb;
        if((fd = ::open(jsonFile, O_RDONLY)) == -1)
            return message;
        fstat(fd, &sb);

        if(sb.st_size == 0)
            return message;

        char *buffer = (char *)malloc(sb.st_size);
        if(!buffer)
            return message;

        json_object *jobj;
        json_tokener *jtok = json_tokener_new();
        enum json_tokener_error jerr;

        int read_count = read(fd, buffer, sb.st_size);
        if(read_count == -1) {
            free(buffer);
            json_tokener_free(jtok);
            return message;
        }
        jobj = json_tokener_parse_ex(jtok, buffer, read_count);
        free(buffer);

        jobj = json_object_object_get(jobj, "message");
        message = json_object_get_string(jobj);
        json_tokener_free(jtok);
        return message;
    }

    int __getRowCount(int fd, size_t stSize)
    {
        int matches = -1;
        int read_count;
        char *buffer = (char *)malloc(stSize);
        if(!buffer)
            return false;

        json_object *jobj;
        json_tokener *jtok = json_tokener_new();
        enum json_tokener_error jerr;

        read_count = read(fd, buffer, stSize);
        if(read_count == -1) {
            free(buffer);
            json_tokener_free(jtok);
            return -1;
        }
        jobj = json_tokener_parse_ex(jtok, buffer, read_count);

        free(buffer);

        jobj = json_object_object_get(jobj, "matches");
        matches = json_object_get_int(jobj);
        json_tokener_free(jtok);
        return matches;
    }

    void __date(const char *dateStr, int colIdx, struct tm *date)
    {
        memset(date, 0, sizeof(struct tm));
        if(!dateStr)
            return;

        switch(__cols[colIdx].m_dtType) {
        case DATE_YYMMDD:
        case DATE_YYYYMMDD:
            sscanf(dateStr, __cols[colIdx].m_formatSpec.c_str(), &(date->tm_year), &(date->tm_mon), &(date->tm_mday));
            break;
        case DATE_DDMMYY:
        case DATE_DDMMYYYY:
            sscanf(dateStr, __cols[colIdx].m_formatSpec.c_str(), &(date->tm_mday), &(date->tm_mon), &(date->tm_year));
            break;
        case DATE_MMDDYY:
        case DATE_MMDDYYYY:
            sscanf(dateStr, __cols[colIdx].m_formatSpec.c_str(), &(date->tm_mon), &(date->tm_mday), &(date->tm_year));
            break;
        }
    }

    void __date(int colIdx, struct tm *date)
    {
        switch(__cols[colIdx].m_dtType) {
        case DATE_YYMMDD:
        case DATE_YYYYMMDD:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                date->tm_year, date->tm_mon, date->tm_mday);
            break;
        case DATE_DDMMYY:
        case DATE_DDMMYYYY:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                date->tm_mday, date->tm_mon, date->tm_year);
            break;
        case DATE_MMDDYY:
        case DATE_MMDDYYYY:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                date->tm_mon, date->tm_mday, date->tm_year);
            break;
        }
    }

    void __time(const char *timeStr, int colIdx, struct tm *time)
    {
        char ampm[16];
        char *pampm;

        memset(time, 0, sizeof(struct tm));
        if(!timeStr)
            return;

        switch(__cols[colIdx].m_dtType) {
        case TIME_12MMSS:
            sscanf(timeStr, __cols[colIdx].m_formatSpec.c_str(), &(time->tm_hour), &(time->tm_min), &(time->tm_sec), ampm);
            pampm = ampm;
            while (isspace(*pampm))
                pampm++;
            time->tm_hour = (time->tm_hour % 12);
            if(!strcasecmp(pampm, "pm")) {
                time->tm_hour += 12;
            }
            break;
        case TIME_24MMSS:
            sscanf(timeStr, __cols[colIdx].m_formatSpec.c_str(), &(time->tm_hour), &(time->tm_min), &(time->tm_sec));
            break;
        }
    }

    void __time(int colIdx, struct tm *time)
    {
        int hour = time->tm_hour % 12;
        switch(__cols[colIdx].m_dtType) {
        case TIME_12MMSS:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                hour ? hour : 12, time->tm_min, time->tm_sec, (time->tm_hour < 12) ? "AM" : "PM");
            break;
        case TIME_24MMSS:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                time->tm_hour, time->tm_min, time->tm_sec);
            break;
        }
    }

    void __datetime(const char *datetimeStr, int colIdx, struct tm *datetime)
    {
        char ampm[16];
        char *pampm;

        memset(datetime, 0, sizeof(struct tm));
        if(!datetimeStr)
            return;

        switch(__cols[colIdx].m_dtType) {
        case DATETIME_YYYYMMDD_12MMSS:
        case DATETIME_YYMMDD_12MMSS:
            sscanf(datetimeStr, __cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_year), &(datetime->tm_mon), &(datetime->tm_mday),
                &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec), ampm);
            pampm = ampm;
            while (isspace(*pampm))
                pampm++;
            datetime->tm_hour = (datetime->tm_hour % 12);
            if(!strcasecmp(pampm, "pm")) {
                datetime->tm_hour += 12;
            }
            break;
        case DATETIME_DDMMYYYY_12MMSS:
        case DATETIME_DDMMYY_12MMSS:
            sscanf(datetimeStr, __cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_mday), &(datetime->tm_mon), &(datetime->tm_year),
                &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec), ampm);
            pampm = ampm;
            while (isspace(*pampm))
                pampm++;
            datetime->tm_hour = (datetime->tm_hour % 12);
            if(!strcasecmp(pampm, "pm")) {
                datetime->tm_hour += 12;
            }
            break;
        case DATETIME_MMDDYYYY_12MMSS:
        case DATETIME_MMDDYY_12MMSS:
            sscanf(datetimeStr, __cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_mon), &(datetime->tm_mday), &(datetime->tm_year),
                &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec), ampm);
            pampm = ampm;
            while (isspace(*pampm))
                pampm++;
            datetime->tm_hour = (datetime->tm_hour % 12);
            if(!strcasecmp(pampm, "pm")) {
                datetime->tm_hour += 12;
            }
            break;
        case DATETIME_YYYYMMDD_24MMSS:
        case DATETIME_YYMMDD_24MMSS:
            sscanf(datetimeStr, __cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_year), &(datetime->tm_mon), &(datetime->tm_mday),
                &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec));
            break;
        case DATETIME_DDMMYYYY_24MMSS:
        case DATETIME_DDMMYY_24MMSS:
            sscanf(datetimeStr, __cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_mday), &(datetime->tm_mon), &(datetime->tm_year),
                &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec));
            break;
        case DATETIME_MMDDYYYY_24MMSS:
        case DATETIME_MMDDYY_24MMSS:
            sscanf(datetimeStr, __cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_mon), &(datetime->tm_mday), &(datetime->tm_year),
                &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec));
            break;
        }
    }

    void __datetime(int colIdx, struct tm *datetime)
    {
        int hour = datetime->tm_hour % 12;
        switch(__cols[colIdx].m_dtType) {
        case DATETIME_YYYYMMDD_12MMSS:
        case DATETIME_YYMMDD_12MMSS:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                datetime->tm_year, datetime->tm_mon, datetime->tm_mday,  hour ? hour : 12, datetime->tm_min, datetime->tm_sec, 
                (datetime->tm_hour < 12) ? "AM" : "PM");
            break;
        case DATETIME_DDMMYYYY_12MMSS:
        case DATETIME_DDMMYY_12MMSS:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                datetime->tm_mday, datetime->tm_mon, datetime->tm_year, hour ? hour : 12, datetime->tm_min, datetime->tm_sec, 
                (datetime->tm_hour < 12) ? "AM" : "PM");
            break;
        case DATETIME_MMDDYYYY_12MMSS:
        case DATETIME_MMDDYY_12MMSS:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                datetime->tm_mon, datetime->tm_mday, datetime->tm_year, hour ? hour : 12, datetime->tm_min, datetime->tm_sec, 
                (datetime->tm_hour < 12) ? "AM" : "PM");
            break;
        case DATETIME_YYYYMMDD_24MMSS:
        case DATETIME_YYMMDD_24MMSS:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                datetime->tm_year, datetime->tm_mon, datetime->tm_mday, datetime->tm_hour, datetime->tm_min, datetime->tm_sec);
            break;
        case DATETIME_DDMMYYYY_24MMSS:
        case DATETIME_DDMMYY_24MMSS:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                datetime->tm_mday, datetime->tm_mon, datetime->tm_year, datetime->tm_hour, datetime->tm_min, datetime->tm_sec);
            break;
        case DATETIME_MMDDYYYY_24MMSS:
        case DATETIME_MMDDYY_24MMSS:
            sprintf(__cursor.__row[colIdx].colResult.text, __cols[colIdx].m_formatSpec.c_str(), 
                datetime->tm_mon, datetime->tm_mday, datetime->tm_year, datetime->tm_hour, datetime->tm_min, datetime->tm_sec);
            break;
        }
    }

};

struct vtab : public sqlite3_vtab
{
    IQueryResult *__qresult;
};
struct vtab_cursor : public sqlite3_vtab_cursor
{
    sqlite_int64 __rowId;
    IQueryResult *__qresult;
};

int xCreate(sqlite3* in_sqlite, void *in_qresult, int in_argc, const char * const * in_argv, sqlite3_vtab **out_ppVTab, char**out_pzErr)
{
    vtab * out_vtab = new vtab;
    if(!out_vtab) 
        return SQLITE_NOMEM;

    out_vtab->__qresult = (IQueryResult *)in_qresult;

    string coldefs;
    for(int idx = 3; idx < in_argc; idx++) {
        if(!coldefs.empty())
            coldefs += ",";
        coldefs += in_argv[idx];
    }
    char *sql = sqlite3_mprintf("CREATE TABLE \"%s\" (%s)", in_argv[2], coldefs.c_str());
    int sqlret = sqlite3_declare_vtab(in_sqlite, sql);
    sqlite3_free(sql);
    if(sqlret != SQLITE_OK) 
        return sqlret;

    *out_ppVTab = out_vtab;
    return SQLITE_OK;
}

int xConnect(sqlite3* in_sqlite, void *in_qresult, int in_argc, const char * const * in_argv, sqlite3_vtab **out_ppVTab, char**out_pzErr)
{
    return xCreate(in_sqlite, in_qresult, in_argc, in_argv, out_ppVTab, out_pzErr);
}

int xDisconnect(sqlite3_vtab *in_pVTab)
{
    return xDestroy(in_pVTab);
}

int xDestroy(sqlite3_vtab *in_pVTab)
{
    vtab * in_vtab = (vtab *)in_pVTab;
    delete in_vtab;
    return SQLITE_OK;
}

int xOpen(sqlite3_vtab *in_pVTab, sqlite3_vtab_cursor **out_ppCursor)
{
    vtab * in_vtab = (vtab *)in_pVTab;
    if(!in_vtab->__qresult->OpenIndexedResult())
        return SQLITE_ERROR;
    vtab_cursor * out_vtab_cursor = new vtab_cursor;
    if(!out_vtab_cursor)
        return SQLITE_NOMEM;
    out_vtab_cursor->__rowId = 0;
    out_vtab_cursor->__qresult = in_vtab->__qresult;
    *out_ppCursor = out_vtab_cursor;
    return SQLITE_OK;
}

int xClose(sqlite3_vtab_cursor *in_pVTabCursor)
{
    vtab_cursor * in_vtab_cursor = (vtab_cursor *)in_pVTabCursor;
    in_vtab_cursor->__qresult->CloseIndexedResult();
    delete in_vtab_cursor;
    return SQLITE_OK;
}

int xBestIndex(sqlite3_vtab *pVTab, sqlite3_index_info*)
{
    return SQLITE_OK;
}

int xFilter(sqlite3_vtab_cursor*, int idxNum, const char *idxStr, int argc, sqlite3_value **argv)
{
    return SQLITE_OK;
}

int xNext(sqlite3_vtab_cursor *in_pVTabCursor)
{
    vtab_cursor * in_vtab_cursor = (vtab_cursor *)in_pVTabCursor;
    in_vtab_cursor->__qresult->FetchNextIndexedResult();
    in_vtab_cursor->__rowId++;
    return SQLITE_OK;
}

int xEof(sqlite3_vtab_cursor *in_pVTabCursor)
{
    vtab_cursor * in_vtab_cursor = (vtab_cursor *)in_pVTabCursor;
    return in_vtab_cursor->__qresult->IndexedResultEof();
}

int xColumn(sqlite3_vtab_cursor *in_pVTabCursor, sqlite3_context* in_context, int in_col)
{
    vtab_cursor * in_vtab_cursor = (vtab_cursor *)in_pVTabCursor;
    const char *out_text = in_vtab_cursor->__qresult->GetIndexedResultColumn(in_col);
    sqlite3_result_text(in_context, out_text, -1, SQLITE_TRANSIENT);
    return SQLITE_OK;
}

int xRowid(sqlite3_vtab_cursor *in_pVTabCursor, sqlite3_int64 *pRowid)
{
    vtab_cursor * in_vtab_cursor = (vtab_cursor *)in_pVTabCursor;
    *pRowid = in_vtab_cursor->__rowId;
    return SQLITE_OK;
}

int xRename(sqlite3_vtab *pVtab, const char *zNew)
{
    return SQLITE_READONLY;
}
#endif