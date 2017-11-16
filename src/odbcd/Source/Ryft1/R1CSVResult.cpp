// =================================================================================================
///  @file R1CSVResult.cpp
///
///  Implements the class for CSV file search results
///
///  Copyright (C) 2017 Ryft Systems, Inc.
// =================================================================================================

#include "R1CSVResult.h"
#include <sstream>

NodeAction RyftOne_CSVResult::CSVStartRow()
{
    return __startRow();
}

NodeAction RyftOne_CSVResult::CSVAddElement(int valueIndex)
{
    return __addElement(valueIndex-1);
}

void RyftOne_CSVResult::CSVAddText(std::string sText) 
{
    __addText(sText, "");
}

void RyftOne_CSVResult::CSVExitRow() 
{
    __exitRow();
}

RyftOne_Columns RyftOne_CSVResult::__getColumns(__meta_config__ meta_config)
{
    int idx;
    RyftOne_Column col;
    RyftOne_Columns cols;
    vector<__meta_config__::__meta_col__>::iterator colItr;
    for(idx = 0, colItr = meta_config.columns.begin(); colItr != meta_config.columns.end(); colItr++, idx++) {
        col.m_ordinal = idx+1;
        col.m_tableName = meta_config.table_name;
        col.m_colAlias = colItr->meta_name;
        col.m_colName = colItr->name;
        col.m_typeName = colItr->type_def;
        RyftOne_Util::RyftToSqlType(col.m_typeName, &col.m_dataType, &col.m_charCols, &col.m_bufLength, col.m_formatSpec, &col.m_dtType);
        col.m_description = colItr->description;
        cols.push_back(col);
    }
    return cols;
}

IFile *RyftOne_CSVResult::__createFormattedFile(const char *filename)
{
    return new CSVFile(filename, __field_delimiter, __record_delimiter);
}

string RyftOne_CSVResult::__getFormatString()
{
    return "csv";
}

bool RyftOne_CSVResult::__isStructuredType()
{
    return true;
}

void RyftOne_CSVResult::__loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr)
{
    size_t idx1;
    string path;
    glob_t glob_results;
    char **relpaths;
    vector<__meta_config__::__meta_col__>::iterator colItr;

    __name = in_name;
    __path = in_itr->__path;

    for(colItr = in_itr->meta_config.columns.begin(); (colItr != in_itr->meta_config.columns.end()); colItr++) 
        __metaTags.push_back(colItr->json_or_xml_tag);

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

    __field_delimiter = in_itr->meta_config.field_delimiter;
    __record_delimiter = in_itr->meta_config.record_delimiter;
}

bool RyftOne_CSVResult::__execute() 
{
    string find;
    string replace;
    size_t at;
    size_t idx;
    vector<string>::iterator tagItr;
    for(idx = 1, tagItr = __metaTags.begin(); tagItr != __metaTags.end(); tagItr++, idx++) {
        find = "RECORD." + *tagItr;
        replace = "RECORD." + static_cast<ostringstream*>( &(ostringstream() << idx) )->str();
        while((at = __query.find(find)) != string::npos) {
            __query.replace(at, find.length(), replace);
        }
    }

    return IQueryResult::__execute();
}

void RyftOne_CSVResult::__parse(int fd, size_t st_size, bool no_top, string top_object)
{
    CSVParse(fd, st_size, __field_delimiter, __record_delimiter);
}
