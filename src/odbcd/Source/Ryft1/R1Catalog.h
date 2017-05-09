// =================================================================================================
///  @file R1Catalog.h
///
///  Implements the R1 catalog class
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================
#ifndef _R1CATALOG_H_
#define _R1CATALOG_H_

#include "RyftOne.h"

namespace Simba
{
namespace Support
{
    class ILogger;
}
}

#include <string.h>
#include <string>
#include <vector>
using namespace std;

#include "libmeta.h"
#include "R1Util.h"
#include "R1IQueryResult.h"
#include "R1JSONResult.h"
#include "R1XMLResult.h"
#include "R1RAWResult.h"
#include "R1Catalog.h"

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

    bool GetAuthRequired();
    bool Logon(string& in_user, string& in_password);
    void Logoff() { }

    RyftOne_Tables GetTables(string in_search, string in_type);
    RyftOne_Columns GetColumns(string& in_table);

    IQueryResult *OpenTable(string& in_table);

    bool TableExists(string& in_table);
    void CreateTable(string& in_table, RyftOne_Columns& in_columns);
    void DropTable(string& in_table);

private:
    bool __logonToREST();
    void __loadCatalog();
    bool __matches(string& in_search, string& in_name);
    vector<__catalog_entry__>::iterator __findTable(string& in_table);
    int __remove_directory(const char *path);

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
#endif



