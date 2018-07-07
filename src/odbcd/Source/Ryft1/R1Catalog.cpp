// =================================================================================================
///  @file R1Catalog.cpp
///
///  Implements the R1 catalog class
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================

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

#include "R1Catalog.h"

RyftOne_Database::RyftOne_Database(ILogger *log) : __authType( AUTH_NONE ), __log(log)
{
    static char encrypted_content[] = "ENCRYPTED_CONTENT";
    GKeyFile *keyfile = g_key_file_new( );
    GKeyFileFlags flags;
    GError *error = NULL;
    gchar *authType = NULL;

    if(g_key_file_load_from_file(keyfile, SERVER_LINUX_BRANDING, flags, &error)) {
        // auth
        authType = g_key_file_get_string(keyfile, "Auth", "Type", &error);
        if(authType && !strcmp(authType, "ldap")) {
            gchar *ldapString;
            __authType = AUTH_LDAP;
            ldapString = g_key_file_get_string(keyfile, "Auth", "LDAPServer", &error);
            if(ldapString) {
                __ldapServer = ldapString;
                free(ldapString);
            }
            if(error != NULL)
                g_clear_error(&error);

            ldapString = g_key_file_get_string(keyfile, "Auth", "LDAPUser", &error);
            if(ldapString) {
                __ldapUser = ldapString;
                free(ldapString);
            }
            if(error != NULL)
                g_clear_error(&error);

            ldapString = g_key_file_get_string(keyfile, "Auth", "LDAPPassword", &error);
            if(ldapString) {
                char *pstr = strstr(ldapString, encrypted_content);
                if(pstr) {
                    pstr += strlen(encrypted_content) + 1;
                    pstr[strlen(pstr)-1] = '\0';
                    char *decrypted = __decrypt(pstr);
                    __ldapPassword = decrypted;
                    free(decrypted);
                }
                else
                    __ldapPassword = ldapString;
                free(ldapString);
            }
            if(error != NULL)
                g_clear_error(&error);

            __ldapBaseDN = ldapString = g_key_file_get_string(keyfile, "Auth", "LDAPBaseDN", &error);
            if(ldapString) {
                __ldapBaseDN = ldapString;
                free(ldapString);
            }
            if(error != NULL)
                g_clear_error(&error);

        }
        else if(authType && !strcmp(authType, "system")) 
            __authType = AUTH_SYSTEM;

        if(authType)
            free(authType);

        // REST 
        gchar *restString;
        restString = g_key_file_get_string(keyfile, "REST", "RESTServer", &error);
        if(restString) {
            __restServer = restString;
            free(restString);
        }
        if(error != NULL)
            g_clear_error(&error);

        restString = g_key_file_get_string(keyfile, "REST", "RESTUser", &error);
        if(restString) {
            __restUser = restString;
            free(restString);
        }
        if(error != NULL)
            g_clear_error(&error);

        restString = g_key_file_get_string(keyfile, "REST", "RESTPass", &error);
        if(restString) {
            char *pstr = strstr(restString, encrypted_content);
            if(pstr) {
                pstr += strlen(encrypted_content) + 1;
                pstr[strlen(pstr)-1] = '\0';
                char *decrypted = __decrypt(pstr);
                __restPass = decrypted;
                free(decrypted);
            }
            else
                __restPass = restString;
            free(restString);
        }
        if(error != NULL)
            g_clear_error(&error);

        // ODBC root path
        __rootPath = s_R1Root;
        gchar *rootString;
        rootString = g_key_file_get_string(keyfile, "Server", "RootPath", &error);
        if(rootString) {
            __rootPath = rootString;
            free(rootString);
        }
        if(error != NULL)
            g_clear_error(&error);

        // Cache control
        __lruMaxDepth = LRU_NORMAL;
        gchar *cacheString;
        cacheString = g_key_file_get_string(keyfile, "Server", "UseCache", &error);
        if(cacheString && !strcmp(cacheString, "none")) {
            __lruMaxDepth = LRU_NONE;            
        }
        else if(cacheString && !strcmp(cacheString, "all")) {
            __lruMaxDepth = LRU_ALL;
        }
        if(cacheString) 
            free(cacheString);
        if(error != NULL)
            g_clear_error(&error);

        // GeoIP database path
        gchar *geoipString;
        geoipString = g_key_file_get_string(keyfile, "PCAP", "GeoIP", &error);
        if (geoipString) {
            __geoipPath = geoipString;
            free(geoipString);
        }
        if (error != NULL)
            g_clear_error(&error);

        // manuf file path
        gchar *manufString;
        manufString = g_key_file_get_string(keyfile, "PCAP", "manuf", &error);
        if (manufString) {
            __manufPath = manufString;
            free(manufString);
        }
        if (error != NULL)
            g_clear_error(&error);
    }
    if(error != NULL)
        g_clear_error(&error);

    g_key_file_free(keyfile);

    __loadCatalog();
}

bool RyftOne_Database::GetAuthRequired() 
{
    return (__authType != AUTH_NONE);
}

bool RyftOne_Database::Logon(string& in_user, string& in_password)
{
    switch(__authType) {
    default:
    case AUTH_NONE:
        break;
    case AUTH_SYSTEM: {
        spwd *pw = getspnam(in_user.c_str());
        if(pw == NULL)
            return false;
        if(pw->sp_pwdp == '\0')
            break;
        char *epasswd = crypt(in_password.c_str(), pw->sp_pwdp);
        if(!strcmp(epasswd, pw->sp_pwdp)) {
            break;
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
        if(result != LDAP_SUCCESS)
            return false;
        }
        break;
    }

    return __logonToREST();
}

RyftOne_Tables RyftOne_Database::GetTables(string in_search, string in_type)
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

RyftOne_Columns RyftOne_Database::GetColumns(string& in_table)
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
            if(itr->meta_config.data_type == dataType_XML) {
                // xml uses the RDF field name (assumed to equal the xml tag name in searches without an RDF)
                col.m_colAlias = colItr->meta_name;
            }
            else
                // JSON will use the fully qualified JSON path (e.g. parent.[].child) which is built during table load
                col.m_colAlias = colItr->json_or_xml_tag;
            col.m_colName = colItr->name;
            col.m_typeName = colItr->type_def;
            RyftOne_Util::RyftToSqlType(col.m_typeName, &col.m_dataType, &col.m_charCols, &col.m_bufLength, col.m_formatSpec, &col.m_dtType);
            col.m_description = colItr->description;
            cols.push_back(col);
        }
    }
    return cols;
}

IQueryResult *RyftOne_Database::OpenTable(string& in_table)
{
    IQueryResult * result = NULL;

    vector<__catalog_entry__>::iterator itr = __findTable(in_table);
    if(itr != __catalog.end()) {
        string auth;
        if(!__restUser.empty()) {
            string basic = __restUser + ":" + __restPass;
            gchar * basic64 = g_base64_encode((const guchar *)basic.c_str(), basic.length());
            auth = basic64;
            if(basic64)
                free(basic64);
        }
        switch(itr->meta_config.data_type) {
        case dataType_JSON:
            result = new RyftOne_JSONResult(__log);
            break;
        case dataType_XML:
            result = new RyftOne_XMLResult(__log);
            break;
        case dataType_CSV:
            result = new RyftOne_CSVResult(__log);
            break;
        case dataType_PCAP:
            result = new RyftOne_PCAPResult(__log, __geoipPath, __manufPath);
            break;
        default:
            result = new RyftOne_RAWResult(__log);
            break;
        }
        result->OpenQuery(in_table, itr, __restServer, auth, __restPath, __rootPath, __lruMaxDepth);
    }
    return result;
}

bool RyftOne_Database::TableExists(string& in_table)
{
    vector<__catalog_entry__>::iterator itr = __findTable(in_table);
    if(itr != __catalog.end())
        return true;
    return false;
}

// new tables will use an XML datatype
void RyftOne_Database::CreateTable(string& in_table, RyftOne_Columns& in_columns)
{
    char path[PATH_MAX];
    char config_path[PATH_MAX];
    char rdf_path[PATH_MAX];
    char rdf_glob[PATH_MAX];

    __odbcRoot(path);
    strcat(path, "/");
    // TODO: schema?
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

void RyftOne_Database::DropTable(string& in_table)
{
    char path[PATH_MAX];
    __odbcRoot(path);
    strcat(path, "/");
    strcat(path, in_table.c_str());
    __remove_directory(path);
    __loadCatalog();
}

//////////////////////////////////////////////////////////////
#include <json/json.h>
#include <curl/curl.h>
size_t token_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    string *json_token = (string *)userdata;
    (*json_token).append(ptr, size * nmemb);
    return size * nmemb;
}

bool RyftOne_Database::__logonToREST()
{
    __restPath = __rootPath;

    // if no user specified run without authenticating
    if(__restUser.empty())
        return true;

    // login to the REST server
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();

    string url = __restServer + "/login";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    string post = "{\"username\":\"" + __restUser + "\",\"password\":\"" + __restPass + "\"}";
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());

    __restToken.clear();
    __restExpire.clear();

    // WORKWORK VERIFY WITH GOOD CERT
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    string json_token;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, token_write_callback); 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_token);
    CURLcode code = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    if(code != CURLE_OK)
        return false;

    struct json_object *login_token = json_tokener_parse(json_token.c_str());
    if(login_token) {
        struct json_object *token = json_object_object_get(login_token, "token");
        if(token)
            __restToken = json_object_get_string(token);
        struct json_object *expire = json_object_object_get(login_token, "expire");
        if(expire)
            __restExpire = json_object_get_string(expire);
    }

    // WORKWORK MAKING A BAD ASSUMPTION THAT ANYTIME WE ARE LOGGED IN THAT THE PATH IS /RYFTONE/ODBC
    if(!__restToken.empty())
        __restPath += s_R1Catalog;

    return !__restToken.empty();
}

void RyftOne_Database::__loadCatalog()
{
    DIR *d;
    struct dirent *dir;
    __catalog.clear();

    char odbcRoot[PATH_MAX];
    __odbcRoot(odbcRoot);
    if(d = opendir(odbcRoot)) {
        while((dir = readdir(d)) != NULL) {

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, ".."))
                continue;

            char path[PATH_MAX];
            struct stat statbuf;
            snprintf(path, PATH_MAX, "%s/%s", odbcRoot, dir->d_name);

            if(dir->d_type == DT_UNKNOWN) {
                if (!stat(path, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode)) 
                        dir->d_type = DT_DIR;
                }
            }
            if(dir->d_type == DT_DIR) {
                __catalog_entry__ cat_ent(path);
                if(cat_ent._is_valid())
                    __catalog.push_back(cat_ent);
            }
        }
        closedir(d);
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

int RyftOne_Database::__remove_directory(const char *path)
{
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;

        r = 0;
        while (!r && (p=readdir(d))) {
            int r2 = -1;
            char file_path[PATH_MAX];

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            snprintf(file_path, PATH_MAX, "%s/%s", path, p->d_name);
            struct stat statbuf;
            if (!stat(file_path, &statbuf)) {
                if (S_ISDIR(statbuf.st_mode)) {
                    r2 = __remove_directory(file_path);
                }
                else
                    r2 = unlink(file_path);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r) 
        r = rmdir(path);

    return r;
}