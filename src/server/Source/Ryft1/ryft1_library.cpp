#include "ryft1_library.h"
#include "TypeDefines.h"

#include <glob.h>
#include <dirent.h>
#include <stdio.h>
#include <libconfig.h>

static char s_R1Catalog[] = "/ryftone/ODBC";
static char s_R1Results[] = "/ryftone/ODBC/.results";
static char s_TableMeta[] = ".meta.table";

RyftOne_Result::RyftOne_Result(RyftOne_Database *ryft1) : m_ryft1(ryft1), m_hamming(0)
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

    m_queryFinished = false;
    return true;
}

bool RyftOne_Result::appendFilter(string in_filter, int in_hamming)
{
    if(!m_query.empty())
        m_query += " AND ";

    m_query += in_filter;
    if(in_hamming > m_hamming)
        m_hamming = in_hamming;
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
    time_t now = time(NULL);
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
        m_query.c_str(), 0, m_hamming, NULL, NULL, true, NULL);
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

RyftOne_Database::RyftOne_Database() 
{
    __loadCatalog();
}

#include <shadow.h>
#include <pwd.h>
bool RyftOne_Database::logon(string& in_user, string& in_password)
{
    struct passwd *pwd = getpwnam(in_user.c_str());

    spwd *pw = getspnam(in_user.c_str());

    if(pw == NULL)
        return false;

    if(pw->sp_pwdp == '\0')
        return true;

    char *epasswd = crypt(in_password.c_str(), pw->sp_pwdp);
    if(strcmp(epasswd, pw->sp_pwdp))
        return false;
    
    return true;
}

void RyftOne_Database::logoff()
{
    ;
}

RyftOne_Tables RyftOne_Database::getTables()
{
    RyftOne_Table table;
    RyftOne_Tables tables;

    vector<__catalog_entry__>::iterator itr;
    for(itr = __catalog.begin(); itr != __catalog.end(); itr++) {
        table.m_tableName = itr->meta_config.table_name;
        table.m_description = itr->meta_config.table_remarks;
        table.m_type = "TABLE";
        tables.push_back(table);
        vector<__meta_config__::__meta_view__>::iterator viewItr;
        for(viewItr = itr->meta_config.views.begin(); viewItr != itr->meta_config.views.end(); viewItr++) {
            table.m_tableName = viewItr->view_name;
            table.m_description = viewItr->description;
            table.m_type = "VIEW";
            tables.push_back(table);
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
            RyftOne_Util::RyftToSqlType(col.m_typeName, &col.m_dataType, &col.m_bufferLen);
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
    config_setting_t *field_type = config_setting_add(root, "field_type", CONFIG_TYPE_STRING);
    config_setting_set_string(field_type, "tagged");

    char tagEntry[PATH_MAX];
    config_setting_t *tagListEntry;
    config_setting_t *tagList = config_setting_add(root, "tags", CONFIG_TYPE_GROUP);
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

int remove_directory(const char *path)
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
                   r2 = remove_directory(buf);
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
    remove_directory(path);
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
                if(!cat_ent.meta_config.table_name.empty())
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

        tagList = config_lookup(&rdfConfig, "tags");
        for(idx=0; tag = config_setting_get_elem(tagList, idx); idx++) {
            rdftag.name = tag->name;
            rdftag.start_tag = config_setting_get_string_elem(tag, 0);
            rdftag.end_tag = config_setting_get_string_elem(tag, 1);
            tags.push_back(rdftag);
        }
    }
    config_destroy(&rdfConfig);
}

__catalog_entry__::__catalog_entry__(string in_table) : meta_config(in_table) 
{
    path = s_R1Catalog;
    path += "/";
    path += in_table;
}

//////////////////////////////////////////////////////////////

void RyftOne_Util::RyftToSqlType(string& in_typeName, unsigned *out_sqlType, unsigned *out_bufferLen)
{
    unsigned length = 0;
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
    else if(!strcasecmp(in_typeName.c_str(), "date")) {
        *out_sqlType = SQL_DATE;
        length = 6;
    }
    else if(!strcasecmp(in_typeName.c_str(), "time")) {
        *out_sqlType = SQL_TIME;
        length = 6;
    }
    else if(!strcasecmp(in_typeName.c_str(), "datetime")) {
        *out_sqlType = SQL_TIMESTAMP;
        length = 16;
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