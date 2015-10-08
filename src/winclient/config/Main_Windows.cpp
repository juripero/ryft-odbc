// =================================================================================================
///  @file Main_Windows.cpp
///
///  Implementation of the DllMain() and DSIDriverFactory() for Windows platforms.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
// =================================================================================================

#include "DSIDriverFactory.h"
#include "SimbaSettingReader.h"
#include "Driver.h"

using namespace Ryft;
using namespace Simba::DSI;

simba_handle s_moduleId = 0;

//==================================================================================================
/// @brief DLL entry point.
///
/// @param in_module        Handle of the current module.
/// @param in_reasonForCall DllMain called with parameter telling if DLL is loaded into process or 
///                         thread address space.
/// @param in_reserved      Unused.
///
/// @return TRUE if successful, FALSE otherwise.
//==================================================================================================
BOOL APIENTRY DllMain(HINSTANCE in_module, DWORD in_reasonForCall, LPVOID in_reserved)
{
    // Avoid compiler warnings.
    UNUSED(in_reserved);

    if (in_reasonForCall == DLL_PROCESS_ATTACH)
    {
        s_moduleId = reinterpret_cast<simba_handle>(in_module);
    }

    return TRUE;
}

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
    out_instanceID = s_moduleId;

    // Set the branding for this driver.
    SimbaSettingReader::SetConfigurationBranding(DRIVER_WINDOWS_BRANDING);

    return new Driver();
}