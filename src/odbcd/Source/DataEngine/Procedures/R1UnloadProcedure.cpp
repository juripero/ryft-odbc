// =================================================================================================
///  @file R1UnloadProcedure.cpp
///
///  Implementation of the Class CBResultAndParamsProcedure
///
///  Copyright (C) 2010-2011 Simba Technologies Incorporated.
// =================================================================================================

#include "R1Table.h"
#include "R1UnloadProcedure.h"

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
R1UnloadProcedure::R1UnloadProcedure(
    RyftOne_Database *in_ryft1,
    IStatement* in_statement) : 
        DSIExtProcedure(L"DBF", L"PROC", R1_PROC_UNLOAD),
        m_log(in_statement->GetLog()),
        m_statement(in_statement),
        m_ryft1(in_ryft1)
{
    ENTRANCE_LOG(
        m_log, "RyftOne", "R1UnloadProcedure", "R1UnloadProcedure");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1UnloadProcedure::Execute(ParameterValues* in_parameters)
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1UnloadProcedure", "Execute");

    assert(NULL != in_parameters);

    in_parameters->at(0).GetStringValue(__table);
    in_parameters->at(1).GetStringValue(__search_term);
 
    IQueryResult * table = m_ryft1->OpenTable(__table);
    if(!table)
        return;

    table->AppendFilter(__search_term);
    int numrows = table->UnloadResults();

    DSISimpleRowCountResult* result = (DSISimpleRowCountResult*)m_results->GetCurrentResult();
    result->SetRowCount(numrows);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ParameterMetadataList* R1UnloadProcedure::GetParameters()
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1UnloadProcedure", "GetParameters");

    if (m_parameters.empty())
    {
        m_parameters.reserve(2);
        m_parameters.push_back(DSIExtParameterMetadata(0, DSI_PARAM_INPUT, SQL_VARCHAR, false, true));
        m_parameters.push_back(DSIExtParameterMetadata(1, DSI_PARAM_INPUT, SQL_VARCHAR, false, true));
    }
    return &m_parameters;
}

// Protected =======================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
IResults* R1UnloadProcedure::DoGetResults()
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1UnloadProcedure", "DoGetResults");

    if (m_results.IsNull()) {

        // Create the results that we'll use.
        m_results = new DSIResults();
        m_results->AddResult(new DSISimpleRowCountResult(ROW_COUNT_UNKNOWN));
    }
    return m_results.Get();
}
