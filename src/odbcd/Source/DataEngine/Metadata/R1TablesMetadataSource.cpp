// =================================================================================================
///  @file R1TablesMetadataSource.cpp
///
///  Implementation of the class R1TablesMetadataSource
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1TablesMetadataSource.h"

#include "BadColumnException.h"
#include "DSITypeUtilities.h"
#include "SqlData.h"

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1TablesMetadataSource::R1TablesMetadataSource(DSIMetadataRestrictions& in_restrictions, RyftOne_Database *in_ryft1) :
    DSIMetadataSource(in_restrictions),
    m_hasStartedFetch(false)
{
    simba_wstring table;
    DSIMetadataRestrictions::const_iterator itr = in_restrictions.find(DSI_TABLE_NAME_COLUMN_TAG);
    if (itr != in_restrictions.end()) {
        table = itr->second;
    }
    else
        table = "%";

    simba_wstring type;
    itr = in_restrictions.find(DSI_TABLE_TYPE_COLUMN_TAG);
    if (itr != in_restrictions.end()) {
        type = itr->second; 
    }
    else
        type = "%";

    m_tables = in_ryft1->GetTables(table.GetAsPlatformString(), type.GetAsPlatformString());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1TablesMetadataSource::~R1TablesMetadataSource()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1TablesMetadataSource::CloseCursor()
{
    m_hasStartedFetch = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1TablesMetadataSource::GetMetadata(
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
                &R1_CATALOG, 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_SCHEMA_NAME_COLUMN_TAG:
        {
            // Note that if your driver does not support schemas, this case statement can simply
            // set NULL on in_data and return false.
            in_data->SetNull(true);
            return false;
        }

        case DSI_TABLE_NAME_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                simba_wstring::CreateFromUTF8(m_tablesItr->m_tableName.c_str()), 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_TABLE_TYPE_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                simba_wstring::CreateFromUTF8(m_tablesItr->m_type.c_str()), 
                in_data, 
                in_offset, 
                in_maxSize);
        }

        case DSI_REMARKS_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                simba_wstring::CreateFromUTF8(m_tablesItr->m_description.c_str()), 
                in_data, 
                in_offset, 
                in_maxSize);
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
bool R1TablesMetadataSource::Move(DSIDirection in_direction, simba_signed_native in_offset)
{
    if (DSI_DIR_NEXT != in_direction)
    {
        R1THROWGEN(L"R1ResultSetTraverseDirNotSupported");
    }

    if (!m_hasStartedFetch) {
        m_hasStartedFetch = true;
        m_tablesItr = m_tables.begin();
    }
    else
        m_tablesItr++;

    return (m_tables.end() != m_tablesItr);
}
