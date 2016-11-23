// =================================================================================================
///  @file R1RAWResult.cpp
///
///  Implements the class for unstructured file search results
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================

#include "R1RAWResult.h"

RyftOne_Columns RyftOne_RAWResult::__getColumns(__meta_config__ meta_config)
{
    RyftOne_Column col;
    RyftOne_Columns cols;
    vector<__meta_config__::__meta_col__>::iterator colItr = meta_config.columns.begin();
    if(colItr != meta_config.columns.end()) {
        col.m_ordinal = 1;
        col.m_tableName = meta_config.table_name;
        col.m_colTag = "data";
        col.m_colName = colItr->name;
        col.m_typeName = colItr->type_def;
        RyftOne_Util::RyftToSqlType(col.m_typeName, &col.m_dataType, &col.m_charCols, &col.m_bufLength, col.m_formatSpec, &col.m_dtType);
        col.m_description = colItr->description;
        cols.push_back(col);
    }
    return cols;
}

IFile *RyftOne_RAWResult::__createFormattedFile(const char *filename)
{
    return NULL;
}

string RyftOne_RAWResult::__getFormatString()
{
    return "utf8";
}

void RyftOne_RAWResult::__loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr)
{
    size_t idx1, idx2;
    string path;
    glob_t glob_results;
    char **relpaths;
    vector<__meta_config__::__meta_col__>::iterator colItr;

    __name = in_name;
    __path = in_itr->path;

    path = __path;
    path += "/";
    path += in_itr->rdf_config.file_glob;

    idx1 = in_itr->rdf_config.file_glob.find(".");
    if(idx1 != string::npos) 
        __extension = in_itr->rdf_config.file_glob.substr(idx1+1);

    glob(path.c_str(), GLOB_TILDE, NULL, &glob_results);
    relpaths = new char *[glob_results.gl_pathc];
    for(int i = 0; i < glob_results.gl_pathc; i++) {
        __glob.push_back(glob_results.gl_pathv[i]);
    }
    globfree(&glob_results);

    __metaTags.push_back("data");
}

bool RyftOne_RAWResult::__execute() 
{
    if(__query.empty())
        R1THROWGEN("UnstructuredSelectException");

    return IQueryResult::__execute();
}

void RyftOne_RAWResult::__parse(int fd, size_t st_size, bool no_top, string top_object)
{
    char *buffer = new char[st_size+1];
    __startRow();
    __addElement("data", NULL, NULL);
    size_t st_read = read(fd, buffer, st_size);
    buffer[st_read] = '\0';
    __addText(buffer, "") ;
    delete[] buffer;
    __exitRow();
}
