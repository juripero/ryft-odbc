// =================================================================================================
///  @file R1Driver.cpp
///
///  RyftOne DSIDriver implementation.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1Driver.h"

#include "DSIDriverProperties.h"
#include "DSILog.h"
#include "DSIMessageSource.h"
#include "R1Environment.h"

using namespace RyftOne;
using namespace Simba::DSI;
using namespace std;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #3: Set the driver-wide logging details.")
R1Driver::R1Driver() : Simba::DSI::DSIDriver(), m_driverlog("ryftone_server.log")
{
    ENTRANCE_LOG(&m_driverlog, "Simba::RyftOne", "R1Driver", "R1Driver");

#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #14: Register the R1Messages.xml file for handling by DSIMessageSource.")
    // This registers the file that is used to look up error messages for the DSII, along with the 
    // unique component ID that identifies the DSII. The component ID is also used in the error 
    // messages XML file that is registered, and is used when throwing exceptions to determine which
    // file to load the error messages from.
    m_msgSrc->RegisterMessages(ERROR_MESSAGES_FILE, R1_ERROR);

#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #15: Set the vendor name, which will be prepended to error messages.")
    /*
    NOTE: We do not set the vendor name here on purpose, because the default vendor name is 'Simba'.
    The code below shows how to set the vendor name. A typical error message will take the form

    [<vendor>][<component>] (nativeErrorCode) <message>

    so setting the vendor name will change the [<vendor>] part from the default of [Simba].

    m_msgSrc->SetVendorName(DRIVER_VENDOR);
    */

    SetDriverPropertyValues();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1Driver::~R1Driver()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IEnvironment* R1Driver::CreateEnvironment()
{
    ENTRANCE_LOG(&m_driverlog, "Simba::RyftOne", "R1Driver", "CreateEnvironment");
    return new R1Environment(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ILogger* R1Driver::GetDriverLog()
{
    return &m_driverlog;
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Driver::SetDriverPropertyValues()
{
#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #2: Set the driver properties.") 
    // Set the following
    //      DSI_DRIVER_DRIVER_NAME
    //          The name of the driver that is shown to the application.
    //
    // Ensure that the following is set for your driver:
    //      DSI_DRIVER_STRING_DATA_ENCODING
    //          The encoding of character data from the perspective of the data store. 
    //          For this driver, the encoding is ENC_CP1252.
    //      DSI_DRIVER_WIDE_STRING_DATA_ENCODING
    //          The encoding of wide character data from the perspective of the data store.  
    //          For this driver, the encoding is ENC_UTF16_LE.

    SetProperty(
        DSI_DRIVER_DRIVER_NAME, 
        AttributeData::MakeNewWStringAttributeData("RyftOne"));

    // The default encoding used for string data and input/output parameter values.
    SetProperty(
        DSI_DRIVER_STRING_DATA_ENCODING,
        AttributeData::MakeNewInt16AttributeData(ENC_CP1252));

    // The default encoding used for wide string data and input/output parameter values.
#if (1 == PLATFORM_IS_LITTLE_ENDIAN)
    SetProperty(
        DSI_DRIVER_WIDE_STRING_DATA_ENCODING, 
        AttributeData::MakeNewInt16AttributeData(ENC_UTF16_LE));
#else
    SetProperty(
        DSI_DRIVER_WIDE_STRING_DATA_ENCODING, 
        AttributeData::MakeNewInt16AttributeData(ENC_UTF16_BE));
#endif

    SetProperty(
        DSI_FILTER_METADATA_SOURCE, 
        AttributeData::MakeNewUInt32AttributeData(DSI_FMS_FALSE));
}
