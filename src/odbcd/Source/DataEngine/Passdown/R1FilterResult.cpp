// =================================================================================================
///  @file R1FilterResult.cpp
///
///  Implementation of the class R1FilterResult
///
///  Copyright (C) 2008-2011 Simba Technologies Incorporated.
// =================================================================================================

#include "R1FilterResult.h"
#include "SEInvalidArgumentException.h"
#include "SEInvalidOperationException.h"
#include "DSIResultSetColumns.h"

using namespace RyftOne;
using namespace Simba::DSI;
using namespace Simba::SQLEngine;
using namespace Simba::Support;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1FilterResult::R1FilterResult(
    SharedPtr<R1Table> in_table, 
    const simba_wstring& in_filter, int in_hamming) : 
        m_table(in_table),
        m_filter(in_filter),
        m_hasStartedFetch(false)
{
    assert(!in_table.IsNull());
    m_table->AppendFilter(m_filter, in_hamming);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1FilterResult::~R1FilterResult()
{
    ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1FilterResult::AppendRow( )
{
    m_table->AppendRow();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IColumns* R1FilterResult::GetSelectColumns()
{
    return m_table->GetSelectColumns();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterResult::HasRowCount()
{
    // The row count won't be known until all rows are fetched.
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
simba_unsigned_native R1FilterResult::GetRowCount()
{
    // It is not known how many rows in the filter result set.
    return ROW_COUNT_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1FilterResult::GetCatalogName(simba_wstring& out_catalogName)
{
    m_table->GetCatalogName(out_catalogName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1FilterResult::GetSchemaName(simba_wstring& out_schemaName)
{
    m_table->GetSchemaName(out_schemaName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1FilterResult::GetTableName(simba_wstring& out_tableName)
{
    m_table->GetTableName(out_tableName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1FilterResult::OnFinishRowUpdate()
{
    m_table->OnFinishRowUpdate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterResult::RetrieveData(
    simba_uint16 in_column,
    Simba::Support::SqlData* in_data,
    simba_signed_native in_offset,
    simba_signed_native in_maxSize)
{
    return m_table->RetrieveData(in_column, in_data, in_offset, in_maxSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterResult::WriteData(
    simba_uint16 in_column,
    Simba::Support::SqlData* in_sqlData,
    simba_signed_native in_offset,
    bool in_isDefault)
{
    return m_table->WriteData(in_column, in_sqlData, in_offset, in_isDefault);
}

// Protected =======================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void R1FilterResult::DoCloseCursor()
{
    m_table->DoCloseCursor();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1FilterResult::MoveToBeforeFirstRow()
{
    m_table->MoveToBeforeFirstRow();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterResult::MoveToNextRow()
{
    return m_table->MoveToNextRow();
}