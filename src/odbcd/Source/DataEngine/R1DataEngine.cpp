// =================================================================================================
///  @file R1DataEngine.cpp
///
///  Implementation of the Class R1DataEngine
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1DataEngine.h"

#include "R1Table.h"
#include "DSIEmptyMetadataSource.h"
#include "DSITableTypeOnlyMetadataSource.h"
#include "DSIExtResultSet.h"
#include "DSIResults.h"

#include "R1OperationHandlerFactory.h"
#include "R1CatalogOnlyMetadataSource.h"
#include "R1ColumnsMetadataSource.h"
#include "R1SchemaOnlyMetadataSource.h"
#include "R1TablesMetadataSource.h"
#include "R1TypeInfoMetadataSource.h"

using namespace RyftOne;
using namespace Simba::DSI;
using namespace Simba::SQLEngine;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1DataEngine::R1DataEngine(IStatement* in_statement, RyftOne_Database *ryft1) : 
    DSIExtSqlDataEngine(in_statement), m_ryft1(ryft1)
{
    ENTRANCE_LOG(GetLog(), "RyftOne", "R1DataEngine", "R1DataEngine");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1DataEngine::~R1DataEngine()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AutoPtr<DSIExtOperationHandlerFactory> R1DataEngine::CreateOperationHandlerFactory()
{
    ENTRANCE_LOG(
        GetLog(), 
        "RyftOne", 
        "R1DataEngine", 
        "CreateOperationHandlerFactory");

    //return AutoPtr<DSIExtOperationHandlerFactory>(NULL);
    return AutoPtr<DSIExtOperationHandlerFactory>(new R1OperationHandlerFactory());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DSIMetadataSource* R1DataEngine::MakeNewMetadataTable(
    Simba::DSI::DSIMetadataTableID in_metadataTableID, 
    Simba::DSI::DSIMetadataRestrictions& in_restrictions,
    const simba_wstring& in_escapeChar, 
    const simba_wstring& in_identifierQuoteChar, 
    bool in_filterAsIdentifier)
{
    ENTRANCE_LOG(GetLog(), "RyftOne", "R1DataEngine", "MakeNewMetadataTable");

#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #8: Create and return your Metadata Sources.")
    // The metadata sources return the catalog metadata about your data-source. They correspond to
    // the catalog functions for the various APIs, for instance SQLColumns in ODBC would result in
    // an DSI_COLUMNS_METADATA source being created to return the data. At the very least, ODBC 
    // conforming applications will require the following metadata sources:
    //
    //  DSI_TABLES_METADATA
    //      List of all tables defined in the data source.
    //
    //  DSI_CATALOGONLY_METADATA
    //      List of all catalogs defined in the data source (If catalogs are supported).
    //
    //  DSI_SCHEMAONLY_METADATA
    //      List of all schemas defined in the data source (If schemas are supported).
    //
    //  DSI_TABLETYPEONLY_METADATA
    //      List of all table types (TABLE,VIEW,SYSTEM) defined within the data source.
    //
    //  DSI_COLUMNS_METADATA
    //      List of all columns defined across all tables in the data source.
    //
    //  DSI_TYPE_INFO_METADATA
    //      List of the types supported by the data source. This means the actual types that can be
    //      stored in the data source, not necessarily the types that can be returned by the driver.
    //      For instance, a conversion may result in a type being returned that is not stored in the
    //      data source.
    //
    //  In some cases applications may provide values to restrict the metadata sources.
    //  These restrictions are stored within in_restrictions and can be used to restrict
    //  the number of rows that are returned from the metadata source.

    // Columns, Tables, CatalogOnly, SchemaOnly, TableTypeOnly, TypeInfo.
    switch (in_metadataTableID)
    {
        case DSI_TABLES_METADATA:
        {
            return new R1TablesMetadataSource(in_restrictions, m_ryft1);
        }

        case DSI_CATALOGONLY_METADATA:
        {
            // Note that if your driver does not support catalogs and you have disabled support for
            // them in the driver, this case statement may be removed entirely.
            return new R1CatalogOnlyMetadataSource(in_restrictions);
        }

        case DSI_SCHEMAONLY_METADATA:
        {
            // Note that if your driver does not support schemas and you have disabled support for
            // them in the driver, this case statement may be removed entirely.
            return new R1SchemaOnlyMetadataSource(in_restrictions);
        }

        case DSI_TABLETYPEONLY_METADATA:
        {
            return new DSITableTypeOnlyMetadataSource(in_restrictions);
        }

        case DSI_COLUMNS_METADATA:
        {
            return new R1ColumnsMetadataSource(in_restrictions, m_ryft1);
        }

        case DSI_TYPE_INFO_METADATA:
        {
            return new R1TypeInfoMetadataSource(in_restrictions, IsODBCVersion3());
        }

        default:
        {
            return new DSIEmptyMetadataSource(in_restrictions);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DataEngine::CreateTable(const SharedPtr<TableSpecification> in_specification)
{
    assert(!in_specification.IsNull());
    RyftOne_Columns ryft_columns;
    RyftOne_Column ryft_column;
    simba_wstring simba_string;
    SqlTypeMetadata *colSqlMeta;

    simba_wstring in_tableName = in_specification->GetName();
    string tableName = in_tableName.GetAsPlatformString();
    IColumns *simba_columns = in_specification->GetColumns();
    for(simba_uint16 idx = 0; idx < simba_columns->GetColumnCount(); idx++)  {

        ryft_column.m_ordinal = idx;
        ryft_column.m_tableName = tableName;

        IColumn *simba_column = simba_columns->GetColumn(idx);

        simba_column->GetName(simba_string);
        ryft_column.m_colName = simba_string.GetAsPlatformString();

        colSqlMeta = simba_column->GetMetadata();
        ryft_column.m_typeName = colSqlMeta->GetTypeName().GetAsPlatformString();
        ryft_column.m_dataType = colSqlMeta->GetSqlType();
        ryft_column.m_bufferLen = colSqlMeta->GetLengthOrIntervalPrecision();
        ryft_column.m_colSize = colSqlMeta->GetColumnSize(ryft_column.m_bufferLen);
        ryft_columns.push_back(ryft_column);
    }

    m_ryft1->createTable(tableName, ryft_columns);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1DataEngine::DoesTableExist(
    const simba_wstring& in_catalogName,
    const simba_wstring& in_schemaName,
    const simba_wstring& in_tableName)
{
    string tableName = in_tableName.GetAsPlatformString();
    return m_ryft1->tableExists(tableName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DataEngine::DropTable(
    const simba_wstring& in_catalogName,
    const simba_wstring& in_schemaName,
    const simba_wstring& in_tableName,
    Simba::SQLEngine::DSIExtTableDropOption in_dropOption)
{
    string tableName = in_tableName.GetAsPlatformString();
    m_ryft1->dropTable(tableName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
SharedPtr<DSIExtResultSet> R1DataEngine::OpenTable(
    const simba_wstring& in_catalogName,
    const simba_wstring& in_schemaName,
    const simba_wstring& in_tableName,
    DSIExtTableOpenType in_openType)
{
    ENTRANCE_LOG(GetLog(), "RyftOne", "R1DataEngine", "OpenTable");

    SharedPtr<DSIExtResultSet> table;

    table = new R1Table(
        GetLog(),
        in_tableName,
        m_ryft1,
        m_statement->GetWarningListener(),
        IsODBCVersion3());

    return table;
}

