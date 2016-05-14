#include <stdio.h>
#include <libconfig.h>
#include <sys/stat.h>
#include <shadow.h>
#include <pwd.h>
#define LDAP_DEPRECATED 1
#include <ldap.h>
#include <glib.h>
#include "RyftOne.h"
using namespace RyftOne;

#include "ryft1_catalog.h"

RyftOne_Database::RyftOne_Database(ILogger *log) : __authType( AUTH_NONE ), __log(log)
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
        else if(authType && !strcmp(authType, "system")) 
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
        if(!strcmp(epasswd, pw->sp_pwdp)) {
            return true;
        }
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
        int nu__entries_returned = ldap_count_entries(ld, msg);
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
        if(((in_type.find("TABLE") != string::npos) || in_type == "%") && __matches(in_search, itr->meta_config.table_name)) {
            table.m_tableName = itr->meta_config.table_name;
            table.m_description = itr->meta_config.table_remarks;
            table.m_type = "TABLE";
            tables.push_back(table);
        }

        if((in_type.find("VIEW") != string::npos) || in_type == "%") {
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
            RyftOne_Util::RyftToSqlType(col.m_typeName, &col.m_dataType, &col.m_charCols, &col.m_bufLength, col.m_formatSpec, &col.m_dtType);
            col.m_description = colItr->description;
            cols.push_back(col);
        }
    }
    return cols;
}

RyftOne_Result *RyftOne_Database::openTable(string& in_table)
{
    RyftOne_Result *result = new RyftOne_Result(__log);

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

// new tables will use an XML datatype
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
        config_setting_set_string(colElem, RyftOne_Util::SqlToRyftType(itr->m_dataType, itr->m_bufLength).c_str());
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
