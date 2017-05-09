// =================================================================================================
///  @file R1ProceduresMetadataSource.h
///
///  Definition of the class R1ProceduresMetadataSource
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1ProceduresMetadataSource.h"

#include "DSITypeUtilities.h"
#include "DSIExtProcedure.h"
#include "SqlData.h"

using namespace RyftOne;
using namespace Simba::DSI;
using namespace Simba::SQLEngine;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1ProceduresMetadataSource::R1ProceduresMetadataSource(
    DSIMetadataRestrictions& in_restrictions,
    R1ProcedureFactory *in_procedureFactory) :
        DSIMetadataSource(in_restrictions),
        m_procedureFactory(in_procedureFactory),
        m_hasStartedFetch(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1ProceduresMetadataSource::~R1ProceduresMetadataSource()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1ProceduresMetadataSource::CloseCursor()
{
    m_hasStartedFetch = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1ProceduresMetadataSource::GetMetadata(
    DSIOutputMetadataColumnTag in_columnTag, 
    SqlData* in_data, 
    simba_signed_native in_offset,
    simba_signed_native in_maxSize)
{
    switch (in_columnTag)
    {
        case DSI_CATALOG_NAME_COLUMN_TAG:
        {
            // Note that if your driver does not support catalogs, this case statement can simply
            // set NULL on in_data and return false.
            return DSITypeUtilities::OutputWVarCharStringData(
                R1_CATALOG, 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_SCHEMA_NAME_COLUMN_TAG:
        {
            in_data->SetNull(true);
            return false;
        }

        case DSI_PROCEDURE_NAME_COLUMN_TAG:
        {
            simba_wstring procName;
            (*m_procItr)->GetProcedureName(procName);
            return DSITypeUtilities::OutputWVarCharStringData(
                &(procName),
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_NUM_INPUT_PARAMS_COLUMN_TAG:
        {
            // As per the ODBC spec: http://msdn.microsoft.com/en-us/library/ms715368(VS.85).aspx,
            // this value is reserved for future use and applications should not rely on the data
            // returned in this column.
            simba_int32 numInputParams = -1;
            memcpy(in_data->GetBuffer(), &numInputParams, sizeof(simba_int32));
            return false;
        }

        case DSI_NUM_OUTPUT_PARAMS_COLUMN_TAG:
        {
            // As per the ODBC spec: http://msdn.microsoft.com/en-us/library/ms715368(VS.85).aspx,
            // this value is reserved for future use and applications should not rely on the data
            // returned in this column.
            simba_int32 numOutputParams = -1;
            memcpy(in_data->GetBuffer(), &numOutputParams, sizeof(simba_int32));
            return false;
        }

        case DSI_NUM_RESULT_SETS_COLUMN_TAG:
        {
            // As per the ODBC spec: http://msdn.microsoft.com/en-us/library/ms715368(VS.85).aspx,
            // this value is reserved for future use and applications should not rely on the data
            // returned in this column.
            simba_int32 numResultSets = -1;
            memcpy(in_data->GetBuffer(), &numResultSets, sizeof(simba_int32));
            return false;
        }

        case DSI_REMARKS_COLUMN_TAG:
        {
            in_data->SetNull(true);
            return false;
        }

        case DSI_PROCEDURE_TYPE_COLUMN_TAG:
        {   
            simba_int16 procType = this->GetProcedureType();
            memcpy(in_data->GetBuffer(), &procType, sizeof(simba_int16));
            return false;
        }

        default:
        {
            R1THROWGEN(L"R1MetadataColumnNotFound");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1ProceduresMetadataSource::Move(DSIDirection in_direction, simba_signed_native in_offset)
{
    UNUSED(in_offset);

    if (DSI_DIR_NEXT != in_direction)
    {
        R1THROWGEN(L"R1ResultSetTraverseDirNotSupported");
    }

    // Set up the iterators.
    if (!m_hasStartedFetch) 
    {
        m_hasStartedFetch = true;

        m_procItr = m_procedureFactory->GetProcedures().begin();
        if (m_procedureFactory->GetProcedures().end() == m_procItr)
        {
            return false;
        }
    }
    else
    {
        ++m_procItr;
        if (m_procedureFactory->GetProcedures().end() == m_procItr)
        {
            return false;
        }
    }

    return true;
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
simba_int16 R1ProceduresMetadataSource::GetProcedureType()
{
    if ((*m_procItr)->HasReturnValue())
    {
        return SQL_PT_FUNCTION;
    }

    return SQL_PT_PROCEDURE;
}
