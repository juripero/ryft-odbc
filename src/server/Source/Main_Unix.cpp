// =================================================================================================
///  @file Main_Unix.cpp
///
///  Implementation of the DSIDriverFactory() for Unix platforms.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "DSIDriverFactory.h"
#include "SimbaSettingReader.h"
#include "R1Driver.h"

#ifdef SERVERTARGET
#include "CppServerAPI.h"
#endif

#include <unistd.h>

using namespace RyftOne;
using namespace Simba::DSI;

//==================================================================================================
/// @brief Creates an instance of IDriver for a driver. 
///
/// The resulting object is made available through DSIDriverSingleton::GetDSIDriver().
///
/// @param out_instanceID   Unique identifier for the IDriver instance.
///
/// @return IDriver instance. (OWN)
//==================================================================================================
IDriver* Simba::DSI::DSIDriverFactory(simba_handle& out_instanceID)
{
    out_instanceID = getpid();
    
#ifdef SERVERTARGET
    SimbaSettingReader::SetConfigurationBranding(SERVER_LINUX_BRANDING);
#else
    SimbaSettingReader::SetConfigurationBranding(DRIVER_LINUX_BRANDING);
#endif

    return new R1Driver();
}
