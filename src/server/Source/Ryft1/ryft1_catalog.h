#pragma once

#include "RyftOne.h"

namespace Simba
{
namespace Support
{
    class ILogger;
}
}

#include <libryftone.h>

#include <string.h>
#include <string>
#include <vector>
using namespace std;

#include "libmeta.h"
#include "ryft1_util.h"
#include "ryft1_result.h"
#include "ryft1_catalog.h"

#define AUTH_NONE   0
#define AUTH_SYSTEM 1
#define AUTH_LDAP   2

typedef struct _RyftOne_Table {
    string m_tableName;
    string m_description;
    string m_type;
} RyftOne_Table;
typedef vector<RyftOne_Table> RyftOne_Tables;

class RyftOne_Database {
public:
    RyftOne_Database(ILogger *);

    bool getAuthRequired();
    bool logon(string& in_user, string& in_password);
    void logoff() { }

    RyftOne_Tables getTables(string in_search, string in_type);
    RyftOne_Columns getColumns(string& in_table);

    RyftOne_Result *openTable(string& in_table);
    bool tableExists(string& in_table);
    void createTable(string& in_table, RyftOne_Columns& in_columns);
    void dropTable(string& in_table);

private:
    bool __logonToREST();
    void __loadCatalog();
    bool __matches(string& in_search, string& in_name);
    int __remove_directory(const char *path);

    vector<__catalog_entry__>::iterator __findTable(string& in_table);

    vector<__catalog_entry__> __catalog;

    int __authType;
    string __ldapServer;
    string __ldapUser;
    string __ldapPassword;
    string __ldapBaseDN;

    string __restServer;
    string __restUser;
    string __restPass;
    string __restToken;
    string __restExpire;
    string __restPath;

    // Reference to the ILogger. (NOT OWN)
    ILogger* __log;
};




