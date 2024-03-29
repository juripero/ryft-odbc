// =================================================================================================
///  @file libmeta.cpp
///
///  Handles meta functions for ODBC
///
///  Copyright (C) 2017 Ryft Systems, Inc.
// =================================================================================================
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "libmeta.h"

const char s_R1Root[] = "/ryftone";
const char s_R1Catalog[] = "/ODBC";
const char s_R1Results[] = "/.results";
const char s_R1Unload[] = "/unload";
const char s_R1Caches[] = "/.caches";
const char s_TableMeta[] = ".meta.table";
const char s_RyftUser[] = "ryftuser";

inline string to_name(const char *name)
{
    string __name = name;
    size_t pos = 0;
    while((pos = __name.find('*', pos)) != string::npos) {
        char c;
        c  = ((__name[pos+1] >= 'A') ? (__name[pos+1] - 'A' + 10) : (__name[pos+1] - '0') << 4);
        c |=  (__name[pos+2] >= 'A') ? (__name[pos+2] - 'A' + 10) : (__name[pos+2] - '0');
        __name.replace(pos, 3, 1, c);
    }
    return __name;
}

__meta_config__::__meta_config__() : data_type(dataType_None) { }
__meta_config__::__meta_config__(string& in_dir) : data_type(dataType_None)
{
    config_t tableMeta;
    char path[PATH_MAX];
    config_setting_t *view;
    config_setting_t *viewList;
    config_setting_t *filter;
    config_setting_t *filterList;
    __meta_view__ v;
    __meta_filter__ f;
    const char *result;
    int value;
    int idx;
    vector<__meta_config__::__meta_col__>::iterator colItr;

    strcpy(path, in_dir.c_str());
    strcat(path, "/");
    strcat(path, s_TableMeta);

    int fd = open(path, O_RDONLY);
    if(fd != -1) {
        fstat(fd, &metafile_stat);
        close(fd);
    }

    config_init(&tableMeta);
    if(CONFIG_TRUE == config_read_file(&tableMeta, path)) {

        // common to both versions
        if(CONFIG_TRUE == config_lookup_string(&tableMeta, "table_name", &result)) 
            table_name = result;
        if(CONFIG_TRUE == config_lookup_string(&tableMeta, "table_remarks", &result))
            table_remarks = result;
        delimiter = "|";
        if(CONFIG_TRUE == config_lookup_string(&tableMeta, "delimiter", &result))
            delimiter = result;

        version = 1;
        if(CONFIG_TRUE == config_lookup_int(&tableMeta, "version", &value))
            version = value;

        string name, jsonroot;
        column_meta(tableMeta, "columns", name, jsonroot);

        switch(version) {
        default:
        case 1: {
            if(CONFIG_TRUE == config_lookup_string(&tableMeta, "rdf", &result)) 
                rdf_path = result;

            // load and copy required parameters from the RDF
            __rdf_config__ rdf_config(rdf_path);
            data_type = rdf_config.data_type;
            file_glob = rdf_config.file_glob;
            record_tag = rdf_config.record_start;
            no_top = rdf_config.no_top;

            // map xml tags to columns
            size_t idx1, idx2;
            vector<__meta_config__::__meta_col__>::iterator colItr;
            vector<__rdf_config__::__rdf_tag__>::iterator rdfItr;
            for(colItr = columns.begin(); colItr != columns.end(); colItr++) {
                for(rdfItr = rdf_config.tags.begin(); rdfItr != rdf_config.tags.end(); rdfItr++) {
                    if(rdfItr->name == colItr->json_or_xml_tag) {
                        idx1 = rdfItr->start_tag.find("<");
                        idx2 = rdfItr->start_tag.find(">");
                        colItr->json_or_xml_tag = rdfItr->start_tag.substr(idx1+1, idx2-idx1-1);
                    }
                }
            }
            break;
        }
        case 2:
            if(CONFIG_TRUE == config_lookup_string(&tableMeta, "data_type", &result)) {
                if(!strcasecmp(result, "JSON")) {
                    data_type = dataType_JSON;
                }
                else if(!strcasecmp(result, "XML")) {
                    data_type = dataType_XML;
                }
                else if(!strcasecmp(result, "CSV")) {
                    data_type = dataType_CSV;
                }
                else if (!strcasecmp(result, "PCAP")) {
                    data_type = dataType_PCAP;
                }
            }
            if(CONFIG_TRUE == config_lookup_string(&tableMeta, "file_glob", &result)) 
                file_glob = result;

            no_top = false;
            switch(data_type) {
            case dataType_XML:
                if(CONFIG_TRUE == config_lookup_string(&tableMeta, "record_tag", &result))
                    record_tag = result;
                break;
            case dataType_JSON:
                if(CONFIG_TRUE == config_lookup_string(&tableMeta, "record_path", &result)) {
                    if(!strcmp(result, "."))
                        no_top = true;
                }
                break;
            case dataType_CSV:
                if(CONFIG_TRUE == config_lookup_string(&tableMeta, "record_delimiter", &result))
                    record_delimiter = result;
                if(CONFIG_TRUE == config_lookup_string(&tableMeta, "field_delimiter", &result))
                    field_delimiter = result;
                break;
            case dataType_PCAP:
                // map pcap values to column names
                for (colItr = columns.begin(); colItr != columns.end(); colItr++) {
                    colItr->json_or_xml_tag = colItr->description;
                }
                break;
            }
            // Add PIP configuration to meta table
            if(CONFIG_TRUE == config_lookup_string(&tableMeta, "pip_format", &result)) {
                pip_format = result;
            }
            else {
                pip_format = "None";
            }
        }
        
        viewList = config_lookup(&tableMeta, "views");
        for(idx=0; viewList && (view = config_setting_get_elem(viewList, idx)); idx++) {
            v.id = view->name;
            v.view_name = config_setting_get_string_elem(view, 0);
            v.restriction = config_setting_get_string_elem(view, 1);
            v.description = config_setting_get_string_elem(view, 2);
            views.push_back(v);
        }

        filterList = config_lookup(&tableMeta, "filters");
        for (idx = 0; filterList && (filter = config_setting_get_elem(filterList, idx)); idx++) {
            f.id = filter->name;
            f.filter_name = config_setting_get_string_elem(filter, 0);
            f.eq = config_setting_get_string_elem(filter, 1);
            f.ne = config_setting_get_string_elem(filter, 2);
            filters.push_back(f);
        }
    }
    config_destroy(&tableMeta);
}

void __meta_config__::column_meta(config_t in_table_meta, string in_group, string in_name, string in_jsonroot)
{
    config_setting_t *colList;
    config_setting_t *column;
    __meta_col__ col;
    size_t idx1, idx2;
    int idx;

    if(!in_name.empty())
        in_name += ".";
    if(!in_jsonroot.empty())
        in_jsonroot += ".";

    colList = config_lookup(&in_table_meta, in_group.c_str());
    for(idx = 0; colList && (column = config_setting_get_elem(colList, idx)); idx++) {
        string name = to_name(column->name);
        col.name = in_name + config_setting_get_string_elem(column, 0);
        col.type_def = config_setting_get_string_elem(column, 1);
        col.description = config_setting_get_string_elem(column, 2);
        col.meta_name = name;
        // in version 1, the xml tag will be mapped from the rdf, in version 2 we assume
        // the xml tag is equal to the meta_name 
        col.json_or_xml_tag = in_jsonroot + name;

        if(!strncasecmp(col.type_def.c_str(), "list", strlen("list"))) 
            col.json_or_xml_tag += ".[]";
        if(!strncasecmp(col.type_def.c_str(), "arrayof", strlen("arrayof"))) {
            char *arrayof = new char[col.type_def.length()+1];
            const char *paren = strchr(col.type_def.c_str(), '(');
            arrayof[0] = '\0';
            if(paren) {
                sscanf(paren, "(%s)", arrayof);
                arrayof[strlen(arrayof)-1] = '\0';
            }
            if(*arrayof)
                column_meta(in_table_meta, arrayof, col.name, col.json_or_xml_tag + string(".[]"));
        }
        else if(!strncasecmp(col.type_def.c_str(), "groupof", strlen("groupof"))) {
            char *groupof = new char[col.type_def.length()+1];
            const char *paren = strchr(col.type_def.c_str(), '(');
            groupof[0] = '\0';
            if(paren) {
                sscanf(paren, "(%s)", groupof);
                groupof[strlen(groupof)-1] = '\0';
            }
            if(*groupof)
                column_meta(in_table_meta, groupof, col.name, col.json_or_xml_tag);
        }
        else
            columns.push_back(col);
    }
}

void __meta_config__::write_meta_config(string path)
{
    config_t tableMeta;
    config_init(&tableMeta);

    char config_path[PATH_MAX];
    strcpy(config_path, path.c_str());
    strcat(config_path, "/");
    strcat(config_path, s_TableMeta);

    config_setting_t *root = config_root_setting(&tableMeta);

    config_setting_t *cs_table_name = config_setting_add(root, "table_name", CONFIG_TYPE_STRING);
    config_setting_set_string(cs_table_name, table_name.c_str());
    config_setting_t *cs_table_remarks = config_setting_add(root, "table_remarks", CONFIG_TYPE_STRING);
    config_setting_set_string(cs_table_remarks, table_remarks.c_str());
    config_setting_t *cs_table_rdf = config_setting_add(root, "rdf", CONFIG_TYPE_STRING);
    config_setting_set_string(cs_table_rdf, rdf_path.c_str());
    config_setting_t *cs_pip_format = config_setting_add(root, "pip_format", CONFIG_TYPE_STRING);
    config_setting_set_string(cs_pip_format, pip_format.c_str());

    config_setting_t *colListEntry;
    config_setting_t *colList = config_setting_add(root, "columns", CONFIG_TYPE_GROUP);
    vector<__meta_col__>::iterator itr;
    for(itr = columns.begin(); itr != columns.end(); itr++) {
        colListEntry = config_setting_add(colList, itr->json_or_xml_tag.c_str(), CONFIG_TYPE_LIST);
        config_setting_t *colElem = config_setting_add(colListEntry, "", CONFIG_TYPE_STRING);
        config_setting_set_string(colElem, itr->name.c_str());
        colElem = config_setting_add(colListEntry, "", CONFIG_TYPE_STRING);
        config_setting_set_string(colElem, itr->type_def.c_str());
        colElem = config_setting_add(colListEntry, "", CONFIG_TYPE_STRING);
        config_setting_set_string(colElem, itr->description.c_str());
    }
    config_write_file(&tableMeta, config_path);
    struct passwd *pwd = getpwnam(s_RyftUser);
    if(pwd != NULL)
        chown(config_path, pwd->pw_uid, pwd->pw_gid);

    config_destroy(&tableMeta);
}

__rdf_config__::__rdf_config__() : data_type(dataType_None) { }
__rdf_config__::__rdf_config__(string& in_path) : data_type(dataType_None)
{
    config_t rdfConfig;
    config_setting_t *tagList;
    config_setting_t *tag;
    __rdf_tag__ rdftag;
    const char *result;
    int idx;

    config_init(&rdfConfig);
    if(CONFIG_TRUE == config_read_file(&rdfConfig, in_path.c_str())) {
        if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "data_type", &result)) {
            if(!strcasecmp(result, "JSON")) {
                data_type = dataType_JSON;
            }
            else if(!strcasecmp(result, "XML")) {
                data_type = dataType_XML;
            }
            else if(!strcasecmp(result, "CSV")) {
                data_type = dataType_CSV;
            }
        }
        if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "file_glob", &result)) 
            file_glob = result;
        switch(data_type) {
        case dataType_XML:
            no_top = false;
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
            break;
        case dataType_JSON:
            no_top = false;
            if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "record_path", &result)) {
                if(!strcmp(result, "."))
                    no_top = true;
            }
            break;
        case dataType_CSV:
            if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "record_delimiter", &result))
                record_delimiter = result;
            if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "field_delimiter", &result))
                field_delimiter = result;
 
        }
    }
    config_destroy(&rdfConfig);
}

void __rdf_config__::write_rdf_config(string path)
{
    config_t tableRdf;
    config_init(&tableRdf);

    config_setting_t *root = config_root_setting(&tableRdf);
    char tagEntry[PATH_MAX];
    config_setting_t *rdf_name_cs = config_setting_add(root, "rdf_name", CONFIG_TYPE_STRING);
    config_setting_set_string(rdf_name_cs, rdf_name.c_str());
    config_setting_t *chunk_size_cs = config_setting_add(root, "chunk_size_mb", CONFIG_TYPE_INT);
    config_setting_set_int(chunk_size_cs, chunk_size);
    config_setting_t *file_glob_cs = config_setting_add(root, "file_glob", CONFIG_TYPE_STRING);
    config_setting_set_string(file_glob_cs, file_glob.c_str());
    config_setting_t *data_type_cs = config_setting_add(root, "data_type", CONFIG_TYPE_STRING);
    if(data_type == dataType_XML) {
        config_setting_set_string(data_type_cs, "XML");
        config_setting_t *record_start_cs = config_setting_add(root, "record_start", CONFIG_TYPE_STRING);
        config_setting_set_string(record_start_cs, record_start.c_str());
        config_setting_t *record_end_cs = config_setting_add(root, "record_end", CONFIG_TYPE_STRING);
        config_setting_set_string(record_end_cs, record_end.c_str());
        vector<__rdf_tag__>::iterator itr;
        config_setting_t *tagListEntry;
        config_setting_t *tagList = config_setting_add(root, "fields", CONFIG_TYPE_GROUP);
        for(itr = tags.begin(); itr != tags.end(); itr++) {
            tagListEntry = config_setting_add(tagList, itr->name.c_str(), CONFIG_TYPE_LIST);
            config_setting_t *tagElem = config_setting_add(tagListEntry, "", CONFIG_TYPE_STRING);
            config_setting_set_string(tagElem, itr->start_tag.c_str());
            tagElem = config_setting_add(tagListEntry, "", CONFIG_TYPE_STRING);
            config_setting_set_string(tagElem, itr->end_tag.c_str());
        }
    }
    else if (data_type == dataType_JSON) {
        config_setting_set_string(data_type_cs, "JSON");
        config_setting_t *record_path_cs = config_setting_add(root, "record_path", CONFIG_TYPE_STRING);
        if(no_top) {
            config_setting_set_string(record_path_cs, ".");
        }
        else
            config_setting_set_string(record_path_cs, "[]");
    }
    else if (data_type == dataType_CSV) {
        config_setting_set_string(data_type_cs, "CSV");
        config_setting_t *record_delimiter_cs = config_setting_add(root, "record_delimiter", CONFIG_TYPE_STRING);
        config_setting_set_string(record_delimiter_cs, record_delimiter.c_str());
        config_setting_t *field_delimiter_cs = config_setting_add(root, "field_delimiter", CONFIG_TYPE_STRING);
        config_setting_set_string(field_delimiter_cs, field_delimiter.c_str());
    }

    config_write_file(&tableRdf, path.c_str());
    struct passwd *pwd = getpwnam(s_RyftUser);
    if(pwd != NULL)
        chown(path.c_str(), pwd->pw_uid, pwd->pw_gid);
    config_destroy(&tableRdf);
}

__catalog_entry__::__catalog_entry__(string in_dir) : meta_config(in_dir) 
{
    __path = in_dir;
}

bool __catalog_entry__::_is_valid()
{
    if(meta_config.table_name.empty() || meta_config.file_glob.empty())
        return false;

    return true;
}

// XMLFile

NodeAction XMLFile::StartRow( )
{
    __field.clear();
    startRecord();
    return ProcessNode;
}

NodeAction XMLFile::AddElement( std::string sName, const char **psAttribs, XMLElement **ppElement )
{
    if(!__in_row)
        return ProcessNode;

    if(!__field.empty()) {
        outputField(__field, __value);
    }
    __field = sName;
    __value.clear();
    return ProcessNode;
}

void XMLFile::AddText( std::string sText )
{
    __value += sText;
}

void XMLFile::ExitRow()
{
    if(!__field.empty()) {
        outputField(__field, __value);
    }
    endRecord();
}

bool XMLFile::prolog( ) 
{ 
    if(__ffile) {
        fputs("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n", __ffile);
        fputs("<xml_root>\n", __ffile);
        return true; 
    }
    return false;
}

bool XMLFile::epilog( ) 
{
    if(__ffile) {
        fputs("</xml_root>\n", __ffile);
        return true;
    }
    return false; 
}

bool XMLFile::startRecord( ) 
{ 
    if(__ffile) {
        fprintf(__ffile, "<%s>", __delim.c_str());
        __in_row = true;
        return true; 
    }
    return false;
}

bool XMLFile::endRecord( ) 
{ 
    if(__ffile) {
        fprintf(__ffile, "</%s>\n", __delim.c_str());
        __in_row = false;
        return true; 
    }
    return false;
}

bool XMLFile::outputField( string field, string value ) 
{ 
    if(__ffile) {
        fprintf(__ffile, "<%s>%s</%s>", field.c_str(), value.c_str(), field.c_str());
        return true; 
    }
    return false;
}

bool XMLFile::copyFile( char *src_path ) 
{ 
    int ffile;
    struct stat sb;
    if((ffile = open(src_path, O_RDONLY)) != -1) {
        fstat(ffile, &sb);
        __in_row = false;
        XMLParse(ffile, sb.st_size, __delim, false);
        close(ffile);
        return true;
    }
    return false;    
}

// JSONFile
inline vector<string> dot_tokenize( string str ) 
{
    vector<string> strings;
    char *full_path = new char[str.length()+1];
    strcpy(full_path, str.c_str());
    char *segment = strtok(full_path, ".");
    for( ; segment ; ) {
        strings.push_back(segment);
        segment = strtok(NULL, ".");
    }
    delete full_path;
    return strings;
}

NodeAction JSONFile::JSONStartRow( )
{
    __field.clear();
    startRecord();
    return ProcessNode;
}

NodeAction JSONFile::JSONStartGroup( std::string sName )
{
    __qualifiedCol.push_back(sName);
    return ProcessNode;
}

NodeAction JSONFile::JSONStartArray( std::string sName )
{
    __qualifiedCol.push_back(sName + ".[]");
    return ProcessNode;
}

NodeAction JSONFile::JSONAddElement( std::string sName, const char **psAttribs, JSONElement **ppElement )
{
    if(!__field.empty()) {
        outputField(__field, __value);
    }
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
    __field = qualifiedName;
    __value.clear();
    return ProcessNode;
}

void JSONFile::JSONAddText( std::string sText )
{
    __value += sText;
}

void JSONFile::JSONExitGroup()
{
    __qualifiedCol.pop_back();
}

void JSONFile::JSONExitArray()
{
    __qualifiedCol.pop_back();
}

void JSONFile::JSONExitRow()
{
    if(!__field.empty()) {
        outputField(__field, __value);
    }
    endRecord();
}

bool JSONFile::prolog( ) 
{ 
    if(__ffile && !__no_top) {
        fputs("[\n", __ffile);
        return true; 
    }
    return false;
}

bool JSONFile::epilog( ) 
{
    if(__ffile) {
        if(!__no_top) {
            fputs("\n]", __ffile);
        }
        fputs("\n", __ffile);
        return true;
    }
    return false; 
}

bool JSONFile::startRecord( ) 
{ 
    __num_fields = 0;
    __predecessor.clear();
    if(__ffile) {
        if(__num_rows++) {
            if(!__no_top) {
                fputs(",\n", __ffile);
            }
            else
                fputs("\n", __ffile);
        }
        fputs("{\n", __ffile);
        return true; 
    }
    return false;
}

bool JSONFile::endRecord( ) 
{ 
    if(__ffile) {
        int idx;
        vector<string> predecessor_strings = dot_tokenize(__predecessor);
        for( idx = predecessor_strings.size(); idx > 1; idx-- ) {
            if(predecessor_strings[idx-1] == "[]") {
                fputs("]", __ffile);
            }
            else
                fputs("}", __ffile);
        }
        fputs("\n}", __ffile);
        return true; 
    }
    return false;
}

bool JSONFile::outputField( string field, string value ) 
{ 
    if(__ffile) {
        int match, idx;
        vector<string> field_strings = dot_tokenize(field);
        vector<string> predecessor_strings = dot_tokenize(__predecessor);
        for( match = 0; match < predecessor_strings.size() && match < field_strings.size(); match++) {
            if( predecessor_strings[match] != field_strings[match])
                break;
        }
        for( idx = predecessor_strings.size(); idx > match + 1; idx-- ) {
            if(predecessor_strings[idx-1] == "[]") {
                fputs("]", __ffile);
            }
            else
                fputs("}", __ffile);
        }
        if(__num_fields++) {
            fputs(",\n", __ffile);
        }
        for( idx = match + 1; idx < field_strings.size(); idx++ ) {
            if(field_strings[idx] == "[]") {
                fprintf(__ffile, " \"%s\" : [{", field_strings[idx-1].c_str());
                idx++;
            }
            else
                fprintf(__ffile, " \"%s\" : {", field_strings[idx-1].c_str());
        }
        fprintf(__ffile, " \"%s\" : \"%s\"", field_strings.back().c_str(), value.c_str());
        __predecessor = field;
        return true; 
    }
    return false;
}

bool JSONFile::copyFile( char *src_path ) 
{ 
    int ffile;
    struct stat sb;
    if((ffile = open(src_path, O_RDONLY)) != -1) {
        fstat(ffile, &sb);
        JSONParse(ffile, sb.st_size, __no_top, "");
        close(ffile);
        return true;
    }
    return false;    
}

// CSVFile

NodeAction CSVFile::CSVStartRow()
{
    startRecord();
    return ProcessNode;
}

NodeAction CSVFile::CSVAddElement(int valueIndex)
{
    __value.clear();
    return ProcessNode;
}

void CSVFile::CSVAddText( std::string sText )
{
    __value += sText;
}

void CSVFile::CSVExitElement()
{
    outputField("", __value);
}

void CSVFile::CSVExitRow()
{
    endRecord();
}

bool CSVFile::prolog( ) 
{ 
    // CSV Header?
    return true;
}

bool CSVFile::epilog( ) 
{
    return true;
}

bool CSVFile::startRecord( ) 
{ 
    __output = false;
    return true; 
}

bool CSVFile::endRecord( ) 
{ 
    if(__ffile) {
        fputs(__record_delimiter.c_str(), __ffile);
        return true; 
    }
    return false;
}

bool CSVFile::outputField( string field, string value ) 
{ 
    if(__ffile) {
        if(__output)
            fputs(__field_delimiter.c_str(), __ffile);
        if(value.find_first_of("\"\n,") != string::npos) {
            // output must be escaped
            size_t pos = value.find_first_of("\"");
            while(pos != string::npos) {
                // escape double quotes
                value.insert(pos++, "\"");
                pos = value.find_first_of("\"", ++pos);
            }
            // escape whole value
            value.insert(0, "\"");
            value.append("\"");
        }
        fputs(value.c_str(), __ffile);
        __output = true;
        return true; 
    }
    return false;
}

bool CSVFile::copyFile( char *src_path ) 
{ 
    int ffile;
    struct stat sb;
    if((ffile = open(src_path, O_RDONLY)) != -1) {
        fstat(ffile, &sb);
        CSVParse(ffile, sb.st_size, __field_delimiter, __record_delimiter);
        close(ffile);
        return true;
    }
    return false;    
}
