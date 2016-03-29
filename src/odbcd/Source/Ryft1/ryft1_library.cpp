#include "ryft1_library.h"
#include "TypeDefines.h"

#include <glob.h>
#include <dirent.h>
#include <stdio.h>
#include <libconfig.h>

static char s_R1Catalog[] = "/ryftone/ODBC";
static char s_R1Results[] = "/ryftone/ODBC/.results";
static char s_TableMeta[] = ".meta.table";

RyftOne_Result::RyftOne_Result(RyftOne_Database *ryft1) : m_ryft1(ryft1), m_hamming(0), m_edit(0), m_caseSensitive(true)
{
    ;
}

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

bool RyftOne_Result::open(string& in_name, vector<__catalog_entry__>::iterator in_catentry)
{
    m_name = in_name;

    __loadTable(in_catentry);

    vector<__meta_config__::__meta_view__>::iterator viewItr;
    for(viewItr = in_catentry->meta_config.views.begin(); viewItr != in_catentry->meta_config.views.end(); viewItr++) {
        if(in_name == viewItr->view_name) {
            m_query = viewItr->restriction;
            break;
        }
    }

    m_cols = m_ryft1->getColumns(in_name);
    m_queryFinished = false;
    return true;
}

bool RyftOne_Result::appendFilter(string in_filter, int in_hamming, int in_edit, bool in_caseSensitive)
{
    if(!m_query.empty())
        m_query += " AND ";

    m_query += in_filter;
    m_hamming = in_hamming;
    m_edit = in_edit;
    m_caseSensitive = in_caseSensitive;
    return true;
}

bool RyftOne_Result::appendRow()
{
    Row newRow;
    newRow.__type = typeInserted;
    m_rows.push_back(newRow);
}

bool RyftOne_Result::flush()
{
    time_t now = ::time(NULL);
    tm *lt = localtime(&now);
    char file_path[PATH_MAX];
    
    sprintf(file_path, "%s/%04d%02d%02d.%s", m_path.c_str(), 1900 + lt->tm_year, lt->tm_mon + 1, lt->tm_mday, m_extension.c_str());
    FILE *f = fopen(file_path, "a");
    if(f == NULL)
        return false;
    
    for(m_rowItr = m_rows.begin(); m_rowItr != m_rows.end(); m_rowItr++) {
        if((*m_rowItr).__type == typeInserted) {
            __writeRow(f, m_rowItr);
        }
    }
    fclose(f);
    return true;
}

bool RyftOne_Result::fetchFirst()
{
    if(!m_queryFinished)
        m_queryFinished = __execute();

    m_rowItr = m_rows.begin();
    return (m_rowItr < m_rows.end());
}

bool RyftOne_Result::fetchNext()
{
    m_rowItr++;
    return (m_rowItr < m_rows.end());
}

int RyftOne_Result::getColumnIndex(int col)
{
    int colIdx = -1;
    int i = m_rowItr->__row.size()-1;
    if(col < i)
        i = col;
    for( ; i >= 0; i--) {
        if(m_rowItr->__row[i].colNum == col) {
            colIdx = i;
            break;
        }
    }
    return colIdx;
}

int RyftOne_Result::getColumnNum(string colTag) 
{
    int idx;
    vector<string>::iterator itr;
    for(idx = 0, itr = m_rdfTags.begin(); itr != m_rdfTags.end(); itr++, idx++) {
        if((*itr) == colTag) 
            return idx;
    }
    return -1;
}

string& RyftOne_Result::getStringValue(int colIdx)
{
    return (*m_rowItr).__row[colIdx].colResult;
}

void RyftOne_Result::date(int colIdx, struct tm *date)
{
    switch(m_cols[colIdx].m_dtType) {
    case DATE_YYMMDD:
    case DATE_YYYYMMDD:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(date->tm_year), &(date->tm_mon), &(date->tm_mday));
        break;
    case DATE_DDMMYY:
    case DATE_DDMMYYYY:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(date->tm_mday), &(date->tm_mon), &(date->tm_year));
        break;
    case DATE_MMDDYY:
    case DATE_MMDDYYYY:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(date->tm_mon), &(date->tm_mday), &(date->tm_year));
        break;
    }
}

void RyftOne_Result::time(int colIdx, struct tm *time)
{
    char ampm[16];
    char *pampm;
    switch(m_cols[colIdx].m_dtType) {
    case TIME_12MMSS:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(time->tm_hour), &(time->tm_min), &(time->tm_sec), ampm);
        pampm = ampm;
        while (isspace(*pampm))
            pampm++;
        if(!strcasecmp(pampm, "pm"))
            time->tm_hour += 12;
        break;
    case TIME_24MMSS:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(time->tm_hour), &(time->tm_min), &(time->tm_sec));
        break;
    }
}

void RyftOne_Result::datetime(int colIdx, struct tm *datetime)
{
    char ampm[16];
    char *pampm;
    switch(m_cols[colIdx].m_dtType) {
    case DATETIME_YYYYMMDD_12MMSS:
    case DATETIME_YYMMDD_12MMSS:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_year), &(datetime->tm_mon), &(datetime->tm_mday),
            &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec), ampm);
        pampm = ampm;
        while (isspace(*pampm))
            pampm++;
        if(!strcasecmp(pampm, "pm"))
            datetime->tm_hour += 12;
        break;
    case DATETIME_DDMMYYYY_12MMSS:
    case DATETIME_DDMMYY_12MMSS:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_mday), &(datetime->tm_mon), &(datetime->tm_year),
            &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec), ampm);
        pampm = ampm;
        while (isspace(*pampm))
            pampm++;
        if(!strcasecmp(pampm, "pm"))
            datetime->tm_hour += 12;
        break;
    case DATETIME_MMDDYYYY_12MMSS:
    case DATETIME_MMDDYY_12MMSS:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_mon), &(datetime->tm_mday), &(datetime->tm_year),
            &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec), ampm);
        pampm = ampm;
        while (isspace(*pampm))
            pampm++;
        if(!strcasecmp(pampm, "pm"))
            datetime->tm_hour += 12;
        break;
    case DATETIME_YYYYMMDD_24MMSS:
    case DATETIME_YYMMDD_24MMSS:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_year), &(datetime->tm_mon), &(datetime->tm_mday),
            &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec));
        break;
    case DATETIME_DDMMYYYY_24MMSS:
    case DATETIME_DDMMYY_24MMSS:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_mday), &(datetime->tm_mon), &(datetime->tm_year),
            &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec));
        break;
    case DATETIME_MMDDYYYY_24MMSS:
    case DATETIME_MMDDYY_24MMSS:
        sscanf((*m_rowItr).__row[colIdx].colResult.c_str(), m_cols[colIdx].m_formatSpec.c_str(), &(datetime->tm_mon), &(datetime->tm_mday), &(datetime->tm_year),
            &(datetime->tm_hour), &(datetime->tm_min), &(datetime->tm_sec));
        break;
    }
}

bool RyftOne_Result::putStringValue(int colIdx, string colValue)
{
    resultCol result;
    result.colNum = colIdx;
    result.colResult = colValue;
    m_rows.back().__row.push_back(result);
}

//////////////////////////////////////////////////////////////
// Private

bool RyftOne_Result::__loadTable(vector<__catalog_entry__>::iterator in_itr)
{
    __rdf_config__ rdf_config(in_itr->meta_config.rdf_path);
    string path = in_itr->path;
    m_path = in_itr->path;

    path += "/";
    path += rdf_config.file_glob;
    const char *perr;

    size_t idx1 = rdf_config.file_glob.find(".");
    if(idx1 != string::npos) 
        m_extension = rdf_config.file_glob.substr(idx1+1);

    glob_t glob_results;
    glob(path.c_str(), GLOB_TILDE, NULL, &glob_results);
    m_loaded = rol_ds_create();
    char **relpaths = new char *[glob_results.gl_pathc];
    for(int i = 0; i < glob_results.gl_pathc; i++) {
        relpaths[i] = glob_results.gl_pathv[i] + strlen("/ryftone/");
        rol_ds_add_file(m_loaded, relpaths[i]);
        if(rol_ds_has_error_occurred(m_loaded)) {
            perr = rol_ds_get_error_string(m_loaded);
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
                m_rdfTags.push_back(rdfItr->start_tag.substr(idx1+1, idx2-idx1-1));
            }
        }
    }
    return true;
}

bool RyftOne_Result::__execute()
{
    int fd;
    struct stat sb;
    const char *perr;
    char results[PATH_MAX];

    mkdir(s_R1Results, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
    sprintf(results, "%s/results.%08x", s_R1Results, pthread_self());
    if(m_query.empty())
        m_query = "(RECORD CONTAINS ?)";
    string results_path(results);
    size_t idx = results_path.find("/", 1);
    rol_data_set_t result = rol_ds_search_fuzzy_hamming(m_loaded, results_path.substr(idx+1, string::npos).c_str(), 
        m_query.c_str(), 0, m_hamming, NULL, NULL, m_caseSensitive, NULL);
    if( rol_ds_has_error_occurred(result)) {
        perr = rol_ds_get_error_string(result);
        return false;
    }

    fd = ::open(results, O_RDONLY);
    fstat(fd, &sb);
    char *buffer = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    Parse(buffer, sb.st_size, m_delim);
    munmap(buffer, sb.st_size);
    close(fd);
    unlink(results);
    return true;
}

bool RyftOne_Result::__writeRow(FILE *f, vector<Row>::iterator in_itr)
{
    std::vector<resultCol>::iterator itr;
    fprintf(f, "<%s>", m_delim.c_str());
    for(itr = (*in_itr).__row.begin(); itr != (*in_itr).__row.end(); itr++) {
        fprintf(f, "<%s>%s</%s>", m_rdfTags[(*itr).colNum].c_str(), (*itr).colResult.c_str(), m_rdfTags[(*itr).colNum].c_str());
    }
    fprintf(f, "</%s>\n", m_delim.c_str());
}

IElement::NodeAction RyftOne_Result::AddRow()
{
    Row newRow;
    newRow.__type = typeNative;
    m_rows.push_back(newRow);
    return ProcessNode;
}

IElement::NodeAction RyftOne_Result::AddElement(std::string sName, const char **psAttribs, IElement **ppElement)
{
    int colNum = getColumnNum(sName);
    if(colNum != -1) {
        resultCol result;
        result.colNum = colNum;
        m_rows.back().__row.push_back(result);
    }
    return ProcessNode;
}

void RyftOne_Result::AddText(std::string sText) 
{
    if(!m_rows.empty() && !m_rows.back().__row.empty())
        m_rows.back().__row.back().colResult += sText;
}

//////////////////////////////////////////////////////////////
#include <shadow.h>
#include <pwd.h>
#define LDAP_DEPRECATED 1
#include <ldap.h>
#include <glib.h>
#include "RyftOne.h"
using namespace RyftOne;

RyftOne_Database::RyftOne_Database() : __authType( AUTH_NONE )
{
    GKeyFile *keyfile = g_key_file_new( );
    GKeyFileFlags flags;
    GError *error = NULL;
    gchar *authType = NULL;

    if(g_key_file_load_from_file(keyfile, SERVER_LINUX_BRANDING, flags, &error)) {
        authType = g_key_file_get_string(keyfile, "Auth", "Type", &error);
        if(authType && !strcmp(authType, "ldap")) {
            __authType = AUTH_LDAP;
            __ldapServer = g_key_file_get_string(keyfile, "Auth", "LDAPServer", &error);
            __ldapUser = g_key_file_get_string(keyfile, "Auth", "LDAPUser", &error);
            __ldapPassword = g_key_file_get_string(keyfile, "Auth", "LDAPPassword", &error);
            __ldapBaseDN = g_key_file_get_string(keyfile, "Auth", "LDAPBaseDN", &error);
            free(authType);
        }
        else if(!strcmp(authType, "system")) 
            __authType = AUTH_SYSTEM;
    }
    g_key_file_free(keyfile);

    __loadCatalog();
}

bool RyftOne_Database::getAuthRequired() 
{
    return (__authType != AUTH_NONE);
}

bool RyftOne_Database::logon(string& in_user, string& in_password)
{
    switch(__authType) {
    default:
    case AUTH_NONE:
        return true;
    case AUTH_SYSTEM: {
        spwd *pw = getspnam(in_user.c_str());
        if(pw == NULL)
            return false;
        if(pw->sp_pwdp == '\0')
            return true;
        char *epasswd = crypt(in_password.c_str(), pw->sp_pwdp);
        if(!strcmp(epasswd, pw->sp_pwdp))
            return true;
    
        return false;
        }
    case AUTH_LDAP: {
        LDAP *ld;
        LDAP *ld2;
        int  result;
        int  auth_method = LDAP_AUTH_SIMPLE;
        int  desired_version = LDAP_VERSION3;
        LDAPMessage *msg;
        LDAPMessage *entry;
        char *dn;
        char bind_dn[512];
            
        ldap_initialize(&ld, __ldapServer.c_str());
        ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &desired_version);

        sprintf(bind_dn, "cn=%s,%s", __ldapUser.c_str(), __ldapBaseDN.c_str());
        result = ldap_simple_bind_s(ld, bind_dn, __ldapPassword.c_str());
        if(result != LDAP_SUCCESS) {
            ldap_unbind(ld);
            return false;
        }

        sprintf(bind_dn, "(uid=%s)", in_user.c_str());
        result = ldap_search_s(ld, __ldapBaseDN.c_str(), LDAP_SCOPE_SUBTREE, bind_dn, NULL, 0, &msg);
        int num_entries_returned = ldap_count_entries(ld, msg);
        for( entry = ldap_first_entry(ld, msg); entry != NULL; entry = ldap_next_entry( ld, entry )) {
            dn = ldap_get_dn(ld, entry);
            ldap_initialize(&ld2, __ldapServer.c_str());
            ldap_set_option(ld2, LDAP_OPT_PROTOCOL_VERSION, &desired_version);
            result = ldap_simple_bind_s(ld2, dn, in_password.c_str());
            ldap_unbind(ld2);
            ldap_memfree(dn);
            if(!result) 
                break;
        }
        ldap_msgfree(msg);
        ldap_unbind(ld);
        return (result == LDAP_SUCCESS);
        }
    }
}

void RyftOne_Database::logoff()
{
    ;
}

bool RyftOne_Database::__matches(string& in_search, string& in_name)
{
    int idx1 = 0;
    int idx2;
    string term;
    int req_distance;
    bool match = true;
    const char *search = in_search.c_str();
    // empty search term matches all
    if(*search == '\0')
        return true;
    while(*search && match) {
        switch (*search) {
        case '%':
            term.clear();
            req_distance = 0;
            idx2 = 0;
            for(search++; *search; search++) {
                if(*search == '%' || *search == '_') { 
                    if(!term.empty()) {
                        break;
                    }
                    else if(*search == '_') {
                        req_distance++;
                    }
                }
                else
                    term += *search;
            }
            if(term.length()) {
                idx2 = in_name.find(term, idx1);
                if(idx2 == string::npos || (idx2 - idx1) < req_distance) {
                    match = false;
                }
                else
                    idx1 = idx2 + term.length();
            }
            else if(*search == '\0') {
                // if wildcard came at the end of the search term, consume remaining name
                idx1 = in_name.length();
            }
            break;
        case '_':
            if(idx1 >= in_name.length())
                match = false;
            search++;
            idx1++;
            break;
        default:
            if((idx1 >= in_name.length()) || *search != in_name[idx1])
                match = false;
            search++;
            idx1++;
            break;
        }
    }
    // did we consume the entire table name string
    if(idx1 < in_name.length())
        match = false;
    return match;
}

RyftOne_Tables RyftOne_Database::getTables(string in_search, string in_type)
{
    RyftOne_Table table;
    RyftOne_Tables tables;

    vector<__catalog_entry__>::iterator itr;
    for(itr = __catalog.begin(); itr != __catalog.end(); itr++) {
        if((in_type == "TABLE" || in_type == "%") && __matches(in_search, itr->meta_config.table_name)) {
            table.m_tableName = itr->meta_config.table_name;
            table.m_description = itr->meta_config.table_remarks;
            table.m_type = "TABLE";
            tables.push_back(table);
        }

        if(in_type == "VIEW" || in_type == "%") {
            vector<__meta_config__::__meta_view__>::iterator viewItr;
            for(viewItr = itr->meta_config.views.begin(); viewItr != itr->meta_config.views.end(); viewItr++) {
                if(__matches(in_search, viewItr->view_name)) {
                    table.m_tableName = viewItr->view_name;
                    table.m_description = viewItr->description;
                    table.m_type = "VIEW";
                    tables.push_back(table);
                }
            }
        }
    }
    return tables;
}

RyftOne_Columns RyftOne_Database::getColumns(string& in_table)
{
    RyftOne_Column col;
    RyftOne_Columns cols;
    int idx;

    vector<__catalog_entry__>::iterator itr = __findTable(in_table);
    if(itr != __catalog.end()) {
        vector<__meta_config__::__meta_col__>::iterator colItr;
        for(idx=0, colItr = itr->meta_config.columns.begin(); colItr != itr->meta_config.columns.end(); colItr++, idx++) {
            col.m_ordinal = idx+1;
            col.m_tableName = itr->meta_config.table_name;
            col.m_colTag = colItr->rdf_name;
            col.m_colName = colItr->name;
            col.m_typeName = colItr->type_def;
            RyftOne_Util::RyftToSqlType(col.m_typeName, &col.m_dataType, &col.m_bufferLen, col.m_formatSpec, &col.m_dtType);
            col.m_description = colItr->description;
            cols.push_back(col);
        }
    }
    return cols;
}

RyftOne_Result *RyftOne_Database::openTable(string& in_table)
{
    RyftOne_Result *result = new RyftOne_Result(this);

    vector<__catalog_entry__>::iterator itr = __findTable(in_table);
    if(itr != __catalog.end())
        result->open(in_table, itr);
    return result;
}

bool RyftOne_Database::tableExists(string& in_table)
{
    vector<__catalog_entry__>::iterator itr = __findTable(in_table);
    if(itr != __catalog.end())
        return true;
    return false;
}

void RyftOne_Database::createTable(string& in_table, RyftOne_Columns& in_columns)
{
    char path[PATH_MAX];
    char config_path[PATH_MAX];
    char rdf_path[PATH_MAX];
    char rdf_glob[PATH_MAX];

    strcpy(path, s_R1Catalog);
    strcat(path, "/");
    strcat(path, in_table.c_str());
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    strcpy(config_path, path);
    strcat(config_path, "/");
    strcat(config_path, s_TableMeta);

    sprintf(rdf_path, "%s/%s.rdf", path, in_table.c_str());
    sprintf(rdf_glob, "*.%s", in_table.c_str());

    config_t tableMeta;
    config_init(&tableMeta);

    config_setting_t *root = config_root_setting(&tableMeta);

    config_setting_t *table_name = config_setting_add(root, "table_name", CONFIG_TYPE_STRING);
    config_setting_set_string(table_name, in_table.c_str());
    config_setting_t *table_remarks = config_setting_add(root, "table_remarks", CONFIG_TYPE_STRING);
    config_setting_set_string(table_remarks, "");
    config_setting_t *table_rdf = config_setting_add(root, "rdf", CONFIG_TYPE_STRING);
    config_setting_set_string(table_rdf, rdf_path);

    RyftOne_Columns::iterator itr;
    config_setting_t *colListEntry;
    config_setting_t *colList = config_setting_add(root, "columns", CONFIG_TYPE_GROUP);
    for(itr = in_columns.begin(); itr != in_columns.end(); itr++) {
        colListEntry = config_setting_add(colList, itr->m_colName.c_str(), CONFIG_TYPE_LIST);
        config_setting_t *colElem = config_setting_add(colListEntry, "", CONFIG_TYPE_STRING);
        config_setting_set_string(colElem, itr->m_colName.c_str());
        colElem = config_setting_add(colListEntry, "", CONFIG_TYPE_STRING);
        config_setting_set_string(colElem, RyftOne_Util::SqlToRyftType(itr->m_dataType, itr->m_bufferLen).c_str());
        colElem = config_setting_add(colListEntry, "", CONFIG_TYPE_STRING);
        config_setting_set_string(colElem, "");
    }
    config_write_file(&tableMeta, config_path);
    config_destroy(&tableMeta);

    // create an rdf
    config_t tableRdf;
    config_init(&tableRdf);

    root = config_root_setting(&tableRdf);
    config_setting_t *rdf_name = config_setting_add(root, "rdf_name", CONFIG_TYPE_STRING);
    config_setting_set_string(rdf_name, in_table.c_str());
    config_setting_t *chunk_size = config_setting_add(root, "chunk_size_mb", CONFIG_TYPE_INT);
    config_setting_set_int(chunk_size, 1);
    config_setting_t *file_glob = config_setting_add(root, "file_glob", CONFIG_TYPE_STRING);
    config_setting_set_string(file_glob, rdf_glob);
    config_setting_t *record_start = config_setting_add(root, "record_start", CONFIG_TYPE_STRING);
    config_setting_set_string(record_start, "<record>");
    config_setting_t *record_end = config_setting_add(root, "record_end", CONFIG_TYPE_STRING);
    config_setting_set_string(record_end, "</record>");
    config_setting_t *data_type = config_setting_add(root, "data_type", CONFIG_TYPE_STRING);
    config_setting_set_string(data_type, "XML");

    char tagEntry[PATH_MAX];
    config_setting_t *tagListEntry;
    config_setting_t *tagList = config_setting_add(root, "fields", CONFIG_TYPE_GROUP);
    for(itr = in_columns.begin(); itr != in_columns.end(); itr++) {
        tagListEntry = config_setting_add(tagList, itr->m_colName.c_str(), CONFIG_TYPE_LIST);
        config_setting_t *tagElem = config_setting_add(tagListEntry, "", CONFIG_TYPE_STRING);
        sprintf(tagEntry, "<%s>", itr->m_colName.c_str());
        config_setting_set_string(tagElem, tagEntry);
        tagElem = config_setting_add(tagListEntry, "", CONFIG_TYPE_STRING);
        sprintf(tagEntry, "</%s>", itr->m_colName.c_str());
        config_setting_set_string(tagElem, tagEntry);
    }
    config_write_file(&tableRdf, rdf_path);
    config_destroy(&tableRdf);

    int pid = 0;
    int ret;
    char rhfsctl[PATH_MAX];
    sprintf(rhfsctl, "rhfsctl -a %s", rdf_path);
    ret = system(rhfsctl);
    ret = WEXITSTATUS(ret);
    __loadCatalog();
}

int RyftOne_Database::__remove_directory(const char *path)
{
   DIR *d = opendir(path);
   size_t path_len = strlen(path);
   int r = -1;

   if (d) {
      struct dirent *p;

      r = 0;
      while (!r && (p=readdir(d)))
      {
          int r2 = -1;
          char *buf;
          size_t len;

          /* Skip the names "." and ".." as we don't want to recurse on them. */
          if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
             continue;

          len = path_len + strlen(p->d_name) + 2; 
          buf = (char *)malloc(len);

          if (buf) {
             struct stat statbuf;

             snprintf(buf, len, "%s/%s", path, p->d_name);
             if (!stat(buf, &statbuf)) {
                if (S_ISDIR(statbuf.st_mode)) {
                   r2 = __remove_directory(buf);
                }
                else
                   r2 = unlink(buf);
             }
             free(buf);
          }
          r = r2;
      }
      closedir(d);
   }

   if (!r) {
      r = rmdir(path);
   }

   return r;
}

void RyftOne_Database::dropTable(string& in_table)
{
    char path[PATH_MAX];
    strcpy(path, s_R1Catalog);
    strcat(path, "/");
    strcat(path, in_table.c_str());
    __remove_directory(path);
    __loadCatalog();
}

//////////////////////////////////////////////////////////////

void RyftOne_Database::__loadCatalog()
{
    DIR *d;
    struct dirent *dir;
    __catalog.clear();
    if(d = opendir(s_R1Catalog)) {
        while((dir = readdir(d)) != NULL) {
            if((dir->d_type == DT_DIR) && (strcmp(dir->d_name, ".") != 0)) {
                __catalog_entry__ cat_ent(dir->d_name);
                if(cat_ent._is_valid())
                    __catalog.push_back(cat_ent);
            }
        }
        closedir(d);
    }
}

vector<__catalog_entry__>::iterator RyftOne_Database::__findTable(string& in_table)
{
    vector<__catalog_entry__>::iterator itr;
    vector<__meta_config__::__meta_view__>::iterator viewItr;
    for(itr = __catalog.begin(); itr != __catalog.end(); itr++) {
        if(!itr->meta_config.table_name.compare(in_table))
            return itr;
        for(viewItr = itr->meta_config.views.begin(); viewItr != itr->meta_config.views.end(); viewItr++) {
            if(!viewItr->view_name.compare(in_table))
                return itr;
        }
    }
    return __catalog.end();
}

//////////////////////////////////////////////////////////////

__meta_config__::__meta_config__(string& in_table)
{
    config_t tableMeta;
    config_setting_t *colList;
    config_setting_t *column;
    char path[PATH_MAX];
    __meta_col__ col;
    config_setting_t *view;
    config_setting_t *viewList;
    __meta_view__ v;
    const char *result;
    int idx;

    strcpy(path, s_R1Catalog);
    strcat(path, "/");
    strcat(path, in_table.c_str());
    strcat(path, "/");
    strcat(path, s_TableMeta);

    config_init(&tableMeta);
    if(CONFIG_TRUE == config_read_file(&tableMeta, path)) {
        if(CONFIG_TRUE == config_lookup_string(&tableMeta, "table_name", &result)) 
            table_name = in_table;
        if(CONFIG_TRUE == config_lookup_string(&tableMeta, "table_remarks", &result))
            table_remarks = result;
        if(CONFIG_TRUE == config_lookup_string(&tableMeta, "rdf", &result))
            rdf_path = result;

        colList = config_lookup(&tableMeta, "columns");
        for(idx=0; colList && (column = config_setting_get_elem(colList, idx)); idx++) {
            col.rdf_name = column->name;
            col.name = config_setting_get_string_elem(column, 0);
            col.type_def = config_setting_get_string_elem(column, 1);
            col.description = config_setting_get_string_elem(column, 2);
            columns.push_back(col);
        }

        viewList = config_lookup(&tableMeta, "views");
        for(idx=0; viewList && (view = config_setting_get_elem(viewList, idx)); idx++) {
            v.id = view->name;
            v.view_name = config_setting_get_string_elem(view, 0);
            v.restriction = config_setting_get_string_elem(view, 1);
            v.description = config_setting_get_string_elem(view, 2);
            views.push_back(v);
        }
    }
    config_destroy(&tableMeta);
}

__rdf_config__::__rdf_config__(string& in_path)
{
    config_t rdfConfig;
    config_setting_t *tagList;
    config_setting_t *tag;
    __rdf_tag__ rdftag;
    const char *result;
    int idx;

    config_init(&rdfConfig);
    if(CONFIG_TRUE == config_read_file(&rdfConfig, in_path.c_str())) {
        if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "file_glob", &result)) 
            file_glob = result;
        if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "record_start", &result))
            record_start = result;
        if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "record_end", &result))
            record_end = result;

        tagList = config_lookup(&rdfConfig, "fields");
        for(idx=0; tagList && (tag = config_setting_get_elem(tagList, idx)); idx++) {
            rdftag.name = tag->name;
            rdftag.start_tag = config_setting_get_string_elem(tag, 0);
            rdftag.end_tag = config_setting_get_string_elem(tag, 1);
            tags.push_back(rdftag);
        }
    }
    config_destroy(&rdfConfig);
}

__catalog_entry__::__catalog_entry__(string in_table) : meta_config(in_table), rdf_config(meta_config.rdf_path) 
{
    path = s_R1Catalog;
    path += "/";
    path += in_table;
}

bool __catalog_entry__::_is_valid()
{
    if(meta_config.table_name.empty())
        return false;
    if(rdf_config.file_glob.empty())
        return false;
    vector<__meta_config__::__meta_col__>::iterator colitr;
    vector<__rdf_config__::__rdf_tag__>::iterator tagitr;
    for(colitr = meta_config.columns.begin(); colitr != meta_config.columns.end(); colitr++) {
        for(tagitr = rdf_config.tags.begin(); tagitr != rdf_config.tags.end(); tagitr++) {
            if(!colitr->rdf_name.compare(tagitr->name))
                break;
        }
        if(tagitr == rdf_config.tags.end())
            return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////

void RyftOne_Util::RyftToSqlType(string& in_typeName, unsigned *out_sqlType, unsigned *out_bufferLen, string& out_format, unsigned *out_dtType)
{
    unsigned length = 0;
    char format[80];
    char formatSpec[80];
    if(!strcasecmp(in_typeName.c_str(), "integer")) {
        *out_sqlType = SQL_INTEGER;
        length = 4;
    }
    else if(!strcasecmp(in_typeName.c_str(), "smallint")) {
        *out_sqlType = SQL_SMALLINT;
        length = 2;
    }
    else if(!strcasecmp(in_typeName.c_str(), "bigint")) {
        *out_sqlType = SQL_BIGINT;
        length = 20;
    }
    else if(!strcasecmp(in_typeName.c_str(), "double")) {
        *out_sqlType = SQL_DOUBLE;
        length = 8;
    }
    else if(!strncasecmp(in_typeName.c_str(), "datetime", strlen("datetime"))) {
        *out_sqlType = SQL_TYPE_TIMESTAMP;
        const char *paren = strchr(in_typeName.c_str(), '(');
        strcpy(format, "MM-DD-YYYY-24:MM:SS");
        if(paren) {
            strcpy(format, paren+1);
            format[strlen(format)-1] = '\0';
        }
        if(format[0] == 'Y' && format[2] == 'Y') { // YYYY/MM/DD-HH:MM:SS
            if(format[11] == '1') {
                *out_dtType = DATETIME_YYYYMMDD_12MMSS;
                sprintf(formatSpec, "%%04d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d %%s", format[4], format[7], format[10], format[13], format[16]);
            }
            else {
                *out_dtType = DATETIME_YYYYMMDD_24MMSS;
                sprintf(formatSpec, "%%04d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d", format[4], format[7], format[10], format[13], format[16]);
            }
        }
        else if(format[0] == 'Y') { // YY/MM/DD-HH:MM:SS
            if(format[11] == '1') {
                *out_dtType = DATETIME_YYMMDD_12MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d %%s", format[2], format[5], format[8], format[11], format[14]);
            }
            else {
                *out_dtType = DATETIME_YYMMDD_24MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d", format[2], format[5], format[8], format[11], format[14]);
            }
        }
        else if (format[0] == 'D' && format[8] == 'Y') { // DD/MM/YYYY-HH:MM:SS
            if(format[11] == '1') {
                *out_dtType = DATETIME_DDMMYYYY_12MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%04d%c%%02d%c%%02d%c%%02d %%s", format[2], format[5], format[10], format[13], format[16]);
            }
            else {
                *out_dtType = DATETIME_DDMMYYYY_24MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d", format[2], format[5], format[10], format[13], format[16]);
            }
        }
        else if (format[0] == 'D') { // DD/MM/YY-HH:MM:SS
            if(format[11] == '1') {
                *out_dtType = DATETIME_DDMMYY_12MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d %%s", format[2], format[5], format[8], format[11], format[14]);
            }
            else {
                *out_dtType = DATETIME_DDMMYY_24MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d", format[2], format[5], format[8], format[11], format[14]);
            }
        }
        else if (format[0] == 'M' && format[8] == 'Y') { // MM/DD/YYYY-HH:MM:SS
            if(format[11] == '1') {
                *out_dtType = DATETIME_MMDDYYYY_12MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%04d%c%%02d%c%%02d%c%%02d %%s", format[2], format[5], format[10], format[13], format[16]);
            }
            else {
                *out_dtType = DATETIME_MMDDYYYY_24MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d", format[2], format[5], format[10], format[13], format[16]);
            }
        }
        else if (format[0] == 'M') { // MM/DD/YY-HH:MM:SS
            if(format[11] == '1') {
                *out_dtType = DATETIME_MMDDYY_12MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d %%s", format[2], format[5], format[8], format[11], format[14]);
            }
            else {
                *out_dtType = DATETIME_MMDDYY_24MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d", format[2], format[5], format[8], format[11], format[14]);
            }
        }
        out_format = formatSpec;
        length = 16;
    }
    else if(!strncasecmp(in_typeName.c_str(), "date", strlen("date"))) {
        *out_sqlType = SQL_TYPE_DATE;
        const char *paren = strchr(in_typeName.c_str(), '(');
        strcpy(format, "YYYY/MM/DD");
        if(paren) {
            sscanf(paren, "(%s)", format);
            format[strlen(format)-1] = '\0';
        }
        if(strlen(format) == 10) {
            if(format[0] == 'Y') { // YYYY/MM/DD
                *out_dtType = DATE_YYYYMMDD;
                sprintf(formatSpec, "%%04d%c%%02d%c%%02d", format[4], format[7]);
            }
            else if (format[0] == 'D') { // DD/MM/YYYY
                *out_dtType = DATE_DDMMYYYY;
                sprintf(formatSpec, "%%02d%c%%02d%c%%04d", format[2], format[5]);
            }
            else { // MM/DD/YYYY
                *out_dtType = DATE_MMDDYYYY;
                sprintf(formatSpec, "%%02d%c%%02d%c%%04d", format[2], format[5]);
            }
        }
        else if(strlen(format) == 8) {
            if(format[0] == 'Y') { // YY/MM/DD
                *out_dtType = DATE_YYMMDD;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d", format[2], format[5]);
            }
            else if(format[0] == 'D') { // DD/MM/YY
                *out_dtType = DATE_DDMMYY;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d", format[2], format[5]);
            }
            else { // MM/DD/YY
                *out_dtType = DATE_MMDDYY;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d", format[2], format[5]);
            }
        }
        out_format = formatSpec;
        length = 6;
    }
    else if(!strncasecmp(in_typeName.c_str(), "time", strlen("time"))) {
        *out_sqlType = SQL_TYPE_TIME;
        const char *paren = strchr(in_typeName.c_str(), '(');
        strcpy(format, "24:MM:SS");
        if(paren) {
            sscanf(paren, "(%s)", format);
            format[strlen(format)-1] = '\0';
        }
        if(!strcmp(format, "24:MM:SS")) {
            *out_dtType = TIME_24MMSS;
            strcpy(formatSpec, "%02d:%02d:02d");
        }
        else {
            *out_dtType = TIME_12MMSS;
            strcpy(formatSpec, "%02d:%02d:02d %s");
        }
        out_format = formatSpec;
        length = 6;
    }
    else if(!strncasecmp(in_typeName.c_str(), "varchar", strlen("varchar"))) {
        *out_sqlType = SQL_VARCHAR;
        const char * paren = strchr(in_typeName.c_str(), '(');
        if(paren)
            sscanf(paren, "(%d)", &length);
    }
    *out_bufferLen = length;
}

string RyftOne_Util::SqlToRyftType(unsigned in_type, unsigned in_bufLen)
{
    switch(in_type) {
    case SQL_INTEGER:
        return "INTEGER";
    case SQL_SMALLINT:
        return "SMALLINT";
    case SQL_BIGINT:
        return "BIGINT";
    case SQL_DOUBLE:
        return "DOUBLE";
    case SQL_DATE:
        return "DATE";
    case SQL_TIME:
        return "TIME";
    case SQL_TIMESTAMP:
        return "DATETIME";
    default: {
        char length[16];
        string out_typeName = "VARCHAR";
        snprintf(length, sizeof(length), "(%d)", in_bufLen);
        out_typeName += length;
        return out_typeName;
        }
    }
}