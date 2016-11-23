// =================================================================================================
///  @file R1JSONResult.h
///
///  Implements the result class for XML data searches
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================
#ifndef _R1JSONRESULT_H_
#define _R1JSONRESULT_H_

#include <string.h>
#include <string>
#include <vector>
#include <deque>
using namespace std;

#include "R1IQueryResult.h"

class RyftOne_JSONResult : public IQueryResult, public JSONParser {
public:
    RyftOne_JSONResult(ILogger *log) : IQueryResult(log) { ; }
   ~RyftOne_JSONResult( ) { ; }

protected:
    virtual NodeAction JSONStartRow();
    virtual NodeAction JSONStartArray( std::string sName );
    virtual NodeAction JSONStartGroup( std::string sName );
    virtual NodeAction JSONAddElement( std::string sName, const char **psAttribs, JSONElement **ppElement );
    virtual void JSONAddText( std::string sText );
    virtual void JSONExitArray();
    virtual void JSONExitGroup();
    virtual void JSONExitRow();

    virtual RyftOne_Columns __getColumns(__meta_config__ meta_config);
    virtual IFile * __createFormattedFile(const char *filename);
    virtual string __getFormatString();
    virtual bool __isStructuredType();
    virtual void __loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr);
    virtual void __parse(int fd, size_t st_size, bool no_top, string top_object);

private:
    deque<string> __qualifiedCol;
};
#endif
