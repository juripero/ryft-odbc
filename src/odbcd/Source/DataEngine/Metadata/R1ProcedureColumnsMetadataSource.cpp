// =================================================================================================
///  @file R1ProcedureColumnsMetadataSource.cpp
///
///  Implementation of the class R1ProcedureColumnsMetadataSource.
///
///  Copyright (C) 2010-2011 Simba Technologies Incorporated.
// =================================================================================================

#include "R1ProcedureColumnsMetadataSource.h"
#include "R1ProcedureFactory.h"
#include "R1Util.h"
#include "EncodingInfo.h"
#include "IColumn.h"
#include "IColumns.h"
#include "IResults.h"
#include "DSITypeUtilities.h"
#include "DSIPropertyValues.h"      // For DSI_RESULT_COL
#include "DSIExtProcedure.h"
#include "SqlData.h"
#include "SqlDataTypeUtilities.h"
#include "SqlTypeMetadataFactory.h"

using namespace RyftOne;
using namespace Simba::DSI;
using namespace Simba::SQLEngine;
using namespace std;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1ProcedureColumnsMetadataSource::R1ProcedureColumnsMetadataSource(
    DSIMetadataRestrictions& in_restrictions,
    R1ProcedureFactory* in_procedureFactory) :
        DSIMetadataSource(in_restrictions),
        m_procedureFactory(in_procedureFactory),
        m_currentResult(NULL),
        m_currentResultCol(NULL),
        m_resultColumnCounter(0),
        m_hasStartedFetch(false),
        m_isOnParameter(false),
        m_hasReturnValue(false),
        m_hasStartedParamItr(false),
        m_hasCalledResetOnResult(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1ProcedureColumnsMetadataSource::~R1ProcedureColumnsMetadataSource()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1ProcedureColumnsMetadataSource::CloseCursor()
{
    m_hasStartedFetch = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1ProcedureColumnsMetadataSource::GetMetadata(
    DSIOutputMetadataColumnTag in_columnTag, 
    SqlData* in_data, 
    simba_signed_native in_offset,
    simba_signed_native in_maxSize)
{
    switch (in_columnTag)
    {
        case DSI_CATALOG_NAME_COLUMN_TAG:
        {
            simba_wstring catName;
            (*m_procItr)->GetCatalogName(catName);
            return DSITypeUtilities::OutputWVarCharStringData(
                &(catName), 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_SCHEMA_NAME_COLUMN_TAG:
        {
            simba_wstring schName;
            (*m_procItr)->GetSchemaName(schName);
            return DSITypeUtilities::OutputWVarCharStringData(
                &(schName),
                in_data, 
                in_offset, 
                in_maxSize);
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

        case DSI_COLUMN_NAME_COLUMN_TAG:
        {
            simba_wstring colName;
            if (m_isOnParameter)
            {
                colName = m_paramItr->GetName();
            }
            else
            {
                m_currentResultCol->GetName(colName);
            }
            return DSITypeUtilities::OutputWVarCharStringData(
                &(colName),
                in_data,
                in_offset,
                in_maxSize);
        }

        case DSI_PROCEDURE_COLUMN_TYPE_COLUMN_TAG:
        {
            simba_uint16 paramType = DSI_RESULT_COL;
            if (m_isOnParameter)
            {
                paramType = static_cast<simba_uint16> (m_paramItr->GetParameterType());
            }

            memcpy(in_data->GetBuffer(), &(paramType), sizeof(simba_uint16));
            return false;
        }

        case DSI_DATA_TYPE_COLUMN_TAG:
        {
            simba_int16 sqlType = SQL_UNKNOWN_TYPE;
            if (m_isOnParameter)
            {
                sqlType = m_paramItr->GetSqlType();
            }
            else
            {
                sqlType = m_currentResultCol->GetMetadata()->GetSqlType();
            }

            memcpy(in_data->GetBuffer(), &(sqlType), sizeof(simba_int16));
            return false;
        }

        case DSI_DATA_TYPE_NAME_COLUMN_TAG:
        {
            simba_int16 sqlType = SQL_UNKNOWN_TYPE;
            simba_int32 colSize = 0;
            if (m_isOnParameter)
            {
                sqlType = m_paramItr->GetSqlType();
                colSize = m_paramItr->GetPrecision();
            }
            else
            {
                sqlType = m_currentResultCol->GetMetadata()->GetSqlType();
                colSize = m_currentResultCol->GetColumnSize();
            }

            simba_wstring typeName = simba_wstring::CreateFromUTF8(
                RyftOne_Util::SqlToRyftType(sqlType, colSize));

            return DSITypeUtilities::OutputWVarCharStringData(
                &(typeName),
                in_data,
                in_offset,
                in_maxSize);
        } 

        case DSI_COLUMN_SIZE_COLUMN_TAG:
        {
            simba_int32 colSize = 0;
            if (m_isOnParameter)
            {
                colSize = m_paramItr->GetPrecision();
            }
            else
            {
                colSize = m_currentResultCol->GetColumnSize();
            }

            memcpy(in_data->GetBuffer(), &(colSize), sizeof(simba_int32));
            return false;
        }

        case DSI_BUFFER_LENGTH_COLUMN_TAG:
        {
            simba_int32 bufLen = 0;
            if (m_isOnParameter)
            {
                bufLen = m_paramItr->GetLength();
            }
            else
            {
                bufLen = m_currentResultCol->GetColumnSize();
            }

            memcpy(in_data->GetBuffer(), &(bufLen), sizeof(simba_int32));
            return false;
        }

        case DSI_DECIMAL_DIGITS_COLUMN_TAG:
        {
            if (m_isOnParameter)
            {
                SqlTypeMetadata* typeMeta = m_paramItr->GetMetadata();
                simba_int16 sqlType = m_paramItr->GetSqlType();

                if (typeMeta->IsDateTimeType() && (SQL_TYPE_DATE != sqlType))
                {
                    simba_uint32 numDecs = m_paramItr->GetIntervalPrecision();
                    memcpy(in_data->GetBuffer(), &numDecs, sizeof(simba_uint32));
                }
                else if (typeMeta->IsExactNumericType())
                {
                    simba_uint32 numDecs = m_paramItr->GetScale();
                    memcpy(in_data->GetBuffer(), &numDecs, sizeof(simba_uint32));
                }
                else
                {
                    in_data->SetNull(true);
                }
            }
            else
            {
                SqlTypeMetadata* metadata = m_currentResultCol->GetMetadata();
                simba_int16 sqlType = metadata->GetSqlType();

                if (metadata->IsDateTimeType() && (SQL_TYPE_DATE != sqlType))
                {
                    simba_uint32 numDecs = 
                        static_cast<simba_uint32> (metadata->GetLengthOrIntervalPrecision());
                    memcpy(in_data->GetBuffer(), &numDecs, sizeof(simba_uint32));
                }
                else if (metadata->IsExactNumericType())
                {
                    simba_uint32 numDecs = metadata->GetScale();
                    memcpy(in_data->GetBuffer(), &numDecs, sizeof(simba_uint32));
                }
                else
                {
                    in_data->SetNull(true);
                }
            }
            return false;
        }

        case DSI_NUM_PREC_RADIX_COLUMN_TAG:
        {
            SqlTypeMetadata* typeMeta = NULL;
            if (m_isOnParameter)
            {
                typeMeta = m_paramItr->GetMetadata();
            }
            else
            {
                typeMeta = m_currentResultCol->GetMetadata();
            }

            if (typeMeta->IsExactNumericType() || typeMeta->IsApproximateNumericType())
            {
                simba_int16 prec = 0;
                if (m_isOnParameter)
                {
                    prec = static_cast<simba_int16> (m_paramItr->GetNumPrecRadix());
                }
                else
                {
                    prec = 
                        static_cast<simba_int16> (m_currentResultCol->GetMetadata()->GetNumPrecRadix());
                }

                memcpy(in_data->GetBuffer(), &prec, sizeof(simba_int16));
            }
            else
            {
                in_data->SetNull(true);
            }
            return false;
        }

        case DSI_NULLABLE_COLUMN_TAG:
        {            
            // SQL_NO_NULLS if the column could not include NULL values.
            // SQL_NULLABLE if the column accepts NULL values.
            // SQL_NULLABLE_UNKNOWN if it is not known whether the column accepts NULL values.
            DSINullable nullable = DSI_NULLABLE_UNKNOWN;

            if (m_isOnParameter)
            {
                if (DSI_PARAM_RETURN_VALUE == m_paramItr->GetParameterType())
                {
                    nullable = DSI_NO_NULLS;
                }
                else
                {
                    nullable = m_paramItr->IsNullable();
                }
            }
            else
            {
                nullable = m_currentResultCol->IsNullable();
            }

            simba_int16 value = SQL_NULLABLE_UNKNOWN;
            if (DSI_NULLABLE == nullable)
            {
                value = SQL_NULLABLE;
            }
            else if (DSI_NO_NULLS == nullable)
            {
                value = SQL_NO_NULLS;
            }

            memcpy(in_data->GetBuffer(), &value, sizeof(simba_int16));
            return false;
        }

        case DSI_REMARKS_COLUMN_TAG:
        {
            in_data->SetNull(true);
            return false;
        }

        case DSI_COLUMN_DEF_COLUMN_TAG:
        {
            in_data->SetNull(true);
            return false;
        }

        case DSI_SQL_DATA_TYPE_COLUMN_TAG:
        {
            // Get verbose type for date and interval types.
            simba_int16 verboseType = SQL_UNKNOWN_TYPE;
            if (m_isOnParameter)
            {
                verboseType = m_paramItr->GetMetadata()->GetVerboseType();
            }
            else
            {
                verboseType = m_currentResultCol->GetMetadata()->GetVerboseType();
            }

            memcpy(in_data->GetBuffer(), &verboseType, sizeof(simba_int16));
            return false;
        }

        case DSI_SQL_DATETIME_SUB_COLUMN_TAG:
        {
            simba_int16 intervalCode = 0;
            if (m_isOnParameter)
            {
                intervalCode = m_paramItr->GetMetadata()->GetIntervalCode();
            }
            else
            {
                intervalCode = m_currentResultCol->GetMetadata()->GetIntervalCode();
            }

            if (0 == intervalCode)
            {
                // The type is not date or interval.
                in_data->SetNull(true);    
            }
            else
            {
                memcpy(in_data->GetBuffer(), &intervalCode, sizeof(simba_int16));
            }
            return false;
        }

        case DSI_CHAR_OCTET_LENGTH_COLUMN_TAG:
        {
            SqlTypeMetadata* typeMeta = NULL;
            if (m_isOnParameter)
            {
                typeMeta = m_paramItr->GetMetadata();
            }
            else
            {
                typeMeta = m_currentResultCol->GetMetadata();
            }

            if (typeMeta->IsCharacterOrBinaryType())
            {
                simba_uint32 octetLen = 0;
                if (m_isOnParameter)
                {
                    octetLen = typeMeta->GetOctetLength(
                        m_paramItr->GetLength(),
                        EncodingInfo::GetNumBytesInCodeUnit(SIMBA_PLATFORM_WCHAR_ENCODING));
                }
                else
                {
                    octetLen = typeMeta->GetOctetLength(
                        m_currentResultCol->GetColumnSize(),
                        EncodingInfo::GetNumBytesInCodeUnit(SIMBA_PLATFORM_WCHAR_ENCODING));
                }

                memcpy(in_data->GetBuffer(), &octetLen, sizeof(simba_uint32));
            }
            else
            {
                in_data->SetNull(true);
            }
            return false;
        }

        case DSI_ORDINAL_POSITION_COLUMN_TAG:
        {
            simba_int32 ordinalPosition = 0;
            if (m_isOnParameter)
            {
                ordinalPosition = m_paramItr->GetParameterNumber();

                if (!m_hasReturnValue)
                {
                    // Return values always occupy parameter number 0, others start at 1.
                    ordinalPosition++;
                }
            }
            else
            {
                // Result set columns are indexed beginning at 1, so add 1.
                ordinalPosition = static_cast<simba_int32> (m_resultColumnCounter) + 1;
            }

            memcpy(in_data->GetBuffer(), &(ordinalPosition), sizeof(simba_int32));
            return false;
        }

        case DSI_IS_NULLABLE_COLUMN_TAG:
        {
            // "NO" if the column does not include NULLs.
            // "YES" if the column could include NULLs.
            // This column returns a zero-length string if nullability is unknown.  
            DSINullable nullable = DSI_NULLABLE_UNKNOWN;

            if (m_isOnParameter)
            {
                if (DSI_PARAM_RETURN_VALUE == m_paramItr->GetParameterType())
                {
                    nullable = DSI_NO_NULLS;
                }
                else
                {
                    nullable = m_paramItr->IsNullable();
                }
            }
            else
            {
                nullable = m_currentResultCol->IsNullable();
            }

            simba_wstring isNullable("");
            if (DSI_NULLABLE == nullable)
            {
                isNullable = "YES";
            }
            else if (DSI_NO_NULLS == nullable)
            {
                isNullable = "NO";
            }

            return DSITypeUtilities::OutputWVarCharStringData(
                &isNullable, 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_IS_RESULT_SET_COLUMN_TAG:
        {
            simba_int16 isParam = SQL_TRUE;
            if (m_isOnParameter)
            {
                isParam = SQL_FALSE;
            }

            memcpy(in_data->GetBuffer(), &(isParam), sizeof(simba_uint16));
            return false;
        }

        case DSI_USER_DATA_TYPE_COLUMN_TAG:
        {
            // Get the user data type.
            simba_uint16 dataType = UDT_STANDARD_SQL_TYPE;
            if (m_isOnParameter)
            {
                dataType = m_paramItr->GetUserDataType();
            }
            else
            {
                dataType = m_currentResultCol->GetMetadata()->GetUserDataType();
            }
            memcpy(in_data->GetBuffer(), &dataType, sizeof(simba_uint16));
            return false;
        }

        default:
        {
            R1THROWGEN(L"R1MetadataColumnNotFound");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1ProcedureColumnsMetadataSource::Move(
    DSIDirection in_direction,
    simba_signed_native in_offset)
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

        // Set this to true for the first stored procedure.  Parameter columns must
        // come before result set columns.
        m_isOnParameter = true;
    }
   
    return this->GetCurrentColumn();
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1ProcedureColumnsMetadataSource::GetCurrentColumn()
{
    while (m_procedureFactory->GetProcedures().end() != m_procItr)
    {
        if (m_isOnParameter && this->GetCurrentParameterColumn())
        {
            return true;
        }
        else if (this->GetCurrentResultSetColumn())
        {
            // No more parameters for the current stored procedure.  So check if there are result set
            // columns.
            return true;
        }
        else
        {
            ++m_procItr;
            m_isOnParameter = true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1ProcedureColumnsMetadataSource::GetCurrentParameterColumn()
{
    if (!m_hasStartedParamItr)
    {
        ParameterMetadataList* paramsList = (*m_procItr)->GetParameters();
        if (paramsList)
        {
            m_paramItr = paramsList->begin();
            m_isOnParameter = true;
            m_hasStartedParamItr = true;
            m_hasReturnValue = (DSI_PARAM_RETURN_VALUE == m_paramItr->GetParameterType());
            return true;
        }
        
        // The current stored procedure doesn't have any parameters.
        m_isOnParameter = false;
        return false;
    }

    ++m_paramItr;

    if ((*m_procItr)->GetParameters()->end() == m_paramItr)
    {
        m_isOnParameter = false;
        m_hasStartedParamItr = false;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1ProcedureColumnsMetadataSource::GetCurrentResultSetColumn()
{
    if (!m_hasCalledResetOnResult)
    {
        (*m_procItr)->GetResults()->Reset();
        m_currentResult = (*m_procItr)->GetResults()->GetCurrentResult();
        m_resultColumnCounter = 0;  // Reset the column counter.
        m_hasCalledResetOnResult = true;
    }
    else
    {
        ++m_resultColumnCounter;
    }

    if (!m_currentResult->GetSelectColumns() || 
        m_resultColumnCounter >= m_currentResult->GetSelectColumns()->GetColumnCount())
    {
        // Find the next result set that has a column.
        while ((*m_procItr)->GetResults()->Next())
        {
            m_currentResult = (*m_procItr)->GetResults()->GetCurrentResult();

            if (m_currentResult->GetSelectColumns())
            {
                m_resultColumnCounter = 0;
                break;
            }
        }

        if (!m_currentResult->GetSelectColumns() || 
            m_resultColumnCounter >= m_currentResult->GetSelectColumns()->GetColumnCount())
        {
            m_resultColumnCounter = 0;
            m_hasCalledResetOnResult = false;
            m_isOnParameter = true;
            return false;
        }
    }

    m_currentResultCol = m_currentResult->GetSelectColumns()->GetColumn(
        static_cast<simba_uint16> (m_resultColumnCounter));

    m_isOnParameter = false;

    return true;
}
