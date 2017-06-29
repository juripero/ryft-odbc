// =================================================================================================
///  @file R1XMLResult.cpp
///
///  Implements the class for results from XML searches
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================

#include "R1XMLResult.h"

NodeAction RyftOne_XMLResult::StartRow()
{
    return __startRow();
}

NodeAction RyftOne_XMLResult::AddElement(std::string sName, const char **psAttribs, XMLElement **ppElement)
{
    return __addElement(sName, psAttribs, ppElement);
}

void RyftOne_XMLResult::AddText(std::string sText) 
{
    __addText(sText, "");
}

void RyftOne_XMLResult::ExitRow() 
{
    __exitRow();
}

RyftOne_Columns RyftOne_XMLResult::__getColumns(__meta_config__ meta_config)
{
    int idx;
    RyftOne_Column col;
    RyftOne_Columns cols;
    vector<__meta_config__::__meta_col__>::iterator colItr;
    for(idx = 0, colItr = meta_config.columns.begin(); colItr != meta_config.columns.end(); colItr++, idx++) {
        col.m_ordinal = idx+1;
        col.m_tableName = meta_config.table_name;
        col.m_colTag = colItr->xml_tag;
        col.m_colName = colItr->name;
        col.m_typeName = colItr->type_def;
        RyftOne_Util::RyftToSqlType(col.m_typeName, &col.m_dataType, &col.m_charCols, &col.m_bufLength, col.m_formatSpec, &col.m_dtType);
        col.m_description = colItr->description;
        cols.push_back(col);
    }
    return cols;
}

IFile *RyftOne_XMLResult::__createFormattedFile(const char *filename)
{
    return new XMLFile(filename, m_delim);
}

string RyftOne_XMLResult::__getFormatString()
{
    return "xml";
}

bool RyftOne_XMLResult::__isStructuredType()
{
    return true;
}

void RyftOne_XMLResult::__loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr)
{
    size_t idx1, idx2;
    string path;
    glob_t glob_results;
    char **relpaths;
    vector<__meta_config__::__meta_col__>::iterator colItr;

    __name = in_name;
    __path = in_itr->__path;

    idx1 = in_itr->meta_config.record_tag.find("<");
    idx2 = in_itr->meta_config.record_tag.find(">");
    m_delim = in_itr->meta_config.record_tag;
    if(idx1 != string::npos && idx2 != string::npos) 
        m_delim = in_itr->meta_config.record_tag.substr(idx1+1, idx2-idx1-1);

    for(colItr = in_itr->meta_config.columns.begin(); (colItr != in_itr->meta_config.columns.end()); colItr++) {
        __metaTags.push_back(colItr->xml_tag);
    }

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

    __delimiter = in_itr->meta_config.delimiter;
}

void RyftOne_XMLResult::__parse(int fd, size_t st_size, bool no_top, string top_object)
{
    XMLParse(fd, st_size, m_delim, no_top);
}
