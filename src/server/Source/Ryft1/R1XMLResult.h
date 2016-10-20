// =================================================================================================
///  @file R1XMLResult.h
///
///  Implements the class for results from XML searches
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================
#ifndef _R1XMLRESULT_H_
#define _R1XMLRESULT_H_

#include <string.h>
#include <string>
#include <vector>
using namespace std;

#include "R1IQueryResult.h"

class RyftOne_XMLResult : public IQueryResult, public XMLParser {
public:
    RyftOne_XMLResult(ILogger *log) : IQueryResult(log) { ; }
   ~RyftOne_XMLResult( ) { ; }

protected:
    // XMLParser

    virtual NodeAction StartRow();
    virtual NodeAction AddElement(std::string sName, const char **psAttribs, XMLElement **ppElement);
    virtual void AddText(std::string sText) ;
    virtual void ExitRow();

    virtual RyftOne_Columns __getColumns(__meta_config__ meta_config);
    virtual IFile * __createFormattedFile(const char *filename);
    virtual string __getFormatString();
    virtual bool __isStructuredType();
    virtual void __loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr);
    virtual void __parse(int fd, size_t st_size, bool no_top, string top_object);
};
#endif
