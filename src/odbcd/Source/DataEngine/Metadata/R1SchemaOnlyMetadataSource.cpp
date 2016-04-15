// =================================================================================================
///  @file R1SchemaOnlyMetadataSource.cpp
///
///  Implementation of the class R1SchemaOnlyMetadataSource
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1SchemaOnlyMetadataSource.h"

#include "BadColumnException.h"
#include "DSITypeUtilities.h"
#include "SqlData.h"

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1SchemaOnlyMetadataSource::R1SchemaOnlyMetadataSource(DSIMetadataRestrictions& in_restrictions) :
    DSIMetadataSource(in_restrictions),
    m_hasStartedFetch(false)
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1SchemaOnlyMetadataSource::~R1SchemaOnlyMetadataSource()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1SchemaOnlyMetadataSource::CloseCursor()
{
    m_hasStartedFetch = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1SchemaOnlyMetadataSource::GetMetadata(
    DSIOutputMetadataColumnTag in_columnTag, 
    SqlData* in_data, 
    simba_signed_native in_offset,
    simba_signed_native in_maxSize)
{
    switch (in_columnTag)
    {
        case DSI_SCHEMA_NAME_COLUMN_TAG:
        {
            in_data->SetNull(true);
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
bool R1SchemaOnlyMetadataSource::Move(DSIDirection in_direction, simba_signed_native in_offset)
{
    if (DSI_DIR_NEXT != in_direction)
    {
        R1THROWGEN(L"R1ResultSetTraverseDirNotSupported");
    }

    return false;
}
