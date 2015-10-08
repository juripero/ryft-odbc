// =================================================================================================
///  @file R1QueryExecutor.cpp
///
///  Implementation of the Class R1QueryExecutor
///
///  Copyright (C) 2005-2013 Simba Technologies Incorporated.
// =================================================================================================

#include "R1QueryExecutor.h"

#include "BadDefaultParamException.h"
#include "DSIResults.h"
#include "DSISimpleRowCountResult.h"
#include "IColumn.h"
#include "ILogger.h"
#include "IParameterManager.h"
#include "IParameterSet.h"
#include "IParameterSetIter.h"
#include "IParameterSetStatusSet.h"
#include "IParameterSource.h"
#include "SqlData.h"
#include "R1ResultSet.h"

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1QueryExecutor::R1QueryExecutor(
    ILogger* in_log,  RyftOne_Statement *ryft1stmt, const simba_wstring& in_sqlQuery) :
        m_log(in_log), m_sqlQuery(in_sqlQuery), m_ryft1stmt(ryft1stmt), m_results(new DSIResults())
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1QueryExecutor", "R1QueryExecutor");

#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #10: Implement a QueryExecutor.")
    // Create the results. This is necessary even though no execution has happened as ODBC allows
    // metadata to be returned at prepare time. If your data source cannot return metadata before
    // execution, simply create an empty result set with no columns at prepare time, and then create
    // the proper columns at execution time. Note that some applications may have problems with the
    // lack of metadata, however ODBC states that applications should not rely on metadata at 
    // prepare. You should test to determine if any key applications have problems if you cannot
    // return metadata until execution.
    m_results->AddResult(new R1ResultSet(in_log));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1QueryExecutor::~R1QueryExecutor()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1QueryExecutor::CancelExecute()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1QueryExecutor", "CancelExecute");

    // It's not possible to cancel execution in the RyftOne driver, as there is no actual
    // 'execution', everything is already hardcoded within the driver. Note that if you do implement
    // cancellation, the canceled thread (the one doing the work, not the one that enters this 
    // function) should throw an OperationCanceledException to properly indicate that the operation
    // was canceled.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1QueryExecutor::ClearCancel()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1QueryExecutor", "ClearCancel");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1QueryExecutor::PopulateParameters(IParameterManager* in_parameterManager)
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1QueryExecutor", "PopulateParameters");

#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #11: Provide parameter information.")
    // If your statement contains parameters, this is where you will register them with SimbaODBC.
    // The code below provides examples for registering input-only, input-output and output-only
    // parameters. Parameter metadata information will have to be defined in terms of SQL data types.

}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1QueryExecutor::PushParamData(
    simba_unsigned_native in_paramSet,
    IParameterSource* in_paramSrc)
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1QueryExecutor", "PushParamData");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1QueryExecutor::FinalizePushedParamData()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1QueryExecutor", "FinalizePushedParamData");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1QueryExecutor::Execute(
    IWarningListener* in_warningListener, 
    IParameterSetIter* in_inputParamSetIter,
    IParameterSetIter* in_outputParamSetIter,
    IParameterSetStatusSet* in_paramSetStatusSet)
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1QueryExecutor", "Execute");

#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #12: Implement Query Execution.")
    // The in_inputParamSetIter and in_outputParamSetIter arguments provides access to the 
    // parameters that were bound using non-DATA_AT_EXEC version of SQLBindParameter.
    // Statement execution is usually a 3 step process:
    //      1. Serialize all input parameters into a form that can be consumed by the data source.
    //         If your data source does not support parameter streaming for DATA_AT_EXEC parameters,
    //         then you will need to re-assemble them from your parameter cache.
    //         See PushParamData.
    //      2. Send the Execute() message.
    //      3. Retrieve all output parameters from the server and update the in_outputParamSetIter
    //         with their contents.
    //
    // Note that this is simply a suggestion on how execution happens, actual execution steps will
    // depend on your data source.

    // Reset the result to point to before the first entry.
    m_results->Reset();

    string sqlQuery = m_sqlQuery.GetAsPlatformString();
    m_ryft1stmt->execute(sqlQuery.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Simba::DSI::IResults* R1QueryExecutor::GetResults()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1QueryExecutor", "GetResults");

    // Note that GetResults() will be called BEFORE execution to retrieve metadata. If your data 
    // source cannot return metadata before execution, simply create an empty result set with no 
    // columns at prepare time, and then create the proper columns at execution time. Note that some
    // applications may have problems with the lack of metadata, however ODBC states that 
    // applications should not rely on metadata at prepare. You should test to determine if any key 
    // applications have problems if you cannot return metadata until execution.
    return m_results.Get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
simba_uint16 R1QueryExecutor::GetNumParams()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1QueryExecutor", "GetNumParams");
    // no support for parameters
    return 0;
}
