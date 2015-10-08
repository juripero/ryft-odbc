//==================================================================================================
///  @file R1TypeInfoMetadataSource.cpp
///
///  Definition of the class R1TypeInfoMetadataSource.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
//==================================================================================================

#include "R1TypeInfoMetadataSource.h"

#include "BadColumnException.h"
#include "DSITypeUtilities.h"
#include "SqlData.h"
#include "SqlDataTypeUtilities.h"
#include "TypeDefines.h"

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1TypeInfoMetadataSource::R1TypeInfoMetadataSource(
    DSIMetadataRestrictions& in_restrictions, 
    bool in_isODBCV3) :
        DSIMetadataSource(in_restrictions),
        m_hasStartedFetch(false)
{
    InitializeData(in_isODBCV3);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1TypeInfoMetadataSource::~R1TypeInfoMetadataSource()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1TypeInfoMetadataSource::CloseCursor()
{
    m_hasStartedFetch = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1TypeInfoMetadataSource::GetMetadata(
    DSIOutputMetadataColumnTag in_columnTag, 
    Simba::Support::SqlData* in_data, 
    simba_signed_native in_offset,
    simba_signed_native in_maxSize)
{
    switch (in_columnTag)
    {
        case DSI_DATA_TYPE_NAME_COLUMN_TAG:
        {
            // Remove the "SQL_" part of the type name when retrieving it.
            SqlDataTypeUtilities* typeUtilities = SqlDataTypeUtilitiesSingleton::GetInstance();
            simba_wstring typeName(
                typeUtilities->GetStringForSqlType(m_rowItr->m_dataType).Substr(4));

            return DSITypeUtilities::OutputWVarCharStringData(
                &typeName, 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_DATA_TYPE_COLUMN_TAG:
        {
            memcpy(in_data->GetBuffer(), &m_rowItr->m_dataType, sizeof(simba_int16));
            return false;
        }

        case DSI_COLUMN_SIZE_COLUMN_TAG:
        {
            memcpy(in_data->GetBuffer(), &m_rowItr->m_columnSize, sizeof(simba_int32));
            return false;
        }

        case DSI_LITERAL_PREFIX_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                &m_rowItr->m_literalPrefix, 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_LITERAL_SUFFIX_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                &m_rowItr->m_literalSuffix, 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_CREATE_PARAM_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                &m_rowItr->m_createParams, 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_NULLABLE_COLUMN_TAG:
        {
            memcpy(in_data->GetBuffer(), &m_rowItr->m_nullable, sizeof(simba_int16));
            return false;
        }

        case DSI_CASE_SENSITIVE_COLUMN_TAG:
        {
            memcpy(in_data->GetBuffer(), &m_rowItr->m_caseSensitive, sizeof(simba_int16));
            return false;
        }

        case DSI_SEARCHABLE_COLUMN_TAG:
        {
            memcpy(in_data->GetBuffer(), &m_rowItr->m_searchable, sizeof(simba_int16));
            return false;
        }

        case DSI_UNSIGNED_ATTRIBUTE_COLUMN_TAG:
        {
            SqlDataTypeUtilities* typeUtilities = SqlDataTypeUtilitiesSingleton::GetInstance();

            if ((!typeUtilities->IsApproximateNumericType(m_rowItr->m_dataType) &&
                 !typeUtilities->IsExactNumericType(m_rowItr->m_dataType) &&
                 !typeUtilities->IsIntegerType(m_rowItr->m_dataType)) ||
                 (SQL_NULL_DATA == m_rowItr->m_unsignedAttr))
            {
                in_data->SetNull(true);
            }
            else
            {
                memcpy(in_data->GetBuffer(), &m_rowItr->m_unsignedAttr, sizeof(simba_int16));
            }

            return false;
        }

        case DSI_FIXED_PREC_SCALE_COLUMN_TAG:
        {
            memcpy(in_data->GetBuffer(), &m_rowItr->m_fixedPrecScale, sizeof(simba_int16));
            return false;
        }

        case DSI_AUTO_UNIQUE_COLUMN_TAG:
        {
            SqlDataTypeUtilities* typeUtilities = SqlDataTypeUtilitiesSingleton::GetInstance();

            if ((!typeUtilities->IsApproximateNumericType(m_rowItr->m_dataType) &&
                 !typeUtilities->IsExactNumericType(m_rowItr->m_dataType) &&
                 !typeUtilities->IsIntegerType(m_rowItr->m_dataType)) ||
                 (SQL_NULL_DATA == m_rowItr->m_autoUnique))
            {
                in_data->SetNull(true);
            }
            else
            {
                memcpy(in_data->GetBuffer(), &m_rowItr->m_autoUnique, sizeof(simba_int16));
            }

            return false;
        }

        case DSI_LOCAL_TYPE_NAME_COLUMN_TAG:
        {
            // Remove the "SQL_" part of the type name when retrieving it.
            SqlDataTypeUtilities* typeUtilities = SqlDataTypeUtilitiesSingleton::GetInstance();
            simba_wstring typeName(
                typeUtilities->GetStringForSqlType(m_rowItr->m_dataType).Substr(4));

            return DSITypeUtilities::OutputWVarCharStringData(
                &typeName, 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_MINIMUM_SCALE_COLUMN_TAG:
        {
            SqlDataTypeUtilities* typeUtilities = SqlDataTypeUtilitiesSingleton::GetInstance();

            if (typeUtilities->IsExactNumericType(m_rowItr->m_dataType) ||
                typeUtilities->IsIntegerType(m_rowItr->m_dataType))
            {
                memcpy(in_data->GetBuffer(), &m_rowItr->m_minScale, sizeof(simba_int16));
            }
            else if ((SQL_TYPE_TIME == m_rowItr->m_dataType) ||
                     (SQL_TYPE_TIMESTAMP == m_rowItr->m_dataType) ||
                     (SQL_TIME == m_rowItr->m_dataType) ||
                     (SQL_TIMESTAMP == m_rowItr->m_dataType))
            {
                // The datetime types except DATE can have scales for their fractional components.
                memcpy(in_data->GetBuffer(), &m_rowItr->m_minScale, sizeof(simba_int16));
            }
            else
            {
                in_data->SetNull(true);
            }

            return false;
        }

        case DSI_MAXIMUM_SCALE_COLUMN_TAG:
        {
            SqlDataTypeUtilities* typeUtilities = SqlDataTypeUtilitiesSingleton::GetInstance();

            if (typeUtilities->IsExactNumericType(m_rowItr->m_dataType) ||
                typeUtilities->IsIntegerType(m_rowItr->m_dataType))
            {
                memcpy(in_data->GetBuffer(), &m_rowItr->m_maxScale, sizeof(simba_int16));
            }
            else if ((SQL_TYPE_TIME == m_rowItr->m_dataType) ||
                     (SQL_TYPE_TIMESTAMP == m_rowItr->m_dataType) ||
                     (SQL_TIME == m_rowItr->m_dataType) ||
                     (SQL_TIMESTAMP == m_rowItr->m_dataType))
            {
                // The datetime types except DATE can have scales for their fractional components.
                memcpy(in_data->GetBuffer(), &m_rowItr->m_maxScale, sizeof(simba_int16));
            }
            else
            {
                in_data->SetNull(true);
            }

            return false;
        }

        case DSI_SQL_DATA_TYPE_COLUMN_TAG:
        {
            SqlDataTypeUtilities* typeUtilities = SqlDataTypeUtilitiesSingleton::GetInstance();

            simba_int16 verboseType = 
                typeUtilities->GetVerboseTypeFromConciseType(m_rowItr->m_dataType);
            memcpy(in_data->GetBuffer(), &verboseType, sizeof(simba_int16));
            return false;
        }

        case DSI_SQL_DATETIME_SUB_COLUMN_TAG:
        {
            SqlDataTypeUtilities* typeUtilities = SqlDataTypeUtilitiesSingleton::GetInstance();

            simba_int16 subType = 
                typeUtilities->GetIntervalCodeFromConciseType(m_rowItr->m_dataType);

            if (0 == subType)
            {
                in_data->SetNull(true);
            }
            else
            {
                memcpy(in_data->GetBuffer(), &subType, sizeof(simba_int16));
            }

            return false;
        }

        case DSI_NUM_PREC_RADIX_COLUMN_TAG:
        {
            SqlDataTypeUtilities* typeUtilities = SqlDataTypeUtilitiesSingleton::GetInstance();

            // See http://msdn.microsoft.com/en-us/library/ms714632%28d=lightweight,v=VS.85%29.aspx
            if (typeUtilities->IsApproximateNumericType(m_rowItr->m_dataType) ||
                typeUtilities->IsIntegerType(m_rowItr->m_dataType))
            {
                simba_int32 numPrecRadix = 2;
                memcpy(in_data->GetBuffer(), &numPrecRadix, sizeof(simba_int32));
            }
            else if (typeUtilities->IsExactNumericType(m_rowItr->m_dataType))
            {
                simba_int32 numPrecRadix = 10;
                memcpy(in_data->GetBuffer(), &numPrecRadix, sizeof(simba_int32));
            }
            else
            {
                in_data->SetNull(true);
            }

            return false;
        }

        case DSI_INTERVAL_PRECISION_COLUMN_TAG:
        {
            if (SQL_NULL_DATA == m_rowItr->m_intervalPrecision)
            {
                in_data->SetNull(true);
            }
            else
            {
                memcpy(in_data->GetBuffer(), &m_rowItr->m_intervalPrecision, sizeof(simba_int16));
            }

            return false;
        }

        case DSI_USER_DATA_TYPE_COLUMN_TAG:
        {
            // Custom column for custom user data types. Return UDT_STANDARD_SQL_TYPE always 
            // since RyftOne does not support any custom user data types.
            memcpy(in_data->GetBuffer(), &UDT_STANDARD_SQL_TYPE, sizeof(simba_uint16));

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
bool R1TypeInfoMetadataSource::Move(DSIDirection in_direction, simba_signed_native in_offset)
{
    UNUSED(in_offset);

    if (DSI_DIR_NEXT != in_direction)
    {
        R1THROWGEN(L"R1ResultSetTraverseDirNotSupported");
    }

    if (!m_hasStartedFetch)
    {
        m_hasStartedFetch = true;
        m_rowItr = m_dataTypes.begin();
    }
    else
    {
        ++m_rowItr;
    }

    return (m_rowItr < m_dataTypes.end());
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void R1TypeInfoMetadataSource::InitializeData(bool in_isODBCV3)
{
    m_dataTypes.reserve(25);
    SQLTypeInfo typeInfo;

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_WLONGVARCHAR;
    typeInfo.m_columnSize = 65500;
    typeInfo.m_literalPrefix = L"\'";
    typeInfo.m_literalSuffix = L"\'";
    typeInfo.m_createParams = L"LENGTH";
    typeInfo.m_caseSensitive = SQL_TRUE;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_WVARCHAR;
    typeInfo.m_columnSize = 510;
    typeInfo.m_literalPrefix = L"\'";
    typeInfo.m_literalSuffix = L"\'";
    typeInfo.m_createParams = L"max length";
    typeInfo.m_caseSensitive = SQL_TRUE;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_WCHAR;
    typeInfo.m_columnSize = 255;
    typeInfo.m_literalPrefix = L"\'";
    typeInfo.m_literalSuffix = L"\'";
    typeInfo.m_createParams = L"LENGTH";
    typeInfo.m_caseSensitive = SQL_TRUE;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_BIT;
    typeInfo.m_columnSize = BIT_COLUMN_SIZE;
    typeInfo.m_searchable = SQL_PRED_BASIC;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_TINYINT;
    typeInfo.m_columnSize = TINYINT_COLUMN_SIZE;
    typeInfo.m_searchable = SQL_PRED_BASIC;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_SMALLINT;
    typeInfo.m_columnSize = SMALLINT_COLUMN_SIZE;
    typeInfo.m_searchable = SQL_PRED_BASIC;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_INTEGER;
    typeInfo.m_columnSize = INTEGER_COLUMN_SIZE;
    typeInfo.m_searchable = SQL_PRED_BASIC;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_BIGINT;
    typeInfo.m_columnSize = SBIGINT_COLUMN_SIZE;
    typeInfo.m_searchable = SQL_PRED_BASIC;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_LONGVARBINARY;
    typeInfo.m_columnSize = 2147483000;
    typeInfo.m_searchable = SQL_PRED_NONE;
    typeInfo.m_literalPrefix = L"0x";
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_VARBINARY;
    typeInfo.m_columnSize = 32767;
    typeInfo.m_literalPrefix = L"0x";
    typeInfo.m_createParams = L"max length";
    typeInfo.m_searchable = SQL_PRED_NONE;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_BINARY;
    typeInfo.m_columnSize = 1024;
    typeInfo.m_literalPrefix = L"0x";
    typeInfo.m_createParams = L"LENGTH";
    typeInfo.m_searchable = SQL_PRED_NONE;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_LONGVARCHAR;
    typeInfo.m_columnSize = 65500;
    typeInfo.m_literalPrefix = L"\'";
    typeInfo.m_literalSuffix = L"\'";
    typeInfo.m_createParams = L"LENGTH";
    typeInfo.m_caseSensitive = SQL_TRUE;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_CHAR;
    typeInfo.m_columnSize = 255;
    typeInfo.m_literalPrefix = L"\'";
    typeInfo.m_literalSuffix = L"\'";
    typeInfo.m_createParams = L"LENGTH";
    typeInfo.m_caseSensitive = SQL_TRUE;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_NUMERIC;
    typeInfo.m_columnSize = 38;
    typeInfo.m_createParams = L"precision,scale";
    typeInfo.m_searchable = SQL_PRED_BASIC;
    typeInfo.m_maxScale = 38;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_DECIMAL;
    typeInfo.m_columnSize = 38;
    typeInfo.m_createParams = L"precision,scale";
    typeInfo.m_searchable = SQL_PRED_BASIC;
    typeInfo.m_maxScale = 38;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_FLOAT;
    typeInfo.m_columnSize = DOUBLE_FLOAT_COLUMN_SIZE;
    typeInfo.m_searchable = SQL_PRED_BASIC;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_REAL;
    typeInfo.m_columnSize = REAL_COLUMN_SIZE;
    typeInfo.m_searchable = SQL_PRED_BASIC;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_DOUBLE;
    typeInfo.m_columnSize = DOUBLE_FLOAT_COLUMN_SIZE;
    typeInfo.m_searchable = SQL_PRED_BASIC;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    typeInfo.m_dataType = SQL_VARCHAR;
    typeInfo.m_columnSize = 510;
    typeInfo.m_literalPrefix = L"\'";
    typeInfo.m_literalSuffix = L"\'";
    typeInfo.m_createParams = L"max length";
    typeInfo.m_caseSensitive = SQL_TRUE;
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    if (in_isODBCV3)
    {
        typeInfo.m_dataType = SQL_TYPE_DATE;
    }
    else
    {
        typeInfo.m_dataType = SQL_DATE;
    }
    typeInfo.m_columnSize = 10;
    typeInfo.m_literalPrefix = L"'";
    typeInfo.m_literalSuffix = L"'";
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    if (in_isODBCV3)
    {
        typeInfo.m_dataType = SQL_TYPE_TIME;
    }
    else
    {
        typeInfo.m_dataType = SQL_TIME;
    }
    typeInfo.m_columnSize = 8;
    typeInfo.m_literalPrefix = L"'";
    typeInfo.m_literalSuffix = L"'";
    m_dataTypes.push_back(typeInfo);

    typeInfo.Reset();
    if (in_isODBCV3)
    {
        typeInfo.m_dataType = SQL_TYPE_TIMESTAMP;
    }
    else
    {
        typeInfo.m_dataType = SQL_TIMESTAMP;
    }
    typeInfo.m_columnSize = 29;
    typeInfo.m_literalPrefix = L"'";
    typeInfo.m_literalSuffix = L"'";
    m_dataTypes.push_back(typeInfo);
}
