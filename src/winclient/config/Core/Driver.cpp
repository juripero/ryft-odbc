// =================================================================================================
///  @file Driver.cpp
///
///  Ryft DSIDriver implementation.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
// =================================================================================================

#include "Driver.h"

#include "DSIDriverProperties.h"
#include "DSILog.h"
#include "DSIMessageSource.h"

using namespace Ryft;
using namespace Simba::DSI;
using namespace std;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
Driver::Driver() : Simba::DSI::DSIDriver(), m_driverlog("ryft_driver.log")
{
    m_msgSrc->RegisterMessages(ERROR_MESSAGES_FILE, RYFT_ERROR);
    m_msgSrc->SetVendorName(DRIVER_VENDOR);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Driver::~Driver()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IEnvironment* Driver::CreateEnvironment()
{
    ENTRANCE_LOG(&m_driverlog, "Ryft", "Driver", "CreateEnvironment");
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ILogger* Driver::GetDriverLog()
{
    return &m_driverlog;
}

