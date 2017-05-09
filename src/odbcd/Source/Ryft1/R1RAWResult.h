// =================================================================================================
///  @file R1RAWResult.h
///
///  Implements the class for unstructured file search results
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================
#ifndef _R1RAWRESULT_H_
#define _R1RAWRESULT_H_

#include "RyftOne.h"
using namespace RyftOne;

#include <string.h>
#include <string>
#include <vector>
using namespace std;

#include "R1IQueryResult.h"

class RyftOne_RAWResult : public IQueryResult {
public:
    RyftOne_RAWResult(ILogger *log) : IQueryResult(log) { ; }
   ~RyftOne_RAWResult( ) { ; }

protected:
    virtual RyftOne_Columns __getColumns(__meta_config__ meta_config);
    virtual IFile * __createFormattedFile(const char *filename);
    virtual string __getFormatString();
    virtual void __loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr);
    virtual bool __execute();
    virtual void __parse(int fd, size_t st_size, bool no_top, string top_object);
};
#endif
