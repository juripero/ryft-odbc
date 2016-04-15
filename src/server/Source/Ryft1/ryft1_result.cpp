#include <glob.h>
#include <dirent.h>
#include <stdio.h>
#include <libconfig.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "ryft1_result.h"
#include "ryft1_util.h"
#include "TypeDefines.h"
#include "NumberConverter.h"

static char s_R1Results[] = "/ryftone/ODBC/.results";

RyftOne_Result::RyftOne_Result( ) : __hamming(0), __edit(0), __caseSensitive(true), __sqlite(0), __stmt(0) { ; }
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
    __query.clear();
    __hamming = 0;
    __edit = 0;
    __caseSensitive = true;

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
    for(idx  =0, colItr = in_catentry->meta_config.columns.begin(); colItr != in_catentry->meta_config.columns.end(); colItr++, idx++) {
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
    for(idx = 0; idx < __rdfTags.size(); idx++) {
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
    for(idx = 0, itr = __rdfTags.begin(); itr != __rdfTags.end(); itr++, idx++) {
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
    for(idx = 0; idx < __rdfTags.size(); idx++) {
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
    
    sprintf(file_path, "%s/%04d%02d%02d.%s", __path.c_str(), 1900 + lt->tm_year, lt->tm_mon + 1, lt->tm_mday, __extension.c_str());
    FILE *f = fopen(file_path, "a");
    if(f == NULL)
        return;
    
    char **prows;
    int nrows, ncols;
    char *errp;
    int sqlret = sqlite3_get_table(__sqlite, "SELECT * FROM \"__APPENDED__\"", &prows, &nrows, &ncols, &errp);
    if(errp)
        sqlite3_free(errp);
    if(sqlret != SQLITE_OK)
        return;

    int idx;
    int colIdx;
    for(idx = 1; idx <= nrows; idx++) {
        std::vector<resultCol>::iterator itr;
        for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) 
            (*itr).colResult.text = prows[idx * ncols + (*itr).colNum];

        __writeRow(f);
    }
    fclose(f);
}

IElement::NodeAction RyftOne_Result::StartRow()
{
    __cursor.__colIdx = -1;
    resultCols::iterator itr;
    for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
        *((*itr).colResult.text) = '\0';
    }
    return ProcessNode;
}

IElement::NodeAction RyftOne_Result::AddElement(std::string sName, const char **psAttribs, IElement **ppElement)
{
    __cursor.__colIdx = __getColumnNum(sName);
    return ProcessNode;
}

void RyftOne_Result::AddText(std::string sText) 
{
    if(__cursor.__colIdx != -1) {
        strncat(__cursor.__row[__cursor.__colIdx].colResult.text, sText.c_str(), 
            __cursor.__row[__cursor.__colIdx].charCols - strlen(__cursor.__row[__cursor.__colIdx].colResult.text));
    }
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

    __rdf_config__ rdf_config(in_itr->meta_config.rdf_path);
    string path = in_itr->path;
    __path = in_itr->path;

    path += "/";
    path += rdf_config.file_glob;
    const char *perr;

    size_t idx1 = rdf_config.file_glob.find(".");
    if(idx1 != string::npos) 
        __extension = rdf_config.file_glob.substr(idx1+1);

    glob_t glob_results;
    glob(path.c_str(), GLOB_TILDE, NULL, &glob_results);
    __loaded = rol_ds_create();
    char **relpaths = new char *[glob_results.gl_pathc];
    for(int i = 0; i < glob_results.gl_pathc; i++) {
        __glob.push_back(glob_results.gl_pathv[i]);
        relpaths[i] = glob_results.gl_pathv[i] + strlen("/ryftone/");
        rol_ds_add_file(__loaded, relpaths[i]);
        if(rol_ds_has_error_occurred(__loaded)) {
            perr = rol_ds_get_error_string(__loaded);
        }
    }
    globfree(&glob_results);

    idx1 = rdf_config.record_start.find("<");
    size_t idx2 = rdf_config.record_start.find(">");
    if(idx1 == string::npos || idx2 == string::npos) {
        m_delim = rdf_config.record_start;
    }
    else
        m_delim = rdf_config.record_start.substr(idx1+1, idx2-idx1-1);

    vector<__meta_config__::__meta_col__>::iterator colItr;
    for(colItr = in_itr->meta_config.columns.begin(); colItr != in_itr->meta_config.columns.end(); colItr++) {
        vector<__rdf_config__::__rdf_tag__>::iterator rdfItr;
        for(rdfItr = rdf_config.tags.begin(); rdfItr != rdf_config.tags.end(); rdfItr++) {
            if(rdfItr->name == colItr->rdf_name) {
                idx1 = rdfItr->start_tag.find("<");
                idx2 = rdfItr->start_tag.find(">");
                __rdfTags.push_back(rdfItr->start_tag.substr(idx1+1, idx2-idx1-1));
            }
        }
    }
}

bool RyftOne_Result::__execute()
{
    const char *perr;
    char results[PATH_MAX];
    string query(__query);
    bool ret = false;

    if(query.empty()) {
        query = "( RECORD CONTAINS ? )";
    }
    else {
        // edit distance, hamming distance and case sensitivity need to be stored with query results
        char attribs[32];
        snprintf(attribs, sizeof(attribs), " = E:%d H:%d I:%d", __edit, __hamming, __caseSensitive);
        query += attribs;
    }

    if(!(ret = __loadFromSqlite(query))) {

        if (!__initTable(query))
            return false;

        if(__query.empty()) {
            vector<string>::iterator globItr;
            for(globItr = __glob.begin(); globItr != __glob.end(); globItr++) {
                ret = __storeToSqlite(query, (*globItr).c_str(), false);
                if(!ret)
                    return false;
            }
            return __loadFromSqlite(query);
        }
        else {
            mkdir(s_R1Results, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
            sprintf(results, "%s/results.%08x", s_R1Results, pthread_self());
            string results_path(results);
            size_t idx = results_path.find("/", 1);
            rol_data_set_t result;
            if(__edit) {
                result = rol_ds_search_fuzzy_edit_distance(__loaded, results_path.substr(idx+1, string::npos).c_str(), 
                    __query.c_str(), 0, __edit, NULL, NULL, __caseSensitive, true, NULL);
            }
            else 
                result = rol_ds_search_fuzzy_hamming(__loaded, results_path.substr(idx+1, string::npos).c_str(), 
                    __query.c_str(), 0, __hamming, NULL, NULL, __caseSensitive, NULL);
            if( rol_ds_has_error_occurred(result)) {
                perr = rol_ds_get_error_string(result);
                return false;
            }
            ret = __storeToSqlite(query, results, true);
            unlink(results);
            if(!ret)
                return false;
            return __loadFromSqlite(query);
        }
    }
    return ret;
}

int RyftOne_Result::__getColumnNum(string colTag) 
{
    int idx;
    vector<string>::iterator itr;
    for(idx = 0, itr = __rdfTags.begin(); itr != __rdfTags.end(); itr++, idx++) {
        if((*itr) == colTag) 
            return idx;
    }
    return -1;
}

bool RyftOne_Result::__writeRow(FILE *f)
{
    std::vector<resultCol>::iterator itr;
    fprintf(f, "<%s>", m_delim.c_str());
    for(itr = __cursor.__row.begin(); itr != __cursor.__row.end(); itr++) {
        fprintf(f, "<%s>%s</%s>", __rdfTags[(*itr).colNum].c_str(), (char *)(*itr).colResult.text, __rdfTags[(*itr).colNum].c_str());
    }
    fprintf(f, "</%s>\n", m_delim.c_str());
}

bool RyftOne_Result::__initTable(string& table)
{
    char * errp;
    string tableName;
    CopyEscapedIdentifier(tableName, table);
    char * sql = sqlite3_mprintf("DROP TABLE \"%s\"", tableName.c_str());
    sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
    sqlite3_free(sql);

    int idx;
    string coldefs;
    vector<string>::iterator itr;
    for(idx = 0, itr = __rdfTags.begin(); itr != __rdfTags.end(); itr++, idx++) {
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

    sql = sqlite3_mprintf("CREATE TABLE \"%s\" (%s)", tableName.c_str(), coldefs.c_str());
    int sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
    sqlite3_free(sql);
    if(errp)
        sqlite3_free(errp);
    if(sqlret != SQLITE_OK) 
        return false;

    sql = sqlite3_mprintf("CREATE TABLE \"_FGLOB_STAT_%s\" (\"__ST_DEV\" INTEGER, \"__ST_INO\" INTEGER, \"__ST_MTIME\" INTEGER)", 
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
        sql = sqlite3_mprintf("INSERT INTO \"_FGLOB_STAT_%s\" VALUES (%d,%d,%d)", tableName.c_str(), sb.st_dev, sb.st_ino, sb.st_mtime);
        sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
        sqlite3_free(sql);
        if(errp)
            sqlite3_free(errp);
        if(sqlret != SQLITE_OK) 
            return false;
    }
    return true;
}

bool RyftOne_Result::__storeToSqlite(string& table, const char *file, bool no_top)
{
    int fd;
    struct stat sb;
    int idx;

    if((fd = ::open(file, O_RDONLY)) == -1)
        return false;

    fstat(fd, &sb);

    string values;
    for(idx = 0; idx < __rdfTags.size(); idx++) {
        if(!values.empty())
            values += ",";
        values += "?";
    }

    char * errp;
    string tableName;
    CopyEscapedIdentifier(tableName, table);
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

    Parse(fd, sb.st_size, m_delim, no_top);
    
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

bool RyftOne_Result::__loadFromSqlite(string& table)
{
    char *sql;
    int sqlret;
    string tableName;
    char **prows;
    int nrows, ncols;
    char *errp;
    int fd;
    int idx;
    struct stat sb;
    vector<string>::iterator globItr;

    CopyEscapedIdentifier(tableName, table);

    sql = sqlite3_mprintf("SELECT * FROM \"_FGLOB_STAT_%s\"", tableName.c_str());
    sqlret = sqlite3_get_table(__sqlite, sql, &prows, &nrows, &ncols, &errp);
    sqlite3_free(sql);
    if(errp)
        sqlite3_free(errp);
    if(sqlret != SQLITE_OK)
        goto __destroy_cache;
    if((nrows) != __glob.size()) {
        sqlite3_free_table(prows);
        goto __destroy_cache;
    }

    for(globItr = __glob.begin(); globItr != __glob.end(); globItr++) {
        fd = ::open((*globItr).c_str(), O_RDONLY);
        if(fd == -1) {
            sqlite3_free_table(prows);
            goto __destroy_cache;
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
            goto __destroy_cache;
        }
    }

    sqlite3_free_table(prows);

    sql = sqlite3_mprintf("SELECT * FROM \"%s\"", tableName.c_str());
    sqlret = sqlite3_prepare_v2(__sqlite, sql, -1, &__stmt, NULL);
    sqlite3_free(sql);
    if(sqlret == SQLITE_OK) 
        return true;

__destroy_cache:
    sql = sqlite3_mprintf("DROP TABLE \"_FGLOB_STAT_%s\"", tableName.c_str());
    sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
    sqlite3_free(sql);
    if(errp)
        sqlite3_free(errp);

    sql = sqlite3_mprintf("DROP TABLE \"%s\"", tableName.c_str());
    sqlret = sqlite3_exec(__sqlite, sql, NULL, NULL, &errp);
    sqlite3_free(sql);
    if(errp)
        sqlite3_free(errp);
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
