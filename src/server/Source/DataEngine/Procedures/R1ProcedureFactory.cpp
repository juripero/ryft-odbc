// =================================================================================================
///  @file R1ProcedureFactory.cpp
///
///  Implementation of the Class R1ProcedureFactory
///
///  Copyright (C) 2010-2011 Simba Technologies Incorporated.
// =================================================================================================

#include "R1ProcedureFactory.h"
#include "R1UnloadProcedure.h"
#include "ILogger.h"
#include "IStatement.h"
#include "SEInvalidArgumentException.h"

using namespace RyftOne;
using namespace Simba::SQLEngine;
using namespace Simba::DSI;
using namespace std;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1ProcedureFactory::R1ProcedureFactory(
    RyftOne_Database *in_ryft1,
    IStatement* in_statement) :
        m_ryft1(in_ryft1),
        m_statement(in_statement)
{
    SE_CHK_INVALID_ARG(
        (NULL == in_statement));

    ENTRANCE_LOG(
        m_statement->GetLog(), "RyftOne", "R1ProcedureFactory", "R1ProcedureFactory");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1ProcedureFactory::~R1ProcedureFactory()
{
    list<DSIExtProcedure*>::iterator itr;
    for (itr = m_procedures.begin(); itr != m_procedures.end(); ++itr)
    {
        delete *itr;
        *itr = NULL;
    }

    m_procedures.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
SharedPtr<DSIExtProcedure> R1ProcedureFactory::CreateProcedure(
    const simba_wstring& in_procName)
{
    ENTRANCE_LOG(
        m_statement->GetLog(), "RyftOne", "R1ProcedureFactory", "CreateProcedure");

    if (in_procName.IsEqual(R1_PROC_UNLOAD, false))
    {
        return SharedPtr<DSIExtProcedure> (
            new R1UnloadProcedure(m_ryft1, m_statement));
    }

    return SharedPtr<DSIExtProcedure>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const list<DSIExtProcedure*>& R1ProcedureFactory::GetProcedures()
{
    ENTRANCE_LOG(
        m_statement->GetLog(), "RyftOne", "R1ProcedureFactory", "GetProcedures");

    if (m_procedures.empty())
    {
        InitializeProcedures();
    }

    return m_procedures;
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void R1ProcedureFactory::InitializeProcedures()
{
    ENTRANCE_LOG(
        m_statement->GetLog(), "RyftOne", "R1ProcedureFactory", "InitializeProcedures");

    m_procedures.push_back(new R1UnloadProcedure(m_ryft1, m_statement));
}
