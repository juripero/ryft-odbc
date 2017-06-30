// =================================================================================================
///  @file R1JSONResult.cpp
///
///  Implements the result class for XML data searches
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================

#include "R1JSONResult.h"

NodeAction RyftOne_JSONResult::JSONStartRow()
{
    return __startRow();
}

NodeAction RyftOne_JSONResult::JSONStartArray( std::string sName )
{
    __qualifiedCol.push_back(sName + ".[]");
    return ProcessNode;
}

NodeAction RyftOne_JSONResult::JSONStartGroup( std::string sName )
{
    __qualifiedCol.push_back(sName);
    return ProcessNode;
}

NodeAction RyftOne_JSONResult::JSONAddElement( std::string sName, const char **psAttribs, JSONElement **ppElement )
{
    string qualifiedName;
    deque<string>::iterator itr;
    for(itr = __qualifiedCol.begin(); itr != __qualifiedCol.end(); itr++) {
        if(!qualifiedName.empty())
            qualifiedName += ".";
        qualifiedName += (*itr);
    }
    // sName will be empty when its enumerating LIST values
    if(!sName.empty()) {
        if(!qualifiedName.empty())
            qualifiedName += ".";
        qualifiedName += sName;
    }
    __addElement(qualifiedName, NULL, NULL);
    return ProcessNode;
}

void RyftOne_JSONResult::JSONAddText( std::string sText )
{
    if(!__qualifiedCol.empty()) {
        __addText(sText, __delimiter);
    }
    else
        __addText(sText, "");
}

void RyftOne_JSONResult::JSONExitArray()
{
    __qualifiedCol.pop_back();
}

void RyftOne_JSONResult::JSONExitGroup()
{
    __qualifiedCol.pop_back();
}

void RyftOne_JSONResult::JSONExitRow()
{
    __exitRow();
}

RyftOne_Columns RyftOne_JSONResult::__getColumns(__meta_config__ meta_config)
{
    int idx;
    RyftOne_Column col;
    RyftOne_Columns cols;
    vector<__meta_config__::__meta_col__>::iterator colItr;
    for(idx = 0, colItr = meta_config.columns.begin(); colItr != meta_config.columns.end(); colItr++, idx++) {
        col.m_ordinal = idx+1;
        col.m_tableName = meta_config.table_name;
        col.m_colTag = colItr->json_tag;
        col.m_colName = colItr->name;
        col.m_typeName = colItr->type_def;
        RyftOne_Util::RyftToSqlType(col.m_typeName, &col.m_dataType, &col.m_charCols, &col.m_bufLength, col.m_formatSpec, &col.m_dtType);
        col.m_description = colItr->description;
        cols.push_back(col);
    }
    return cols;
}

IFile *RyftOne_JSONResult::__createFormattedFile(const char *filename)
{
    return new JSONFile(filename, __no_top);
}

string RyftOne_JSONResult::__getFormatString()
{
    return "json";
}

bool RyftOne_JSONResult::__isStructuredType()
{
    return true;
}

void RyftOne_JSONResult::__loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr)
{
    size_t idx1, idx2;
    string path;
    glob_t glob_results;
    char **relpaths;
    vector<__meta_config__::__meta_col__>::iterator colItr;

    __name = in_name;
    __path = in_itr->__path;

    path = __path;
    path += "/";
    path += in_itr->meta_config.file_glob;

    idx1 = in_itr->meta_config.file_glob.find(".");
    if(idx1 != string::npos) 
        __extension = in_itr->meta_config.file_glob.substr(idx1+1);

    glob(path.c_str(), GLOB_TILDE, NULL, &glob_results);
    relpaths = new char *[glob_results.gl_pathc];
    for(int i = 0; i < glob_results.gl_pathc; i++) {
        __glob.push_back(glob_results.gl_pathv[i]);
    }
    globfree(&glob_results);

    __no_top = in_itr->meta_config.no_top;

    for(colItr = in_itr->meta_config.columns.begin(); (colItr != in_itr->meta_config.columns.end()); colItr++) {
        __metaTags.push_back(colItr->json_tag);
    }

    __delimiter = in_itr->meta_config.delimiter;
}

void RyftOne_JSONResult::__parse(int fd, size_t st_size, bool no_top, string top_object)
{
    JSONParse(fd, st_size, no_top, top_object);   
}