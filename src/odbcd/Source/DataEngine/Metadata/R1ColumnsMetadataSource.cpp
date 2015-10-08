// =================================================================================================
///  @file R1ColumnsMetadataSource.cpp
///
///  Implementation of the class R1ColumnsMetadataSource
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1ColumnsMetadataSource.h"

#include "BadColumnException.h"
#include "DSITypeUtilities.h"
#include "IColumn.h"
#include "SqlData.h"
#include "SqlDataTypeUtilities.h"
#include "TypeDefines.h"

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1ColumnsMetadataSource::R1ColumnsMetadataSource(DSIMetadataRestrictions& in_restrictions, RyftOne_Database *in_ryft1) :
    DSIMetadataSource(in_restrictions),
    m_hasStartedFetch(false)
{
    simba_wstring table;
    DSIMetadataRestrictions::const_iterator itr = in_restrictions.find(DSI_TABLE_NAME_COLUMN_TAG);
    if (itr != in_restrictions.end())
        table = itr->second; 

    string table_str = table.GetAsPlatformString();
    m_columns = in_ryft1->getColumns(table_str);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1ColumnsMetadataSource::~R1ColumnsMetadataSource()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1ColumnsMetadataSource::CloseCursor()
{
    m_hasStartedFetch = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1ColumnsMetadataSource::GetMetadata(
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

        case DSI_TABLE_NAME_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                simba_wstring::CreateFromUTF8(m_colItr->m_tableName.c_str()), 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_COLUMN_NAME_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                simba_wstring::CreateFromUTF8(m_colItr->m_colName.c_str()), 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_DATA_TYPE_COLUMN_TAG:
        case DSI_SQL_DATA_TYPE_COLUMN_TAG:
        {
            *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = m_colItr->m_dataType;
            return false;
        }

        case DSI_DATA_TYPE_NAME_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                simba_wstring::CreateFromUTF8(m_colItr->m_typeName.c_str()), 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_REMARKS_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                simba_wstring::CreateFromUTF8(m_colItr->m_description.c_str()), 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_NULLABLE_COLUMN_TAG:
        {
            *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = DSI_NULLABLE;
            return false;
        }

        case DSI_ORDINAL_POSITION_COLUMN_TAG:
        {
            *reinterpret_cast<simba_int32*>(in_data->GetBuffer()) = m_colItr->m_ordinal;
            return false;
        }

        case DSI_IS_NULLABLE_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                simba_wstring(L"YES"), 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_USER_DATA_TYPE_COLUMN_TAG:
        {
            // Custom column for custom user data types. Return UDT_STANDARD_SQL_TYPE always 
            // since RyftOne does not support any custom user data types.
            *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = UDT_STANDARD_SQL_TYPE;
            return false;
        }

        case DSI_CHAR_OCTET_LENGTH_COLUMN_TAG:
        {
            if (SqlDataTypeUtilitiesSingleton::GetInstance()->IsCharacterOrBinaryType(m_colItr->m_dataType))
            {
                *reinterpret_cast<simba_int32*>(in_data->GetBuffer()) = m_colItr->m_bufferLen;
            }
            else
            {
                in_data->SetNull(true);
            }
            return false;
        }

        case DSI_BUFFER_LENGTH_COLUMN_TAG:
        {
            *reinterpret_cast<simba_int32*>(in_data->GetBuffer()) = m_colItr->m_bufferLen;
            return false;
        }

        case DSI_NUM_PREC_RADIX_COLUMN_TAG:
        {
            // See http://msdn.microsoft.com/en-us/library/ms714632%28d=lightweight,v=VS.85%29.aspx
            if (SqlDataTypeUtilitiesSingleton::GetInstance()->IsApproximateNumericType(m_colItr->m_dataType))
            {
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = NUMPREC_BITS;
            }
            else if (SqlDataTypeUtilitiesSingleton::GetInstance()->IsIntegerType(m_colItr->m_dataType) ||
                     SqlDataTypeUtilitiesSingleton::GetInstance()->IsExactNumericType(m_colItr->m_dataType))
            {
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = NUMPREC_EXACT;
            }
            else
            {
                in_data->SetNull(true);
            }
            return false;
        }

        case DSI_SQL_DATETIME_SUB_COLUMN_TAG:
        {
            if (SqlDataTypeUtilitiesSingleton::GetInstance()->IsDatetimeType(m_colItr->m_dataType) ||
                SqlDataTypeUtilitiesSingleton::GetInstance()->IsIntervalType(m_colItr->m_dataType))
            {
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = 
                    SqlDataTypeUtilitiesSingleton::GetInstance()->GetIntervalCodeFromConciseType(
                        m_colItr->m_dataType);
            }
            else
            {
                in_data->SetNull(true);
            }

            return false;
        }

        case DSI_COLUMN_DEF_COLUMN_TAG:
        {
            in_data->SetNull(true);
            return false;
        }

        case DSI_COLUMN_SIZE_COLUMN_TAG:
        {
            switch(m_colItr->m_dataType) {
            case SQL_INTEGER:
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = 10;
                break;
            case SQL_SMALLINT:
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = 5;
                break;
            case SQL_BIGINT:
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = 20;
                break;
            case SQL_DOUBLE:
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = 15;
                break;
            case SQL_DATE:
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = 10;
                break;
            case SQL_TIME:
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = 8;
                break;
            case SQL_TIMESTAMP:
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = 19;
                break;
            case SQL_VARCHAR:
                *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = m_colItr->m_bufferLen;
                break;
            }
            return false;
        }

        case DSI_DECIMAL_DIGITS_COLUMN_TAG:
        {
            *reinterpret_cast<simba_int16*>(in_data->GetBuffer()) = 0;
            return false;
        }

        default:
        {
            // This shouldn't happen as this will be caught in the ODBC layer, however for safety
            // put an exception here.
            DSITHROWEX1(
                BadColumnException, 
                L"InvalidColumnNum", 
                NumberConverter::ConvertInt32ToWString(in_columnTag));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1ColumnsMetadataSource::Move(DSIDirection in_direction, simba_signed_native in_offset)
{
    if (DSI_DIR_NEXT != in_direction) 
        R1THROWGEN(L"R1ResultSetTraverseDirNotSupported");

    if (!m_hasStartedFetch) {
        m_hasStartedFetch = true;
        m_colItr = m_columns.begin();
    }
    else
        ++m_colItr;

    return (m_colItr < m_columns.end());
}

