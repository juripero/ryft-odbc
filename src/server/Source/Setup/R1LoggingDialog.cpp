//==================================================================================================
///  @file R1LoggingDialog.cpp
///
///  Implementation of the class R1LoggingDialog.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
//==================================================================================================

#include "R1LoggingDialog.h"

#include "DSIFileLogger.h"
#include "NumberConverter.h"
#include "resource.h"
#include "R1DialogConsts.h"
#include "R1ResourceHelper.h"
#include "R1SetupException.h"

#include <atlstr.h>

using namespace RyftOne;
using namespace std; 

// Static ==========================================================================================
const wchar_t* LOG_LEVELS[] = 
{
    L"LOG_OFF",
    L"LOG_FATAL",
    L"LOG_ERROR",
    L"LOG_WARNING",
    L"LOG_INFO",
    L"LOG_DEBUG",
    L"LOG_TRACE"
};

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1LoggingDialog::Show(HWND in_parentWindow, R1DSNConfiguration& in_configSettings)
{
    in_configSettings.GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1LoggingDialog", "Show");

    // Use the windows functions to show a dialog box which returns true if the user OK'ed it, 
    // false otherwise.

    // Show the dialog box and get if the user pressed OK or cancel.
    // MAINTENANCE NOTE: "false !=" is added to avoid compiler warning:
    // warning C4800: 'INT_PTR' : forcing value to bool 'true' or 'false' (performance warning)
    return (false != DialogBoxParam(
        R1ResourceHelper::GetModuleInstance(),
        MAKEINTRESOURCE(IDD_DIALOG_LOGGING),
        in_parentWindow,
        reinterpret_cast<DLGPROC>(ActionProc),
        reinterpret_cast<LPARAM>(&in_configSettings)));
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR R1LoggingDialog::ActionProc(
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

                    case IDCANCEL:
                    {
                        // Cancel button pressed.
                        DoCancelAction(hwndDlg);
                        break;
                    }

                    case IDC_EDIT_LOGNAMESPACE:
                    case IDC_EDIT_LOGPATH:
                    case IDC_EDIT_LOGSIZE:
                    case IDC_EDIT_LOGNUM:
                    {
                        // Only changes to required fields will be checked for disabling/enabling of 
                        // OK button.
                        if (EN_CHANGE == HIWORD(wParam))
                        {
                            // Enable/Disable the OK button if required fields are filled/empty.
                            CheckEnableButtons(hwndDlg);
                        }

                        break;
                    }

                    case IDC_COMBO_LOGLEVEL:
                    {
                        if (CBN_SELCHANGE == HIWORD(wParam))
                        {
                            // If the logging level combo box has changed, check if the logging 
                            // options should be enabled or disabled.
                            CheckEnableLoggingOptions(hwndDlg);
                        }

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
            "R1LoggingDialog",
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
            "R1LoggingDialog",
            "ActionProc",
            e);
        ShowErrorDialog(hwndDlg, e);
    }

    return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1LoggingDialog::CheckEnableButtons(HWND in_dialogHandle)
{
    bool isLoggingEnabled = (0 != SendDlgItemMessage(
        in_dialogHandle, 
        IDC_COMBO_LOGLEVEL, 
        CB_GETCURSEL, 
        0, 
        0));
    bool enableOK = true;

    if (isLoggingEnabled)
    {
        enableOK = !IsEditEmpty(in_dialogHandle, IDC_EDIT_LOGPATH);
        enableOK = enableOK && !IsEditEmpty(in_dialogHandle, IDC_EDIT_LOGSIZE);
        enableOK = enableOK && !IsEditEmpty(in_dialogHandle, IDC_EDIT_LOGNUM);
    }

    // If any required fields are empty, then disable the OK button.
    EnableWindow(GetDlgItem(in_dialogHandle, IDOK), enableOK);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1LoggingDialog::CheckEnableLoggingOptions(HWND in_dialogHandle)
{
    // Disable or enable the logging fields depending on the logging level. If the level is LOG_OFF,
    // disable the other options.
    bool isLoggingEnabled = (0 != SendDlgItemMessage(
        in_dialogHandle, 
        IDC_COMBO_LOGLEVEL, 
        CB_GETCURSEL, 
        0, 
        0));

    // Get the components and set the enabled status of each.
    EnableWindow(GetDlgItem(in_dialogHandle, IDC_EDIT_LOGPATH), isLoggingEnabled);
    EnableWindow(GetDlgItem(in_dialogHandle, IDC_EDIT_LOGNAMESPACE), isLoggingEnabled);
    EnableWindow(GetDlgItem(in_dialogHandle, IDC_EDIT_LOGNUM), isLoggingEnabled);
    EnableWindow(GetDlgItem(in_dialogHandle, IDC_EDIT_LOGSIZE), isLoggingEnabled);

    CheckEnableButtons(in_dialogHandle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1LoggingDialog::DoCancelAction(HWND in_dialogHandle)
{
    R1DSNConfiguration* config = GET_CONFIG(in_dialogHandle, R1DSNConfiguration);
    assert(config);

    config->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1LoggingDialog", "DoCancelAction");

    EndDialog(in_dialogHandle, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1LoggingDialog::DoOkAction(HWND in_dialogHandle)
{
    R1DSNConfiguration* config = GET_CONFIG(in_dialogHandle, R1DSNConfiguration);
    assert(config);

    config->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1LoggingDialog", "DoOkAction");

    // Record the logging level
    LRESR1T index = SendDlgItemMessage(
        in_dialogHandle, 
        IDC_COMBO_LOGLEVEL, 
        CB_GETCURSEL, 
        0, 
        0);

    config->SetLogLevel(
        NumberConverter::ConvertInt32ToWString(static_cast<simba_int32>(index + LOG_MIN)));
    config->SetLogPath(GetEditText(in_dialogHandle, IDC_EDIT_LOGPATH));
    config->SetLogNamespace(GetEditText(in_dialogHandle, IDC_EDIT_LOGNAMESPACE));
    config->SetLogMaxSize(GetEditText(in_dialogHandle, IDC_EDIT_LOGSIZE));
    config->SetLogMaxNum(GetEditText(in_dialogHandle, IDC_EDIT_LOGNUM));

    EndDialog(in_dialogHandle, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1LoggingDialog::Initialize(HWND in_dialogHandle, LPARAM in_lParam)
{
    // Store handle to connect information.
    R1DSNConfiguration* settings = reinterpret_cast<R1DSNConfiguration*>(in_lParam);
    assert(settings);
    SetProp(in_dialogHandle, CONF_PROP, reinterpret_cast<HANDLE>(settings));

    settings->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1LoggingDialog", "Initialize");

    R1BaseDialog::CenterDialog(in_dialogHandle);
    R1BaseDialog::SetIcon(in_dialogHandle, IDI_ICON1);
    InitializeText(in_dialogHandle);

    // Clear the contents of the log level combo-box.
    SendDlgItemMessage(
        in_dialogHandle, 
        IDC_COMBO_LOGLEVEL, 
        CB_RESETCONTENT, 
        0, 
        0);

    // Fill in the log level combo-box.
    for (simba_int32 level = LOG_MIN; LOG_MAX >= level; ++level)
    {
        SendDlgItemMessage(
            in_dialogHandle,
            IDC_COMBO_LOGLEVEL,
            CB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(LOG_LEVELS[level - LOG_MIN]));
    }

    // Set the selection to the correct level.
    SendDlgItemMessage(
        in_dialogHandle, 
        IDC_COMBO_LOGLEVEL, 
        CB_SETCURSEL, 
        Simba::DSI::DSIFileLogger::ConvertStringToLogLevel(settings->GetLogLevel()) - LOG_MIN, 
        0);

    // Set the log file textbox.
    SetEditText(in_dialogHandle, IDC_EDIT_LOGPATH, settings->GetLogPath());
    SetEditText(in_dialogHandle, IDC_EDIT_LOGNAMESPACE, settings->GetLogNamespace());
    SetEditText(in_dialogHandle, IDC_EDIT_LOGNUM, settings->GetLogMaxNum());
    SetEditText(in_dialogHandle, IDC_EDIT_LOGSIZE, settings->GetLogMaxSize());

    CheckEnableLoggingOptions(in_dialogHandle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1LoggingDialog::InitializeText(HWND in_dialogHandle)
{
    CString string;
    R1ResourceHelper::LoadStringResource(IDS_LOGGING_TITLE, string);
    SetWindowText(in_dialogHandle, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_LOGLEVEL, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_LOGLEVEL, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_LOGPATH, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_LOGPATH, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_LOGNAMESPACE, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_LOGNAMESPACE, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_ROTATION, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_ROTATION, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_LOGNUM, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_LOGNUM, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_LOGSIZE, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_LOGSIZE, string);

    R1ResourceHelper::LoadStringResource(IDS_BUTTON_OK, string);
    SetDlgItemText(in_dialogHandle, IDOK, string);

    R1ResourceHelper::LoadStringResource(IDS_BUTTON_CANCEL, string);
    SetDlgItemText(in_dialogHandle, IDCANCEL, string);
}
