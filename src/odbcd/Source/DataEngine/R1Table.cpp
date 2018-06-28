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
    m_result = m_ryft1->OpenTable(tableName);

    InitializeColumns(in_isODBCV3);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::AppendFilter(simba_wstring& in_filter)
{
    m_result->AppendFilter(in_filter.GetAsPlatformString());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::AppendRow( )
{
    m_hasInsertedRecords = true;
    m_isAppendingRow = true;
    m_result->PrepareUpdate();
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
    if (m_isAppendingRow) {
        m_isAppendingRow = false;
        m_result->FinishUpdate();
    }
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

    simba_int16 sqlType = in_data->GetMetadata()->GetTDWType();

    if (m_result->IsNull(in_column)) {
        in_data->SetNull(true);
        return false;
    }

    try {
        switch (sqlType) {
        case TDW_SQL_VARCHAR: {
            const char *colResult = m_result->GetStringValue(in_column);
            if(!colResult) {
                in_data->SetNull(true);
                return false;
            }
            return DSITypeUtilities::OutputVarCharStringData(
			    colResult,
                strlen(colResult),
			    in_data, 
			    in_offset, 
			    in_maxSize);
            }
        case TDW_SQL_SINTEGER: {
		    *reinterpret_cast<simba_int32*>(in_data->GetBuffer()) = m_result->GetIntValue(in_column);
		    return false;
            }
        case TDW_SQL_SBIGINT: {
		    *reinterpret_cast<simba_int64*>(in_data->GetBuffer()) = m_result->GetInt64Value(in_column);
		    return false;
            }
	    case TDW_SQL_DOUBLE: {
		    *reinterpret_cast<simba_double64*>(in_data->GetBuffer()) = m_result->GetDoubleValue(in_column);
            return false;
		    }
        case TDW_SQL_TYPE_DATE: {
            struct tm date = m_result->GetDateValue(in_column);
            TDWDate sqlDate(date.tm_year, date.tm_mon, date.tm_mday);
            memcpy(in_data->GetBuffer(), &sqlDate, sizeof(TDWDate));
            return false;
            }
        case TDW_SQL_TYPE_TIME: {
            struct tm time = m_result->GetTimeValue(in_column);
            TDWTime sqlTime(time.tm_hour, time.tm_min, time.tm_sec, 0);
            memcpy(in_data->GetBuffer(), &sqlTime, sizeof(TDWTime));
            return false;
            }
        case TDW_SQL_TYPE_TIMESTAMP: {
            struct tm ts = m_result->GetDateTimeValue(in_column);
            TDWTimestamp sqlTimestamp(ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec, 0);
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
        m_result->PutStringValue(in_column, "");
        return false;
    }

    simba_int16 sqlType = in_data->GetMetadata()->GetTDWType();
    size_t colSize = GetSelectColumns()->GetColumn(in_column)->GetColumnSize();
    char *out_buf = new char[colSize + 1];
    if(!out_buf) {
        m_result->PutStringValue(in_column, "");
        return false;
    }

    switch (sqlType) {
    case TDW_SQL_VARCHAR:
        strncpy(out_buf, reinterpret_cast<char *>(in_data->GetBuffer()), colSize);
        out_buf[in_data->GetLength()] = '\0';
        m_result->PutStringValue(in_column, out_buf);
        break;
    case TDW_SQL_SINTEGER:
        snprintf(out_buf, colSize+1, "%d", (*reinterpret_cast<simba_int32*> (in_data->GetBuffer())));
        m_result->PutStringValue(in_column, out_buf);
        break;
    case TDW_SQL_SBIGINT:
        snprintf(out_buf, colSize+1, "%lld", (*reinterpret_cast<simba_int64*> (in_data->GetBuffer())));
        m_result->PutStringValue(in_column, out_buf);
        break;
    case TDW_SQL_DOUBLE:
        snprintf(out_buf, colSize+1, "%f", (*reinterpret_cast<simba_double64*> (in_data->GetBuffer())));
        m_result->PutStringValue(in_column, out_buf);
        break;
    case TDW_SQL_TYPE_DATE: {
        TDWDate tdwDate = (*reinterpret_cast<TDWDate *> (in_data->GetBuffer()));
        struct tm date;
        memset(&date, 0, sizeof(tm));
        date.tm_year = tdwDate.Year;
        date.tm_mon = tdwDate.Month;
        date.tm_mday = tdwDate.Day;
        m_result->PutDateValue(in_column, &date);
        break;
        }
    case TDW_SQL_TYPE_TIME: {
        TDWTime tdwTime = (*reinterpret_cast<TDWTime *> (in_data->GetBuffer()));
        struct tm time;
        memset(&time, 0, sizeof(tm));
        time.tm_hour = tdwTime.Hour;
        time.tm_min = tdwTime.Minute;
        time.tm_sec = tdwTime.Second;
        m_result->PutTimeValue(in_column, &time);
        break;
        }
    case TDW_SQL_TYPE_TIMESTAMP: {
        TDWTimestamp tdwTimestamp = (*reinterpret_cast<TDWTimestamp *> (in_data->GetBuffer()));
        struct tm timestamp;
        memset(&timestamp, 0, sizeof(tm));
        timestamp.tm_year = tdwTimestamp.Year;
        timestamp.tm_mon = tdwTimestamp.Month;
        timestamp.tm_mday = tdwTimestamp.Day;
        timestamp.tm_hour = tdwTimestamp.Hour;
        timestamp.tm_min = tdwTimestamp.Minute;
        timestamp.tm_sec = tdwTimestamp.Second;
        m_result->PutDateTimeValue(in_column, &timestamp);
        break;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::GetTypeFormatSpecifier(
    simba_uint16 in_column,
    unsigned *out_dtType,
    string& out_formatSpec)
{
    m_result->GetTypeFormatSpecifier(in_column, out_dtType, out_formatSpec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1Table::IsStructuredType()
{
    return m_result->IsStructuredType();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1Table::IsPCAPDatabase()
{
    return m_result->IsPCAPDatabase();
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
        m_result->FlushUpdate();
    }
    m_result->CloseCursor();
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
        isMoveSuccessful = m_result->FetchNext();
    }
    else {
        m_hasStartedFetch = true;
        isMoveSuccessful = m_result->FetchFirst();
    }
    return isMoveSuccessful;
}

// Helpers =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Converts between ODBC 2.X/3.X date/time types as needed.
///
/// @param in_type      The type to be (potentially) converted.
/// @param in_isODBCV3  Whether ODBC 3.X is in use.
///
/// @return The resulting type.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline simba_int16 ConvertTypeIfNeeded(simba_int16 in_type, bool in_isODBCV3)
{
    if (in_isODBCV3) {
        // Convert the ODBC 2x datetime types to 3x types.
        switch (in_type)  {
        case SQL_DATE:
            return SQL_TYPE_DATE;

        case SQL_TIME:
            return SQL_TYPE_TIME;

        case SQL_TIMESTAMP:
            return SQL_TYPE_TIMESTAMP;

        default:
            return in_type;
        }
    }
    else {
        // Convert the ODBC 3x datetime types to 2x types.
        switch (in_type) {
        case SQL_TYPE_DATE:
            return SQL_DATE;

        case SQL_TYPE_TIME:
            return SQL_TIME;

        case SQL_TYPE_TIMESTAMP:
            return SQL_TIMESTAMP;

        default:
            return in_type;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Table::InitializeColumns(bool in_isODBCV3)
{
    string tableName = m_tableName.GetAsPlatformString();
    RyftOne_Columns ryft1cols = m_ryft1->GetColumns(tableName);
    RyftOne_Columns::iterator ryft1col;

    AutoPtr<DSIResultSetColumns> columns(new DSIResultSetColumns());

    for (ryft1col = ryft1cols.begin(); ryft1col != ryft1cols.end(); ryft1col++) {

        AutoPtr<DSIColumnMetadata> columnMetadata(new DSIColumnMetadata());

        columnMetadata->m_catalogName = R1_CATALOG;
        columnMetadata->m_schemaName.Clear();
        columnMetadata->m_tableName = ryft1col->m_tableName;
        columnMetadata->m_name = ryft1col->m_colName;
        columnMetadata->m_label = ryft1col->m_colAlias;
        columnMetadata->m_unnamed = false;
        columnMetadata->m_charOrBinarySize = ryft1col->m_charCols;
        columnMetadata->m_nullable = DSI_NULLABLE;

        assert(!columnMetadata->m_name.IsNull());
        assert(!columnMetadata->m_tableName.IsNull());

        simba_int16 type = ConvertTypeIfNeeded(ryft1col->m_dataType, in_isODBCV3);

        SqlTypeMetadata* sqlTypeMetadata = 
            SqlTypeMetadataFactorySingleton::GetInstance()->CreateNewSqlTypeMetadata(type);

        if (sqlTypeMetadata->IsCharacterOrBinaryType()) 
            sqlTypeMetadata->SetLengthOrIntervalPrecision(ryft1col->m_charCols);

        // timwells(04/06/17) - don't set precision since we don't support fractional seconds
        // TIMESTAMP_WITH_FRAC_PREC_DISPLAY_SIZE is 20, 19 for a timestamp with no seconds precision and +1 for the decimal point. 
        // So, set the precision is the length minus TIMESTAMP_WITH_FRAC_PREC_DISPLAY_SIZE.
        // if ((TDW_SQL_TYPE_TIMESTAMP == sqlTypeMetadata->GetTDWType()) && (TIMESTAMP_WITH_FRAC_PREC_DISPLAY_SIZE < ryft1col->m_charCols)) 
        //     sqlTypeMetadata->SetPrecision(static_cast<simba_int16>(ryft1col->m_bufLength - TIMESTAMP_WITH_FRAC_PREC_DISPLAY_SIZE));

        columns->AddColumn(new DSIResultSetColumn(sqlTypeMetadata, columnMetadata.Detach()));
    }

    m_columns.Attach(columns.Detach());
}


