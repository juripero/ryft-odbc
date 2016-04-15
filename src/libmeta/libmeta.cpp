#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include <libconfig.h>

#include "libmeta.h"

static char s_R1Catalog[] = "/ryftone/ODBC";
static char s_TableMeta[] = ".meta.table";

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