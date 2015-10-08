//==================================================================================================
///  @file R1TestResultDialog.cpp
///
///  Implementation of the class R1TestResultDialog.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
//==================================================================================================

#include "R1TestResultDialog.h"

#include "DSIDriverSingleton.h"
#include "IConnection.h"
#include "IDriver.h"
#include "IEnvironment.h"
#include "resource.h"
#include "R1DialogConsts.h"
#include "R1ResourceHelper.h"
#include "R1SetupException.h"

#include <atlstr.h>

using namespace Simba::DSI;
using namespace RyftOne;
using namespace std; 

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1TestResultDialog::Show(HWND in_parentWindow, R1DSNConfiguration& in_configSettings)
{
    in_configSettings.GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1TestResultDialog", "Show");

    // Use the windows functions to show a dialog box which returns true if the user OK'ed it, 
    // false otherwise.

    // Show the dialog box and get if the user pressed OK or cancel.
    // MAINTENANCE NOTE: "false !=" is added to avoid compiler warning:
    // warning C4800: 'INT_PTR' : forcing value to bool 'true' or 'false' (performance warning)
    return (false != DialogBoxParam(
        R1ResourceHelper::GetModuleInstance(),
        MAKEINTRESOURCE(IDD_DIALOG_TESTRESR1T),
        in_parentWindow,
        reinterpret_cast<DLGPROC>(ActionProc),
        reinterpret_cast<LPARAM>(&in_configSettings)));
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR R1TestResultDialog::ActionProc(
    HWND hwndDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    INT_PTR returnValue = static_cast<INT_PTR>(true);

    try
    {
        // Switch through the different messages that can be sent to the dialog by Windows and take 
        // the appropriate action.
        switch (message) 
        { 
            case WM_INITDIALOG:
            {
                Initialize(hwndDlg, lParam);
                break;
            }

            case WM_COMMAND:
            {
                // The user has done some action on the dialog box.
                switch (LOWORD(wParam))
                {
                    case IDOK:
                    {
                        // OK button pressed.
                        DoOkAction(hwndDlg);
                        break;
                    }

                    default:
                    {
                        // Unknown command.
                        returnValue = static_cast<INT_PTR>(false);
                        break;
                    }
                }
                break;
            }

            case WM_DESTROY:
            case WM_NCDESTROY:
            {
                // WM_DESTROY - Destroy the dialog box. No action needs to be taken.
                // WM_NCDESTROY - Sent after the dialog box has been destroyed.  No action needs to 
                // be taken.
                break;
            }

            default:
            {
                // Unrecognized message.
                returnValue = static_cast<INT_PTR>(false);
                break;
            }
        } 
    }
    catch (R1SetupException& e)
    {
        const simba_wstring wErrMsg = e.GetErrorMsg();
        const simba_string errMsg = wErrMsg.GetAsUTF8();

        R1DSNConfiguration* config = GET_CONFIG(hwndDlg, R1DSNConfiguration);
        assert(config);
        config->GetLog()->LogError(
            "Simba::RyftOne",
            "R1TestResultDialog",
            "ActionProc",
            "%s",
            errMsg.c_str());
        ShowErrorDialog(hwndDlg, wErrMsg);
    }
    catch (ErrorException& e)
    {
        R1DSNConfiguration* config = GET_CONFIG(hwndDlg, R1DSNConfiguration);
        assert(config);
        config->GetLog()->LogError(
            "Simba::RyftOne",
            "R1TestResultDialog",
            "ActionProc",
            e);
        ShowErrorDialog(hwndDlg, e);
    }

    return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1TestResultDialog::DoOkAction(HWND in_dialogHandle)
{
    R1DSNConfiguration* config = GET_CONFIG(in_dialogHandle, R1DSNConfiguration);
    assert(config);

    config->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1TestResultDialog", "DoOkAction");

    EndDialog(in_dialogHandle, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1TestResultDialog::Initialize(HWND in_dialogHandle, LPARAM in_lParam)
{
    R1DSNConfiguration* settings = reinterpret_cast<R1DSNConfiguration*>(in_lParam);
    assert(settings);
    SetProp(in_dialogHandle, CONF_PROP, reinterpret_cast<HANDLE>(settings));

    settings->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1TestResultDialog", "Initialize");

    R1BaseDialog::CenterDialog(in_dialogHandle);
    R1BaseDialog::SetIcon(in_dialogHandle, IDI_ICON1);
    InitializeText(in_dialogHandle);

    simba_wstring resultDetail;
    try
    {
        SetCursorHourglass();

        // Generate the connection settings.
        DSIConnSettingRequestMap connSettings;
        settings->RetrieveConnectionSettings(connSettings);

        if (LOG_INFO <= settings->GetLog()->GetLogLevel())
        {
            // Log the settings only if at an applicable logging level.
            simba_string settingString;
            for (DSIConnSettingRequestMap::const_iterator itr = connSettings.begin();
                 itr != connSettings.end();
                 ++itr)
            {
                settingString += itr->first.GetAsUTF8() + "=" + itr->second.GetStringValue() + ";";
            }

            settings->GetLog()->LogInfo(
                "Simba::RyftOne",
                "R1TestResultDialog",
                "Initialize",
                "Testing connection using settings: %s",
                settingString.c_str());
        }

        // Create the connection.
        AutoPtr<IEnvironment> env(DSIDriverSingleton::GetDSIDriver()->CreateEnvironment());
        AutoPtr<IConnection> conn(env->CreateConnection());

        // Don't bother checking the responseSettings, the driver should fail on connect if
        // a required setting isn't present.
        DSIConnSettingResponseMap responseSettings;
        conn->UpdateConnectionSettings(connSettings, responseSettings);
        conn->Connect(connSettings);
        conn->Disconnect();

        settings->GetLog()->LogInfo(
            "Simba::RyftOne", 
            "R1TestResultDialog", 
            "Initialize",
            "Test connected successfully.");

        resultDetail = R1ResourceHelper::LoadStringResource(IDS_TEST_SUCCESS);
    }
    catch (ErrorException& e)
    {
        simba_wstring errorMsg = GetErrorMessage(e);
        settings->GetLog()->LogError(
            "Simba::RyftOne", 
            "R1TestResultDialog", 
            "Initialize", 
            "%s", 
            errorMsg.GetAsPlatformString().c_str());
        resultDetail = R1ResourceHelper::LoadStringResource(IDS_TEST_FAILURE) + errorMsg;
    }

    SetCursorArrow();

    // Set the text boxes.
    SetEditText(in_dialogHandle, IDC_EDIT_TESTRESR1T, resultDetail);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1TestResultDialog::InitializeText(HWND in_dialogHandle)
{
    CString string;
    R1ResourceHelper::LoadStringResource(IDS_TEST_RESR1T_TITLE, string);
    SetWindowText(in_dialogHandle, string);

    R1ResourceHelper::LoadStringResource(IDS_BUTTON_OK, string);
    SetDlgItemText(in_dialogHandle, IDOK, string);
}
