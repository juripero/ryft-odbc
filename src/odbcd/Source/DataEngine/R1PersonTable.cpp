// =================================================================================================
///  @file R1PersonTable.cpp
///
///  Implementation of the class R1PersonTable
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1PersonTable.h"

#include "BadColumnException.h"
#include "DSIColumnMetadata.h"
#include "DSIResultSetColumn.h"
#include "DSITypeUtilities.h"
#include "ILogger.h"
#include "NumberConverter.h"
#include "SqlData.h"
#include "SqlTypeMetadata.h"
#include "SqlTypeMetadataFactory.h"

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1PersonTable::R1PersonTable(ILogger* in_log) : DSISimpleResultSet(), m_log(in_log)
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1PersonTable", "R1PersonTable");

#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #13: Implement your DSISimpleResultSet.")
    // Your implementation of DSISimpleResultSet will simply maintain a handle to the cursor created
    // on your SQL-enabled data source. This class will delegate MoveToNextRow() calls to your data 
    // source and will retrieve the next row of data at that time.  In this example the data is hard 
    // coded to return 7 rows of data, thus InitializeData() is placed here.
    InitializeColumns();
    InitializeData();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1PersonTable::~R1PersonTable()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
simba_unsigned_native R1PersonTable::GetRowCount()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1PersonTable", "GetRowCount");
    return PERSON_ROW_COUNT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IColumns* R1PersonTable::GetSelectColumns()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1PersonTable", "GetSelectColumns");
    return &m_columns;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1PersonTable::HasRowCount()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1PersonTable", "HasRowCount");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1PersonTable::RetrieveData(
    simba_uint16 in_column,
    SqlData* in_data,
    simba_signed_native in_offset,
    simba_signed_native in_maxSize)
{
    DEBUG_ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1PersonTable", "RetrieveData");

    switch (in_column)
    {
        case 0:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                &m_rowItr->column1, 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case 1:
        {
            *reinterpret_cast<simba_int32*>(in_data->GetBuffer()) = m_rowItr->column2;
            return false;
        }

        case 2:
        {
            memcpy(in_data->GetBuffer(), &m_rowItr->column3, sizeof(TDWExactNumericType));
            return false;
        }
        
        default:
        {
            // This shouldn't happen as this will be caught in the ODBC layer, however for safety
            // put an exception here.
            DSITHROWEX1(
                BadColumnException, 
                L"InvalidColumnNum", 
                NumberConverter::ConvertInt32ToWString(in_column));
        }
    }
}

// Protected =======================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void R1PersonTable::DoCloseCursor()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1PersonTable", "DoCloseCursor");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1PersonTable::MoveToNextRow()
{
    DEBUG_ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1PersonTable", "MoveToNextRow");

    if (0 == GetCurrentRow())
    {
        m_rowItr = m_data.begin();
    }
    else
    {
        ++m_rowItr;
    }

    return (m_rowItr < m_data.end());
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void R1PersonTable::InitializeColumns()
{
    // Set up column metadata and type info and add new column (#1)
    DSIColumnMetadata* colMetadata = new DSIColumnMetadata;
    colMetadata->m_autoUnique = false;
    colMetadata->m_caseSensitive = false;
    colMetadata->m_label = L"Name";
    colMetadata->m_name = L"Name";
    colMetadata->m_tableName = R1_TABLE;
    colMetadata->m_schemaName = R1_SCHEMA;
    colMetadata->m_catalogName = R1_CATALOG;
    colMetadata->m_unnamed = false;
    colMetadata->m_charOrBinarySize = 100;
    colMetadata->m_nullable = DSI_NULLABLE;
    colMetadata->m_searchable = DSI_SEARCHABLE;
    colMetadata->m_updatable = DSI_READWRITE_UNKNOWN;

    DSIResultSetColumn* column = new DSIResultSetColumn(
        SqlTypeMetadataFactorySingleton::GetInstance()->CreateNewSqlTypeMetadata(SQL_WVARCHAR), 
        colMetadata);
    column->GetMetadata()->SetLengthOrIntervalPrecision(colMetadata->m_charOrBinarySize);
    m_columns.AddColumn(column);
    
    // Set up column metadata and type info and add new column (#2)
    colMetadata = new DSIColumnMetadata;
    colMetadata->m_autoUnique = false;
    colMetadata->m_caseSensitive = false;
    colMetadata->m_label = L"Integer";
    colMetadata->m_name = L"Integer";
    colMetadata->m_tableName = R1_TABLE;
    colMetadata->m_schemaName = R1_SCHEMA;
    colMetadata->m_catalogName = R1_CATALOG;
    colMetadata->m_unnamed = false;
    colMetadata->m_nullable = DSI_NULLABLE;
    colMetadata->m_searchable = DSI_PRED_BASIC;
    colMetadata->m_updatable = DSI_READWRITE_UNKNOWN;

    column = new DSIResultSetColumn(
        SqlTypeMetadataFactorySingleton::GetInstance()->CreateNewSqlTypeMetadata(SQL_INTEGER, true), 
        colMetadata);
    m_columns.AddColumn(column);

    // Set up column metadata and type info and add new column (#3)
    colMetadata = new DSIColumnMetadata;
    colMetadata->m_autoUnique = false;
    colMetadata->m_caseSensitive = false;
    colMetadata->m_label = L"Numeric";
    colMetadata->m_name = L"Numeric";
    colMetadata->m_tableName = R1_TABLE;
    colMetadata->m_schemaName = R1_SCHEMA;
    colMetadata->m_catalogName = R1_CATALOG;
    colMetadata->m_unnamed = false;
    colMetadata->m_nullable = DSI_NULLABLE;
    colMetadata->m_searchable = DSI_PRED_BASIC;
    colMetadata->m_updatable = DSI_READWRITE_UNKNOWN;

    SqlTypeMetadata* sqlMeta = 
        SqlTypeMetadataFactorySingleton::GetInstance()->CreateNewSqlTypeMetadata(SQL_NUMERIC);
    sqlMeta->SetScale(1);
    sqlMeta->SetPrecision(4);

    column = new DSIResultSetColumn(sqlMeta, colMetadata);
    m_columns.AddColumn(column);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1PersonTable::InitializeData()
{
    // Initialize data for each of the columns for all of the rows.
    m_data.resize(PERSON_ROW_COUNT);

    // Set the varchar column.
    m_data[0].column1 = "Amyn";
    m_data[1].column1 = "Hesam";
    m_data[2].column1 = "Jerry";
    m_data[3].column1 = "Kyle";
    m_data[4].column1 = "Marie";
    m_data[5].column1 = "Sylvia";
    m_data[6].column1 = "Trevor";

    for (simba_uint32 i = 0; i < PERSON_ROW_COUNT; ++i)
    {
        // Set the integer column.
        m_data[i].column2 = i;

        // Set the numeric column.
        m_data[i].column3.Overflow = false;
        m_data[i].column3.Scale = -1;
        m_data[i].column3.Size = 1 + 1; // 1 word of data + the sign word.

        memset(m_data[i].column3.Word, 0, MAX_NUMERIC_UNSCALED_VALUE_SIZE);

        // Generate data for the numeric column.
        m_data[i].column3.Word[0] = static_cast<simba_uint16> (1001 + (10 * i));
        m_data[i].column3.Word[1] = 0; // positive.
    }
}
