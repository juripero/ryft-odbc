#include <glob.h>
#include <dirent.h>
#include <stdio.h>
#include <libconfig.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <string.h>

#include "ryft1_result.h"
#include "ryft1_util.h"
#include "TypeDefines.h"
#include "NumberConverter.h"

RyftOne_Result::RyftOne_Result( ILogger *log ) : __hamming(0), __edit(0), __caseSensitive(true), __sqlite(0), __stmt(0), __log(log) { ; }

RyftOne_Result::~RyftOne_Result( ) 
{
    if(__stmt) 
        sqlite3_finalize(__stmt);
    __stmt = NULL;

    if(__sqlite)
        sqlite3_close(__sqlite);
    __sqlite = NULL;
}

void RyftOne_Result::open(string& in_name, vector<__catalog_entry__>::iterator in_catentry)
{
    __edit = 0;
    __hamming = 0;
    __query.clear();
    __caseSensitive = true;

    // JSON, XML
    __dataType = in_catentry->rdf_config.data_type;

    __loadTable(in_name, in_catentry);

    vector<__meta_config__::__meta_view__>::iterator viewItr;
    for(viewItr = in_catentry->meta_config.views.begin(); viewItr != in_catentry->meta_config.views.end(); viewItr++) {
        if(in_name == viewItr->view_name) {
            __query = viewItr->restriction;
            break;
        }
    }

    int idx;
    RyftOne_Column col;
    vector<__meta_config__::__meta_col__>::iterator colItr;
    for(idx = 0, colItr = in_catentry->meta_config.columns.begin(); colItr != in_catentry->meta_config.columns.end(); colItr++, idx++) {
        col.m_ordinal = idx+1;
        col.m_tableName = in_catentry->meta_config.table_name;
        col.m_colTag = colItr->rdf_name;
        col.m_colName = colItr->name;
        col.m_typeName = colItr->type_def;
        RyftOne_Util::RyftToSqlType(col.m_typeName, &col.m_dataType, &col.m_charCols, &col.m_bufLength, col.m_formatSpec, &col.m_dtType);
        col.m_description = colItr->description;
        __cols.push_back(col);
    }

    resultCol rcol;
    for(idx = 0; idx < __metaTags.size(); idx++) {
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

    char *errp;
    sqlret = sqlite3_exec(__sqlite, "CREATE TABLE \"__DIRECTORY__\" (ID TEXT, QUERY TEXT)", NULL, NULL, &errp);
    if(errp)
        sqlite3_free(errp);

    __queryFinished = false;
}

void RyftOne_Result::appendFilter(string in_filter, int in_hamming, int in_edit, bool in_caseSensitive)
{
    if(!__query.empty())
        __query += " AND ";

    __query += in_filter;
    __hamming = in_hamming;
    __edit = in_edit;
    __caseSensitive = in_caseSensitive;
}

void RyftOne_Result::prepareAppend()
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

void RyftOne_Result::finishUpdate()
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

bool RyftOne_Result::fetchFirst()
{
    if(!__queryFinished)
        __queryFinished = __execute();

    int sqlret = SQLITE_DONE;
    if(__stmt) 
        sqlite3_reset(__stmt);

    return fetchNext();
}

bool RyftOne_Result::fetchNext()
{
    int sqlret = SQLITE_DONE;
    if(__stmt) 
        sqlret = sqlite3_step(__stmt);

    return(sqlret == SQLITE_ROW);
}

void RyftOne_Result::closeCursor()
{
    if(__stmt) 
        sqlite3_finalize(__stmt);
    __stmt = NULL;
}

const char *RyftOne_Result::getStringValue(int colIdx)
{
    return (const char *)sqlite3_column_text(__stmt, colIdx);
}

int RyftOne_Result::getIntValue(int colIdx)
{
    return sqlite3_column_int(__stmt, colIdx);
}

long long RyftOne_Result::getInt64Value(int colIdx)
{
    return sqlite3_column_int64(__stmt, colIdx);
}

double RyftOne_Result::getDoubleValue(int colIdx)
{
    return sqlite3_column_double(__stmt, colIdx);
}

struct tm RyftOne_Result::getDateValue(int colIdx)
{
    struct tm date;
    __date((const char *)sqlite3_column_text(__stmt, colIdx), colIdx, &date);
    return date;
}

struct tm RyftOne_Result::getTimeValue(int colIdx)
{
    struct tm time;
    __date((const char *)sqlite3_column_text(__stmt, colIdx), colIdx, &time);
    return time;
}

struct tm RyftOne_Result::getDateTimeValue(int colIdx)
{
    struct tm ts;
    __date((const char *)sqlite3_column_text(__stmt, colIdx), colIdx, &ts);
    return ts;
}

void RyftOne_Result::putStringValue(int colIdx, string colValue)
{
    strcpy(__cursor.__row[colIdx].colResult.text, colValue.c_str());
}

void RyftOne_Result::flush()
{
    time_t now = ::time(NULL);
    tm *lt = localtime(&now);
    char file_path[PATH_MAX];
    char temp_path[PATH_MAX];
    IFile *formattedFile = NULL;
    sprintf(temp_path, "%s/%04d%02d%02d.%s%s", __path.c_str(), 1900 + lt->tm_year, lt->tm_mon + 1, lt->tm_mday, "journal_", __extension.c_str());
    sprintf(file_path, "%s/%04d%02d%02d.%s", __path.c_str(), 1900 + lt->tm_year, lt->tm_mon + 1, lt->tm_mday, __extension.c_str());
    if(__dataType == __rdf_config__::dataType_XML) {
        formattedFile = new XMLFile(temp_path, m_delim);
    }
    else if(__dataType == __rdf_config__::dataType_JSON) {
        formattedFile = new JSONFile(temp_path, __no_top);
    }
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

NodeAction RyftOne_Result::JSONStartRow()
{
    return StartRow();
}

NodeAction RyftOne_Result::StartRow()
{
    __cursor.__colIdx = -1;
    resultCols::iterator itr;
    for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
        *((*itr).colResult.text) = '\0';
    }
    return ProcessNode;
}

NodeAction RyftOne_Result::JSONStartArray( std::string sName )
{
    __qualifiedCol.push_back(sName + ".[]");
    return ProcessNode;
}

NodeAction RyftOne_Result::JSONStartGroup( std::string sName )
{
    __qualifiedCol.push_back(sName);
    return ProcessNode;
}

NodeAction RyftOne_Result::JSONAddElement( std::string sName, const char **psAttribs, JSONElement **ppElement )
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

NodeAction RyftOne_Result::AddElement(std::string sName, const char **psAttribs, XMLElement **ppElement)
{
    __cursor.__colIdx = __getColumnNum(sName);
    return ProcessNode;
}

void RyftOne_Result::JSONAddText( std::string sText )
{
    if(__cursor.__colIdx != -1) {
        if(strlen(__cursor.__row[__cursor.__colIdx].colResult.text) && !__qualifiedCol.empty())
            AddText(__delimiter);
        AddText(sText);
    }
}

void RyftOne_Result::AddText(std::string sText) 
{
    if(__cursor.__colIdx != -1) {
        strncat(__cursor.__row[__cursor.__colIdx].colResult.text, sText.c_str(), 
            __cursor.__row[__cursor.__colIdx].charCols - strlen(__cursor.__row[__cursor.__colIdx].colResult.text));
    }
}

void RyftOne_Result::JSONExitArray()
{
    __qualifiedCol.pop_back();
}

void RyftOne_Result::JSONExitGroup()
{
    __qualifiedCol.pop_back();
}

void RyftOne_Result::JSONExitRow()
{
    ExitRow();
}

void RyftOne_Result::ExitRow()
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

void RyftOne_Result::__loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr)
{
    __name = in_name;
    __path = in_itr->path;

    string path = in_itr->path;
    path += "/";
    path += in_itr->rdf_config.file_glob;

    const char *perr;
    size_t idx1 = in_itr->rdf_config.file_glob.find(".");
    if(idx1 != string::npos) 
        __extension = in_itr->rdf_config.file_glob.substr(idx1+1);

    glob_t glob_results;
    glob(path.c_str(), GLOB_TILDE, NULL, &glob_results);
    __loaded = rol_ds_create();
    char **relpaths = new char *[glob_results.gl_pathc];
    for(int i = 0; i < glob_results.gl_pathc; i++) {
        __glob.push_back(glob_results.gl_pathv[i]);
        relpaths[i] = glob_results.gl_pathv[i] + strlen("/ryftone/");
        INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__loadTable", "relpaths[i] = %s", relpaths[i]);
        rol_ds_add_file(__loaded, relpaths[i]);
        if(rol_ds_has_error_occurred(__loaded)) {
            perr = rol_ds_get_error_string(__loaded);
            ERROR_LOG(__log, "RyftOne", "RyftOne_Result", "__loadTable", "rol_ds_get_error_string returned = %s", perr);
        }
    }
    globfree(&glob_results);

    idx1 = in_itr->rdf_config.record_start.find("<");
    size_t idx2 = in_itr->rdf_config.record_start.find(">");
    if(idx1 == string::npos || idx2 == string::npos) {
        m_delim = in_itr->rdf_config.record_start;
    }
    else
        m_delim = in_itr->rdf_config.record_start.substr(idx1+1, idx2-idx1-1);

    __no_top = in_itr->rdf_config.no_top;

    vector<__meta_config__::__meta_col__>::iterator colItr;
    for(colItr = in_itr->meta_config.columns.begin(); colItr != in_itr->meta_config.columns.end(); colItr++) {
        if(__dataType == __rdf_config__::dataType_XML) {
            vector<__rdf_config__::__rdf_tag__>::iterator rdfItr;
            for(rdfItr = in_itr->rdf_config.tags.begin(); rdfItr != in_itr->rdf_config.tags.end(); rdfItr++) {
                if(rdfItr->name == colItr->rdf_name) {
                    idx1 = rdfItr->start_tag.find("<");
                    idx2 = rdfItr->start_tag.find(">");
                    __metaTags.push_back(rdfItr->start_tag.substr(idx1+1, idx2-idx1-1));
                }
            }
        }
        else
            __metaTags.push_back(colItr->rdf_name);
    }

    __delimiter = in_itr->meta_config.delimiter;
}

ILogger *__slog;

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
void sig_handler(int signum)
{
    if(__slog)
        INFO_LOG(__slog, "RyftOne", "RyftOne_Result", "__execute", "Caught signal = %d", signum);
    signal(signum, SIG_DFL);
    kill(getpid(), signum);
}

bool RyftOne_Result::__execute()
{
    const char *perr;
    char results[PATH_MAX];
    string query(__query);
    bool ret = false;
    string tableName;

    signal(SIGSEGV, sig_handler);
    setuid(1000);
    setgid(1000);

    if(query.empty()) {
        query = "( RECORD CONTAINS ? )";
    }
    else {
        // edit distance, hamming distance and case sensitivity need to be stored with query results
        char attribs[32];
        snprintf(attribs, sizeof(attribs), ";parms = ED:%d HD:%d CS:%s", __edit, __hamming, __caseSensitive ? "T" : "F");
        query += attribs;
    }

    INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__execute", "Executing query = %s", query.c_str());

    if(!(ret = __loadFromSqlite(query))) {

        if (!__initTable(query, tableName)) {
            __dropTable(query);
            return false;
        }

        if(__query.empty()) {
            vector<string>::iterator globItr;
            for(globItr = __glob.begin(); globItr != __glob.end(); globItr++) {
                ret = __storeToSqlite(tableName, (*globItr).c_str(), __no_top);
                if(!ret) {
                    __dropTable(query);
                    return false;
                }
            }
            return __loadFromSqlite(query);
        }
        else {
            mkdir(s_R1Results, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
            sprintf(results, "%s/results.%08x", s_R1Results, pthread_self());
            string results_path(results);
            size_t idx = results_path.find("/", 1);
            rol_data_set_t result;
            __slog = __log;
            INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__execute", "results_path = %s", results_path.substr(idx+1, string::npos).c_str());
            INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__execute", "uid = %d, gid = %d", getuid(), getgid());
            if(__edit) {
                INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__execute", "Issuing rol_ds_search_fuzzy_edit_distance");
                result = rol_ds_search_fuzzy_edit_distance(__loaded, results_path.substr(idx+1, string::npos).c_str(), 
                    __query.c_str(), 0, __edit, "\r\n", NULL, __caseSensitive, true, NULL);
            }
            else {
                INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__execute", "Issuing rol_ds_search_fuzzy_hamming");
                result = rol_ds_search_fuzzy_hamming(__loaded, results_path.substr(idx+1, string::npos).c_str(), 
                    __query.c_str(), 0, __hamming, "\r\n", NULL, __caseSensitive, NULL);
            }
            INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__execute", "result = %d, gid = %d", result);
            if( rol_ds_has_error_occurred(result)) {
                perr = rol_ds_get_error_string(result);
                INFO_LOG(__log, "RyftOne", "RyftOne_Result", "__execute", "rol_ds_has_error_occurred perr = %s", perr);
                __dropTable(query);
                simba_wstring errorMsg = simba_wstring::CreateFromUTF8(perr);
                R1THROWGEN1(L"RolException", errorMsg.GetAsPlatformWString());
                return false;
            }
            ret = __storeToSqlite(tableName, results, true);
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

int RyftOne_Result::__getColumnNum(string colTag) 
{
    int idx;
    vector<string>::iterator itr;
    for(idx = 0, itr = __metaTags.begin(); itr != __metaTags.end(); itr++, idx++) {
        if((*itr) == colTag) 
            return idx;
    }
    return -1;
}

bool RyftOne_Result::__initTable(string& query, string& tableName)
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

void RyftOne_Result::__dropTable(string& query)
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

bool RyftOne_Result::__storeToSqlite(string& tableName, const char *file, bool no_top)
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

    if(__dataType == __rdf_config__::dataType_XML) {
        XMLParse(fd, sb.st_size, m_delim, no_top);
    }
    else if(__dataType == __rdf_config__::dataType_JSON) {
        JSONParse(fd, sb.st_size, no_top);   
    }
    
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

bool RyftOne_Result::__loadFromSqlite(string& query)
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

void RyftOne_Result::__date(const char *dateStr, int colIdx, struct tm *date)
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

void RyftOne_Result::__time(const char *timeStr, int colIdx, struct tm *time)
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

void RyftOne_Result::__datetime(const char *datetimeStr, int colIdx, struct tm *datetime)
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

