#pragma once
#include <vector>
#include <string>
using namespace std;

class __meta_config__ {
public:
    __meta_config__(string& in_table);

    class __meta_col__ {
    public:
        string rdf_name;
        string name;
        string type_def;
        string description;
    };

    class __meta_view__ {
    public:
        string id;
        string view_name;
        string restriction;
        string description;
    };

    string table_name;
    string table_remarks;
    string rdf_path;
    vector<__meta_col__> columns;
    vector<__meta_view__> views;
};

class __rdf_config__ {
public:
    __rdf_config__(string& in_path);

    class __rdf_tag__ {
    public:
        string name;
        string start_tag;
        string end_tag;
    };

    string file_glob;
    string record_start;
    string record_end;
    vector<__rdf_tag__> tags;
};

class __catalog_entry__ {
public:
    __catalog_entry__(string in_table);
    __meta_config__ meta_config;
    __rdf_config__ rdf_config;
    string path;

    bool _is_valid( );
};

