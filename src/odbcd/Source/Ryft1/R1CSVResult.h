// =================================================================================================
///  @file R1CSVResult.h
///
///  Implements the class for CSV file search results
///
///  Copyright (C) 2017 Ryft Systems, Inc.
// =================================================================================================
#ifndef _R1CSVRESULT_H_
#define _R1CSVRESULT_H_

#include "RyftOne.h"
using namespace RyftOne;

#include <string.h>
#include <string>
#include <vector>
using namespace std;

#include "R1IQueryResult.h"

class RyftOne_CSVResult : public IQueryResult, public CSVParser {
public:
    RyftOne_CSVResult(ILogger *log) : IQueryResult(log) { ; }
   ~RyftOne_CSVResult( ) { ; }

protected:

    // CSVParser
    virtual NodeAction          CSVStartRow( );
    virtual NodeAction          CSVAddElement(int valueIndex);
    virtual void                CSVAddText( std::string sText );
    virtual void                CSVExitRow();

    virtual RyftOne_Columns __getColumns(__meta_config__ meta_config);
    virtual IFile * __createFormattedFile(const char *filename);
    virtual string __getFormatString();
    virtual bool __isStructuredType();
    virtual void __loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr);
    virtual bool __execute();
    virtual void __parse(int fd, size_t st_size, bool no_top, string top_object);

    string  __field_delimiter;
    string  __record_delimiter;
};
#endif