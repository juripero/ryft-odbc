// =================================================================================================
///  @file R1CatalogOnlyMetadataSource.cpp
///
///  Implementation of the class R1CatalogOnlyMetadataSource
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1CatalogOnlyMetadataSource.h"

#include "BadColumnException.h"
#include "DSITypeUtilities.h"
#include "SqlData.h"

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1CatalogOnlyMetadataSource::R1CatalogOnlyMetadataSource(DSIMetadataRestrictions& in_restrictions) :
    DSIMetadataSource(in_restrictions),
    m_hasStartedFetch(false)
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1CatalogOnlyMetadataSource::~R1CatalogOnlyMetadataSource()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1CatalogOnlyMetadataSource::CloseCursor()
{
    m_hasStartedFetch = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1CatalogOnlyMetadataSource::GetMetadata(
    DSIOutputMetadataColumnTag in_columnTag, 
    SqlData* in_data, 
    simba_signed_native in_offset,
    simba_signed_native in_maxSize)
{
    switch (in_columnTag)
    {
        case DSI_CATALOG_NAME_COLUMN_TAG:
        {
            return DSITypeUtilities::OutputWVarCharStringData(
                &R1_CATALOG, 
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
bool R1CatalogOnlyMetadataSource::Move(DSIDirection in_direction, simba_signed_native in_offset)
{
    if (DSI_DIR_NEXT != in_direction)
    {
        R1THROWGEN(L"R1ResultSetTraverseDirNotSupported");
    }

    if (!m_hasStartedFetch)
    {
        m_hasStartedFetch = true;
        return true;
    }

    return false;
}
