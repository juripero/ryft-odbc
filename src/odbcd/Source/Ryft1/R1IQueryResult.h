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
    string m_colTag;
    string m_colName;
    string m_description;
    unsigned m_dataType;
    string m_typeName;
    unsigned m_charCols;
    unsigned m_bufLength;
    unsigned m_dtType;
    string m_formatSpec;
} RyftOne_Column;
typedef vector<RyftOne_Column> RyftOne_Columns;

class IQueryResult : public JSONParser {
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
        string in_server, string in_token, string in_path)
    {
        __query.clear();

        __restServer = in_server;
        __restToken = in_token;
        __restPath = in_path;

        // JSON, XML, Unstructured
        __dataType = in_catentry->rdf_config.data_type;

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
        dbpath += "/";
        dbpath += ".caches.db";
        int sqlret = sqlite3_open(dbpath.c_str(), &__sqlite);

        char *errp = NULL;
        sqlret = sqlite3_exec(__sqlite, "CREATE TABLE \"__DIRECTORY__\" (ID TEXT, QUERY TEXT)", NULL, NULL, &errp);
        if(errp)
            sqlite3_free(errp);

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
        unlink(temp_path);
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
        return (const char *)sqlite3_column_text(__stmt, colIdx);
    }

    int GetIntValue(int colIdx)
    {
        return sqlite3_column_int(__stmt, colIdx);
    }

    long long GetInt64Value(int colIdx)
    {
        return sqlite3_column_int64(__stmt, colIdx);
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
        
    void GetTypeFormatSpecifier(int colIdx, unsigned *dtType, string& formatSpec)
    {
        *dtType = __cols[colIdx].m_dtType;
        formatSpec = __cols[colIdx].m_formatSpec;
    }

    bool IsStructuredType() 
    {
        return __isStructuredType();
    }

protected:

    virtual NodeAction JSONStartRow()
    {
        return __startRow();
    }

    NodeAction __startRow()
    {
        __cursor.__colIdx = -1;
        resultCols::iterator itr;
        for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
            *((*itr).colResult.text) = '\0';
        }
        return ProcessNode;
    }

    virtual NodeAction JSONStartArray( std::string sName )
    {
        __qualifiedCol.push_back(sName + ".[]");
        return ProcessNode;
}

    virtual NodeAction JSONStartGroup( std::string sName )
    {
        __qualifiedCol.push_back(sName);
        return ProcessNode;
    }

    virtual NodeAction JSONAddElement( std::string sName, const char **psAttribs, JSONElement **ppElement )
    {
        string qualifiedName;
        deque<string>::iterator itr;
        for(itr = __qualifiedCol.begin(); itr != __qualifiedCol.end(); itr++) {
            if(!qualifiedName.empty())
                qualifiedName += ".";
            qualifiedName += (*itr);
        }
        if(!qualifiedName.empty())
            qualifiedName += ".";
        qualifiedName += sName;
        __cursor.__colIdx = __getColumnNum(qualifiedName);
        return ProcessNode;
    }

    NodeAction __addElement(std::string sName, const char **psAttribs, XMLElement **ppElement)
    {
        __cursor.__colIdx = __getColumnNum(sName);
        return ProcessNode;
    }

    virtual void JSONAddText( std::string sText )
    {
        if(__cursor.__colIdx != -1) {
            if(strlen(__cursor.__row[__cursor.__colIdx].colResult.text) && !__qualifiedCol.empty())
                __addText(__delimiter);
            __addText(sText);
        }
    }

    void __addText(std::string sText) 
    {
        if(__cursor.__colIdx != -1) {
            strncat(__cursor.__row[__cursor.__colIdx].colResult.text, sText.c_str(), 
                __cursor.__row[__cursor.__colIdx].charCols - strlen(__cursor.__row[__cursor.__colIdx].colResult.text));
        }
    }

    virtual void JSONExitArray()
    {
        __qualifiedCol.pop_back();
    }

    virtual void JSONExitGroup()
    {
        __qualifiedCol.pop_back();
    }

    virtual void JSONExitRow()
    {
        __exitRow();
    }

    void __exitRow()
    {
        int sqlret;
        char *errp;
        if(__transMax == 1000) {
            sqlret = sqlite3_exec(__sqlite, "COMMIT", NULL, NULL, &errp);
            if(errp)
                sqlite3_free(errp);
            sqlret = sqlite3_exec(__sqlite, "BEGIN", NULL, NULL, &errp);
            if(errp)
                sqlite3_free(errp);
            __transMax = 0;
        }
        if(__stmt) {
            resultCols::iterator itr;
            for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
                // bind as text, sqlite will do the conversion into the right storage class
                sqlret = sqlite3_bind_text(__stmt, (*itr).colNum + 1, (const char *)(*itr).colResult.text, -1, SQLITE_STATIC);
            }
            sqlret = sqlite3_step(__stmt);
            sqlret = sqlite3_reset(__stmt);
            __transMax++;
        }
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
        return "raw";
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
        const char *perr;
        char results[PATH_MAX];
        string query(__query);
        bool ret = false;
        string tableName;

        if(query.empty()) {
            query = "( RECORD CONTAINS ? )";
        }

        INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__execute", "Executing query = %s", query.c_str());

        if(!(ret = __loadFromSqlite(query))) {
            vector<string>::iterator globItr;

            if (!__initTable(query, tableName)) {
                __dropTable(query);
                return false;
            }

            if(__query.empty()) {
                for(globItr = __glob.begin(); globItr != __glob.end(); globItr++) {
                    ret = __storeToSqlite(tableName, (*globItr).c_str(), __no_top, "");
                    if(!ret) {
                        __dropTable(query);
                        return false;
                    }
                }
                return __loadFromSqlite(query);
            }
            else {
                curl_global_init(CURL_GLOBAL_DEFAULT);
                CURL *curl = curl_easy_init();

                if(!__restToken.empty()) {
                    struct curl_slist *header = NULL;
                    string auth = "Authorization: Basic " + __restToken;
                    header = curl_slist_append(header, auth.c_str());
                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
                }

                string url = __restServer + "/search?query=" + RyftOne_Util::UrlEncode(__query);
            
                string relPath = __path.c_str() + __restPath.length();
                url += "&files=" + relPath + RyftOne_Util::UrlEncode("/") + "*." + __extension; 

                url += "&format=" + __getFormatString();

                url += "&local=true";

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

                mkdir(s_R1Results, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
                sprintf(results, "%s/results.%08x", s_R1Results, pthread_self());

                FILE *f = fopen(results, "w+");

                curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
                CURLcode code = curl_easy_perform(curl);

                curl_easy_cleanup(curl);
                curl_global_cleanup();
                fclose(f);

                ret = __storeToSqlite(tableName, results, false, "results");
                unlink(results);
                if(!ret) {
                    __dropTable(query);
                    return false;
                }
                return __loadFromSqlite(query);
            }
        }
        return ret;
    }

    virtual void __parse(int fd, size_t st_size, bool no_top, string top_object)
    {
        JSONParse(fd, st_size, no_top, top_object);   
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

    bool __queryFinished;

    RyftOne_Columns __cols;

    __rdf_config__::DataType __dataType;

    sqlite3 *__sqlite;
    sqlite3_stmt *__stmt;
    int __transMax;

    Cursor __cursor;

    deque<string> __qualifiedCol;

    bool __initTable(string& query, string& tableName)
    {
        char * errp;

        uuid_t id;
        uuid_generate(id);
        char stringId[100];
        uuid_unparse(id, stringId);
        for(char *pstr = stringId; *pstr; pstr++)
            *pstr = toupper(*pstr);
        tableName = stringId;

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

        char *sql = sqlite3_mprintf("CREATE TABLE \"%s\" (%s)", tableName.c_str(), coldefs.c_str());
        int sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) 
            return false;

        sql = sqlite3_mprintf("CREATE TABLE \"__FGLOB_STAT_%s__\" (\"__ST_DEV\" INTEGER, \"__ST_INO\" INTEGER, \"__ST_MTIME\" INTEGER)", 
            tableName.c_str());
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
            sql = sqlite3_mprintf("INSERT INTO \"__FGLOB_STAT_%s__\" VALUES (%d,%d,%d)", tableName.c_str(), sb.st_dev, sb.st_ino, sb.st_mtime);
            sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
            sqlite3_free(sql);
            if(errp)
                sqlite3_free(errp);
            if(sqlret != SQLITE_OK) 
                return false;
        }

        string escapedQuery;
        CopyEscapedLiteral(escapedQuery, query);
        sql = sqlite3_mprintf("INSERT INTO __DIRECTORY__ VALUES ('%s','%s')", tableName.c_str(), escapedQuery.c_str());
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
        sqlite3_free_table(prows);
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

        string tableName = prows[ncols+0];
        sqlite3_free_table(prows);
        sql = sqlite3_mprintf("SELECT * FROM \"__FGLOB_STAT_%s__\"", tableName.c_str());
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

    bool __storeToSqlite(string& tableName, const char *file, bool no_top, string top_object )
    {
        int fd;
        struct stat sb;
        int idx;

        if((fd = ::open(file, O_RDONLY)) == -1)
            return false;

        fstat(fd, &sb);

        string values;
        for(idx = 0; idx < __metaTags.size(); idx++) {
            if(!values.empty())
                values += ",";
            values += "?";
        }

        char * errp;
        char * sql = sqlite3_mprintf("INSERT INTO \"%s\" VALUES (%s)", tableName.c_str(), values.c_str() );
        int sqlret = sqlite3_prepare_v2(__sqlite, sql, -1, &__stmt, NULL);
        sqlite3_free(sql);
        if(sqlret != SQLITE_OK) 
            return false;
    
        resultCols::iterator itr;
        for(idx = 0, itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++, idx++) {
            (*itr).colResult.text = (char *)sqlite3_malloc((*itr).charCols + 1);
        }
        sqlret = sqlite3_exec(__sqlite, "BEGIN", NULL, NULL, &errp);
        if(errp)
            sqlite3_free(errp);
        __transMax = 0;

        __parse(fd, sb.st_size, no_top, top_object);

        sqlret = sqlite3_exec(__sqlite, "COMMIT", NULL, NULL, &errp);
        if(errp)
            sqlite3_free(errp);

        for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
            sqlite3_free((*itr).colResult.text);
        }

        sqlite3_finalize(__stmt);
        __stmt = NULL;

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

    void __date(const char *dateStr, int colIdx, struct tm *date)
    {
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

    void __time(const char *timeStr, int colIdx, struct tm *time)
    {
        char ampm[16];
        char *pampm;
        switch(__cols[colIdx].m_dtType) {
        case TIME_12MMSS:
            sscanf(timeStr, __cols[colIdx].m_formatSpec.c_str(), &(time->tm_hour), &(time->tm_min), &(time->tm_sec), ampm);
            pampm = ampm;
            while (isspace(*pampm))
                pampm++;
            if(!strcasecmp(pampm, "pm"))
                time->tm_hour += 12;
            break;
        case TIME_24MMSS:
            sscanf(timeStr, __cols[colIdx].m_formatSpec.c_str(), &(time->tm_hour), &(time->tm_min), &(time->tm_sec));
            break;
        }
    }

    void __datetime(const char *datetimeStr, int colIdx, struct tm *datetime)
    {
        char ampm[16];
        char *pampm;
        switch(__cols[colIdx].m_dtType) {
        case DATETIME_YYYYMMDD_12MMSS:
        case DATETIME_YYMMDD_12MMSS:
            sscanf(datetimeStr, __cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_year), &(datetime->tm_mon), &(datetime->tm_mday),
                &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec), ampm);
            pampm = ampm;
            while (isspace(*pampm))
                pampm++;
            if(!strcasecmp(pampm, "pm"))
                datetime->tm_hour += 12;
            break;
        case DATETIME_DDMMYYYY_12MMSS:
        case DATETIME_DDMMYY_12MMSS:
            sscanf(datetimeStr, __cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_mday), &(datetime->tm_mon), &(datetime->tm_year),
                &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec), ampm);
            pampm = ampm;
            while (isspace(*pampm))
                pampm++;
            if(!strcasecmp(pampm, "pm"))
                datetime->tm_hour += 12;
            break;
        case DATETIME_MMDDYYYY_12MMSS:
        case DATETIME_MMDDYY_12MMSS:
            sscanf(datetimeStr, __cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_mon), &(datetime->tm_mday), &(datetime->tm_year),
                &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec), ampm);
            pampm = ampm;
            while (isspace(*pampm))
                pampm++;
            if(!strcasecmp(pampm, "pm"))
                datetime->tm_hour += 12;
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
};
#endif