// =================================================================================================
///  @file R1ConnectionDialog.cpp
///
///  DSIConnection Dialog implementation.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1ConnectionDialog.h"

#include "R1DSNConfiguration.h"
#include "R1MainDialog.h"

using namespace Simba::DSI;
using namespace RyftOne;

// Public Static ===================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1ConnectionDialog::Prompt(
    DSIConnSettingRequestMap& io_connectionSettings, 
    ILogger* in_logger,
    HWND in_parentWindow, 
    bool in_isRequired)
{
    // Set the appropriate dialog properties.
    R1MainDialog dialog;
    R1DSNConfiguration dsnConfig("", in_logger);
    dsnConfig.LoadConnectionSettings(io_connectionSettings);

    // Pop the dialog. False return code indicates the dialog was canceled.
    if (dialog.Show(in_parentWindow, dsnConfig, in_isRequired))
    {
        dsnConfig.RetrieveConnectionSettings(io_connectionSettings);
        return true;
    }

    return false;
}
