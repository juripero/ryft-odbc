// =================================================================================================
///  @file R1QueryProcedure.cpp
///
///  Implementation of the Class CBResultAndParamsProcedure
///
///  Copyright (C) 2010-2011 Simba Technologies Incorporated.
// =================================================================================================

#include "R1QueryProcedure.h"

#include "DSIResults.h"
#include "DSISimpleRowCountResult.h"
#include "ILogger.h"
#include "IStatement.h"
#include "SEInvalidArgumentException.h"

using namespace RyftOne;
using namespace Simba::DSI;
using namespace Simba::SQLEngine;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1QueryProcedure::R1QueryProcedure(
    RyftOne_Database *in_ryft1,
    IStatement* in_statement) : 
        DSIExtProcedure(L"DBF", L"PROC", R1_PROC_CREATEADHOC),
        m_log(in_statement->GetLog()),
        m_statement(in_statement),
        m_ryft1(in_ryft1),
        __surrounding(0)
{
    ENTRANCE_LOG(
        m_log, "RyftOne", "R1QueryProcedure", "R1QueryProcedure");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1QueryProcedure::Execute(ParameterValues* in_parameters)
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1QueryProcedure", "Execute");

    assert(NULL != in_parameters);

    in_parameters->at(0).GetStringValue(__adhoc_table);
    in_parameters->at(1).GetStringValue(__file_glob);
    in_parameters->at(2).GetStringValue(__search_term);
    __surrounding = in_parameters->at(3).GetInt32Value();

    DSISimpleRowCountResult* result = (DSISimpleRowCountResult*)m_results->GetCurrentResult();
    result->SetRowCount(1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ParameterMetadataList* R1QueryProcedure::GetParameters()
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1QueryProcedure", "GetParameters");

    if (m_parameters.empty())
    {
        m_parameters.reserve(4);
        m_parameters.push_back(DSIExtParameterMetadata(0, DSI_PARAM_INPUT, SQL_VARCHAR, false, true));
        m_parameters.push_back(DSIExtParameterMetadata(1, DSI_PARAM_INPUT, SQL_VARCHAR, false, true));
        m_parameters.push_back(DSIExtParameterMetadata(2, DSI_PARAM_INPUT, SQL_VARCHAR, false, true));
        m_parameters.push_back(DSIExtParameterMetadata(3, DSI_PARAM_INPUT, SQL_INTEGER, false, true));
    }
    return &m_parameters;
}

// Protected =======================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
IResults* R1QueryProcedure::DoGetResults()
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1QueryProcedure", "DoGetResults");

    if (m_results.IsNull()) {

        // Create the results that we'll use.
        m_results = new DSIResults();
        m_results->AddResult(new DSISimpleRowCountResult(ROW_COUNT_UNKNOWN));
    }
    return m_results.Get();
}
