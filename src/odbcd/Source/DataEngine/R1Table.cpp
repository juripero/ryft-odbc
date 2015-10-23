//==================================================================================================
///  @file R1Table.cpp
///
///  Implementation of the class R1Table.
///
///  Copyright (C) 2008-2014 Simba Technologies Incorporated.
//==================================================================================================

#include "R1Table.h"

#include "DSITypeUtilities.h"
#include "ErrorException.h"
#include "IColumn.h"
#include "IWarningListener.h"
#include "NumberConverter.h"
#include "SEInvalidArgumentException.h"
#include "SqlData.h"
#include "SqlDataTypeUtilities.h"
#include "SqlTypeMetadata.h"
#include "SqlTypeMetadataFactory.h"
#include "DSIColumnMetadata.h"
#include "DSIResultSetColumn.h"
#include "DSIResultSetColumns.h"


using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1Table::R1Table(
    ILogger* in_log,
    const simba_wstring& in_tableName,
    RyftOne_Database *ryft1,
    IWarningListener* in_warningListener,
    bool in_isODBCV3) :
        m_log(in_log),
        m_tableName(in_tableName),
        m_ryft1(ryft1),
        m_warningListener(in_warningListener),
        m_hasStartedFetch(false),
        m_hasInsertedRecords(false),
        m_isAppendingRow(false)
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "R1Table");

    string tableName = m_tableName.GetAsPlatformString();
    m_result = m_ryft1->openTable(tableName);

    InitializeColumns(in_isODBCV3);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::AppendFilter(simba_wstring& in_filter, int in_hamming)
{
    m_result->appendFilter(in_filter.GetAsPlatformString(), in_hamming);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::AppendRow( )
{
    m_hasInsertedRecords = true;
    m_isAppendingRow = true;
    m_result->appendRow();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
simba_unsigned_native R1Table::GetRowCount()
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "GetRowCount");

    // Return ROW_COUNT_UNKNOWN if HasRowCount() returns false.
    return ROW_COUNT_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IColumns* R1Table::GetSelectColumns()
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "GetSelectColumns");
    return m_columns.Get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1Table::HasRowCount()
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "HasRowCount");
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::GetCatalogName(simba_wstring& out_catalogName)
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "GetCatalogName");
    out_catalogName = R1_CATALOG;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::GetSchemaName(simba_wstring& out_schemaName)
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "GetSchemaName");
    // QuickstartDSII does not support schemas.
    out_schemaName.Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::GetTableName(simba_wstring& out_tableName)
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "GetTableName");
    out_tableName = m_tableName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::OnFinishRowUpdate()
{
    if (m_isAppendingRow) 
        m_isAppendingRow = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1Table::RetrieveData(
    simba_uint16 in_column,
    Simba::Support::SqlData* in_data,
    simba_signed_native in_offset,
    simba_signed_native in_maxSize)
{
    DEBUG_ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "RetrieveData");
    assert(in_data);

    int colIdx = m_result->getColumnIndex(in_column);
    if(colIdx == -1) {
        in_data->SetNull(true);
        return false;
    }
    string colResult = m_result->getStringValue(colIdx);

    simba_int16 sqlType = in_data->GetMetadata()->GetSqlType();

    try {
        switch (sqlType) {
        case SQL_VARCHAR: {
            return DSITypeUtilities::OutputVarCharStringData(
			    colResult,
			    in_data, 
			    in_offset, 
			    in_maxSize);
            }
        case SQL_INTEGER: {
		    *reinterpret_cast<simba_int32*>(in_data->GetBuffer()) = NumberConverter::ConvertStringToInt32(colResult);
		    return false;
            }
        case SQL_BIGINT: {
		    *reinterpret_cast<simba_int64*>(in_data->GetBuffer()) = NumberConverter::ConvertStringToInt64(colResult);
		    return false;
            }
	    case SQL_DOUBLE: {
		    simba_double64 myDouble = NumberConverter::ConvertStringToDouble(colResult);
		    memcpy(in_data->GetBuffer(), &myDouble, sizeof(simba_double64));
            return false;
		    }
        case SQL_DATE: {
            int month, day, year;
            sscanf(colResult.c_str(), "%d-%d-%d", &year, &month, &day);
            TDWDate sqlDate(year, month, day);
            memcpy(in_data->GetBuffer(), &sqlDate, sizeof(TDWDate));
            return false;
            }
        case SQL_TIME: {
            int hour = 0, min = 0, sec = 0;
            int converted = sscanf(colResult.c_str(), "%d:%d:%d", &hour, &min, &sec);
            TDWTime sqlTime(hour, min, sec);
            memcpy(in_data->GetBuffer(), &sqlTime, sizeof(TDWTime));
            return false;
            }
        case SQL_TIMESTAMP: {
            int month = 0, day = 0, year = 0;
            int hour = 0, min = 0, sec = 0;
            int converted = sscanf(colResult.c_str(), "%d-%d-%d %d.%d.%d", &year, &month, &day, &hour, &min, &sec);
            TDWTimestamp sqlTimestamp(year, month, day, hour, min, sec, 0);
            memcpy(in_data->GetBuffer(), &sqlTimestamp, sizeof(TDWTimestamp));
            return false;
            }
        default:
            in_data->SetNull(true);
            return false;
        }
    }
    catch(...) {
        in_data->SetNull(true);
        return false;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1Table::WriteData(
    simba_uint16 in_column, 
    SqlData* in_data, 
    simba_signed_native in_offset,
    bool in_isDefault)
{
    if(!m_isAppendingRow)
        return false;

    if(in_data->IsNull()) {
        m_result->putStringValue(in_column, "");
        return false;
    }

    simba_int16 sqlType = in_data->GetMetadata()->GetSqlType();
    size_t colSize = GetSelectColumns()->GetColumn(in_column)->GetColumnSize();
    char *out_buf = new char[colSize + 1];
    if(!out_buf) {
        m_result->putStringValue(in_column, "");
        return false;
    }

    switch (sqlType) {
    case SQL_VARCHAR:
        strncpy(out_buf, reinterpret_cast<char *>(in_data->GetBuffer()), colSize);
        out_buf[colSize] = '\0';
        m_result->putStringValue(in_column, out_buf);
        break;
    case SQL_INTEGER:
        snprintf(out_buf, colSize+1, "%d", (*reinterpret_cast<simba_int32*> (in_data->GetBuffer())));
        m_result->putStringValue(in_column, out_buf);
        break;
    case SQL_BIGINT:
        snprintf(out_buf, colSize+1, "%lld", (*reinterpret_cast<simba_int64*> (in_data->GetBuffer())));
        m_result->putStringValue(in_column, out_buf);
        break;
    case SQL_DOUBLE:
        snprintf(out_buf, colSize+1, "%f", (*reinterpret_cast<simba_double64*> (in_data->GetBuffer())));
        m_result->putStringValue(in_column, out_buf);
        break;
    case SQL_DATE: {
        TDWDate date = (*reinterpret_cast<TDWDate *> (in_data->GetBuffer()));
        snprintf(out_buf, colSize+1, "%04d-%02d-%02d", date.Year, date.Month, date.Day);
        m_result->putStringValue(in_column, out_buf);
        break;
        }
    case SQL_TIME: {
        TDWTime time = (*reinterpret_cast<TDWTime *> (in_data->GetBuffer()));
        snprintf(out_buf, colSize+1, "%02d:%02d:%02d", time.Hour, time.Minute, time.Second);
        m_result->putStringValue(in_column, out_buf);
        break;
        }
    case SQL_TIMESTAMP: {
        TDWTimestamp timestamp = (*reinterpret_cast<TDWTimestamp *> (in_data->GetBuffer()));
        snprintf(out_buf, colSize+1, "%04d-%02d-%02d %02d.%02d.%02d", timestamp.Year, timestamp.Month, timestamp.Day,
            timestamp.Hour, timestamp.Minute, timestamp.Second);
        m_result->putStringValue(in_column, out_buf);
        break;
        }
    }
    return false;
}

// Protected =======================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1Table::~R1Table()
{
    delete m_result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::DoCloseCursor()
{
    ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "DoCloseCursor");
    if(m_hasInsertedRecords) {
        m_hasInsertedRecords = false;
        m_result->flush();
    }
    m_hasStartedFetch = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::MoveToBeforeFirstRow()
{
    DEBUG_ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "MoveToBeforeFirstRow");
    m_hasStartedFetch = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1Table::MoveToNextRow()
{
    DEBUG_ENTRANCE_LOG(m_log, "RyftOne", "R1Table", "MoveToNextRow");
    bool isMoveSuccessful = false;
    if(m_hasStartedFetch) {
        isMoveSuccessful = m_result->fetchNext();
    }
    else {
        m_hasStartedFetch = true;
        isMoveSuccessful = m_result->fetchFirst();
    }
    return isMoveSuccessful;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::InitializeColumns(bool in_isODBCV3)
{
    string tableName = m_tableName.GetAsPlatformString();
    RyftOne_Columns ryft1cols = m_ryft1->getColumns(tableName);
    RyftOne_Columns::iterator ryft1col;

    AutoPtr<DSIResultSetColumns> columns(new DSIResultSetColumns());

    for (ryft1col = ryft1cols.begin(); ryft1col != ryft1cols.end(); ryft1col++) {

        AutoPtr<DSIColumnMetadata> columnMetadata(new DSIColumnMetadata());

        columnMetadata->m_catalogName = R1_CATALOG;
        columnMetadata->m_schemaName.Clear();
        columnMetadata->m_tableName = ryft1col->m_tableName;
        columnMetadata->m_name = ryft1col->m_colName;
        columnMetadata->m_label = ryft1col->m_colTag;
        columnMetadata->m_unnamed = false;

        assert(!columnMetadata->m_name.IsNull());
        assert(!columnMetadata->m_tableName.IsNull());

        SqlTypeMetadata* sqlTypeMetadata = 
            SqlTypeMetadataFactorySingleton::GetInstance()->CreateNewSqlTypeMetadata(ryft1col->m_dataType);

        if (sqlTypeMetadata->IsCharacterOrBinaryType())
        {
            columnMetadata->m_charOrBinarySize = ryft1col->m_bufferLen;
        }
        else
        {
            columnMetadata->m_charOrBinarySize = 
                SqlDataTypeUtilitiesSingleton::GetInstance()->GetColumnSizeForSqlType(ryft1col->m_dataType);

            if (0 == columnMetadata->m_charOrBinarySize)
            {
                columnMetadata->m_charOrBinarySize = ryft1col->m_bufferLen;
            }
        }

        // Set to NULLABLE by default.
        columnMetadata->m_nullable = DSI_NULLABLE;

         // TIMESTAMP_WITH_FRAC_PREC_DISPLAY_SIZE is 20, 19 for a timestamp with no seconds precision
        // and +1 for the decimal point. So, the precision is the length minus 
        // TIMESTAMP_WITH_FRAC_PREC_DISPLAY_SIZE.
        if ((TDW_SQL_TYPE_TIMESTAMP == sqlTypeMetadata->GetTDWType()) && 
                 (TIMESTAMP_WITH_FRAC_PREC_DISPLAY_SIZE < ryft1col->m_bufferLen))
        {
            sqlTypeMetadata->SetPrecision(
                static_cast<simba_int16>(ryft1col->m_bufferLen - TIMESTAMP_WITH_FRAC_PREC_DISPLAY_SIZE));
        }

        sqlTypeMetadata->SetLengthOrIntervalPrecision(columnMetadata->m_charOrBinarySize);

        columns->AddColumn(new DSIResultSetColumn(sqlTypeMetadata, columnMetadata.Detach()));
    }

    m_columns.Attach(columns.Detach());
}


