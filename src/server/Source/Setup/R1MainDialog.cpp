//==================================================================================================
///  @file R1MainDialog.cpp
///
///  Implementation of the class R1MainDialog.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
//==================================================================================================

#include "R1MainDialog.h"

#include "NumberConverter.h"
#include "ProductVersion.h"
#include "resource.h"
#include "R1DialogConsts.h"
#include "R1LoggingDialog.h"
#include "R1ResourceHelper.h"
#include "R1SetupException.h"

using namespace RyftOne;
using namespace std; 

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1MainDialog::Show(
    HWND in_parentWindow, 
    R1DSNConfiguration& in_configSettings, 
    bool in_onlyRequired /* = false */)
{
    in_configSettings.GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1MainDialog", "Show");

    DialogSettings settings(in_configSettings, in_onlyRequired);

    // Use the windows functions to show a dialog box which returns true if the user OK'ed it, 
    // false otherwise.

    // Show the dialog box and get if the user pressed OK or cancel.
    // MAINTENANCE NOTE: "false !=" is added to avoid compiler warning:
    // warning C4800: 'INT_PTR' : forcing value to bool 'true' or 'false' (performance warning)
    return (false != DialogBoxParam(
        R1ResourceHelper::GetModuleInstance(),
        MAKEINTRESOURCE(IDD_DIALOG_DSN_CONFIG),
        in_parentWindow,
        reinterpret_cast<DLGPROC>(ActionProc),
        reinterpret_cast<LPARAM>(&settings)));
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR R1MainDialog::ActionProc(
    HWND hwndDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    INT_PTR returnValue = static_cast<INT_PTR> (true);

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

                    case IDC_BUTTON_LOGGING:
                    {
                        DoLoggingDialogAction(hwndDlg);
                        break;
                    }

                    case IDC_BUTTON_TEST:
                    {
                        DoTestAction(hwndDlg);
                        break;
                    }

                    case IDC_EDIT_DSN:
                    case IDC_EDIT_LNG:
                    case IDC_EDIT_PWD:
                    case IDC_EDIT_USER:
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

                    default:
                    {
                        // Unknown command.
                        returnValue = static_cast<INT_PTR> (false);
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
                returnValue = static_cast<INT_PTR> (false);
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
            "R1MainDialog",
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
            "R1MainDialog",
            "ActionProc",
            e);
        ShowErrorDialog(hwndDlg, e);
    }

    return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1MainDialog::CheckEnableButtons(HWND in_dialogHandle)
{
    R1DSNConfiguration* config = GET_CONFIG(in_dialogHandle, R1DSNConfiguration);
    assert(config);

    bool enableOK = !IsEditEmpty(in_dialogHandle, IDC_EDIT_USER);

    if (enableOK &&
        (CT_CONNECTION != config->GetConfigType()))
    {
        // We don't care about the DSN if only doing a connection dialog.
        enableOK = !IsEditEmpty(in_dialogHandle, IDC_EDIT_DSN);
    }

    bool enableTest = enableOK && !IsEditEmpty(in_dialogHandle, IDC_EDIT_PWD);

    // If any required fields are empty, then disable the OK and test button.
    EnableWindow(GetDlgItem(in_dialogHandle, IDOK), enableOK);
    EnableWindow(GetDlgItem(in_dialogHandle, IDC_BUTTON_TEST), enableTest);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1MainDialog::DoCancelAction(HWND in_dialogHandle)
{
    R1DSNConfiguration* config = GET_CONFIG(in_dialogHandle, R1DSNConfiguration);
    assert(config);

    config->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1MainDialog", "DoCancelAction");

    EndDialog(in_dialogHandle, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1MainDialog::DoLoggingDialogAction(HWND in_dialogHandle)
{
    R1DSNConfiguration* config = GET_CONFIG(in_dialogHandle, R1DSNConfiguration);
    assert(config);

    config->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1MainDialog", "DoLoggingDialogAction");

    R1LoggingDialog loggingDialog;
    loggingDialog.Show(in_dialogHandle, *config);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1MainDialog::DoOkAction(HWND in_dialogHandle)
{
    R1DSNConfiguration* config = GET_CONFIG(in_dialogHandle, R1DSNConfiguration);
    assert(config);

    config->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1MainDialog", "DoOkAction");

    try
    {
        SaveSettings(in_dialogHandle, (CT_CONNECTION == config->GetConfigType()));
        EndDialog(in_dialogHandle, true);
    }
    catch (R1SetupException& e)
    {
        const simba_wstring wErrMsg = e.GetErrorMsg();
        const simba_string errMsg = wErrMsg.GetAsUTF8();

        config->GetLog()->LogError(
            "Simba::RyftOne", 
            "R1MainDialog", 
            "DoOkAction",
            errMsg.c_str());
        ShowErrorDialog(in_dialogHandle, wErrMsg);
    }
    catch (ErrorException& e)
    {
        config->GetLog()->LogError(
            "Simba::RyftOne", 
            "R1MainDialog", 
            "DoOkAction",
            e);
        ShowErrorDialog(in_dialogHandle, e);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1MainDialog::DoTestAction(HWND in_dialogHandle)
{
    R1DSNConfiguration* config = GET_CONFIG(in_dialogHandle, R1DSNConfiguration);
    assert(config);

    config->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1MainDialog", "DoTestAction");

    try
    {
        SaveSettings(in_dialogHandle, true);

        R1TestResultDialog testDialog;
        testDialog.Show(in_dialogHandle, *config);
    }
    catch (ErrorException& e)
    {
        config->GetLog()->LogError(
            "Simba::RyftOne", 
            "R1MainDialog", 
            "DoTestAction",
            e);
        ShowErrorDialog(in_dialogHandle, e);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1MainDialog::Initialize(HWND in_dialogHandle, LPARAM in_lParam)
{
    // Store handle to connect information.
    DialogSettings* dialogSettings = reinterpret_cast<DialogSettings*>(in_lParam);
    assert(dialogSettings);

    R1DSNConfiguration* settings = &dialogSettings->m_configSettings;
    SetProp(in_dialogHandle, CONF_PROP, reinterpret_cast<HANDLE>(settings));

    settings->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1MainDialog", "Initialize");

    R1BaseDialog::CenterDialog(in_dialogHandle);
    R1BaseDialog::SetIcon(in_dialogHandle, IDI_ICON1);
    InitializeText(in_dialogHandle, (CT_CONNECTION == settings->GetConfigType()));

    // Restrict the length of the DSN edit box.
    SendDlgItemMessage(
        in_dialogHandle,
        IDC_EDIT_DSN,
        EM_SETLIMITTEXT,
        R1DialogConsts::MaxDsnLength,
        0);

    // Restrict the length of the description edit box.
    SendDlgItemMessage(
        in_dialogHandle,
        IDC_EDIT_DESC,
        EM_SETLIMITTEXT,
        R1DialogConsts::MaxDescLength,
        0);

    // Set the text boxes.
    SetEditText(in_dialogHandle, IDC_EDIT_DSN, settings->GetDSN());
    SetEditText(in_dialogHandle, IDC_EDIT_DESC, settings->GetDescription());
    SetEditText(in_dialogHandle, IDC_EDIT_PWD, settings->GetPassword());
    SetEditText(in_dialogHandle, IDC_EDIT_LNG, settings->GetLanguage());
    SetEditText(in_dialogHandle, IDC_EDIT_USER, settings->GetUser());

    if (CT_EXISTING_DSN == settings->GetConfigType())
    {
        settings->GetLog()->LogTrace(
            "Simba::RyftOne",
            "R1MainDialog",
            "Initialize",
            "Modifying existing DSN.");

        // Disable editing the DSN name when not creating a new DSN.
        EnableWindow(GetDlgItem(in_dialogHandle, IDC_EDIT_DSN), FALSE);
    }
    else if (CT_NEW_DSN == settings->GetConfigType())
    {
        settings->GetLog()->LogTrace(
            "Simba::RyftOne",
            "R1MainDialog",
            "Initialize",
            "Creating a new DSN.");
    }
    else
    {
        settings->GetLog()->LogTrace(
            "Simba::RyftOne",
            "R1MainDialog",
            "Initialize",
            "Showing a connection dialog.");

        // Hide some of the configuration settings and disable others if this is a connection
        // dialog.
        EnableWindow(GetDlgItem(in_dialogHandle, IDC_EDIT_DSN), FALSE);
        EnableWindow(GetDlgItem(in_dialogHandle, IDC_EDIT_DESC), FALSE);
        ShowWindow(GetDlgItem(in_dialogHandle, IDC_BUTTON_TEST), SW_HIDE);
        ShowWindow(GetDlgItem(in_dialogHandle, IDC_BUTTON_LOGGING), SW_HIDE);

        // Set the password in the dialog since it may have been specified.
        SetEditText(in_dialogHandle, IDC_EDIT_PWD, settings->GetPassword());

        if (dialogSettings->m_onlyRequired)
        {
            // Disable the optional fields.
            EnableWindow(GetDlgItem(in_dialogHandle, IDC_EDIT_LNG), FALSE);
        }
    }

    // Check to make sure that the required fields are filled.
    CheckEnableButtons(in_dialogHandle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1MainDialog::InitializeText(HWND in_dialogHandle, bool in_isConnDialog)
{
    CString string;

    if (in_isConnDialog)
    {
        R1ResourceHelper::LoadStringResource(IDS_CONN_MAIN_TITLE, string);
    }
    else
    {
        R1ResourceHelper::LoadStringResource(IDS_MAIN_TITLE, string);
    }
    SetWindowText(in_dialogHandle, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_DSN, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_DSN, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_DESC, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_DESC, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_LANGUAGE, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_LNG, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_USER, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_USER, string);

    R1ResourceHelper::LoadStringResource(IDS_STATIC_PWD, string);
    SetDlgItemText(in_dialogHandle, IDC_STATIC_PWD, string);

    CString version("v");
    version += VER_PRODUCTVERSION_STR;
#if (1 == PLATFORM_IS_64_BIT)
    R1ResourceHelper::LoadStringResource(IDS_STATIC_VERSION_64, string);
#else
    R1ResourceHelper::LoadStringResource(IDS_STATIC_VERSION_32, string);
#endif
    version += string;
    SetDlgItemText(in_dialogHandle, IDC_STATIC_VERSION, version);

    R1ResourceHelper::LoadStringResource(IDS_BUTTON_TEST, string);
    SetDlgItemText(in_dialogHandle, IDC_BUTTON_TEST, string);

    R1ResourceHelper::LoadStringResource(IDS_BUTTON_OK, string);
    SetDlgItemText(in_dialogHandle, IDOK, string);

    R1ResourceHelper::LoadStringResource(IDS_BUTTON_CANCEL, string);
    SetDlgItemText(in_dialogHandle, IDCANCEL, string);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1MainDialog::SaveSettings(HWND in_dialogHandle, bool in_savePassword)
{
    R1DSNConfiguration* config = GET_CONFIG(in_dialogHandle, R1DSNConfiguration);
    assert(config);

    config->GetLog()->LogFunctionEntrance("Simba::RyftOne", "R1MainDialog", "SaveSettings");

    config->SetDSN(GetEditText(in_dialogHandle, IDC_EDIT_DSN));
    config->SetDescription(GetEditText(in_dialogHandle, IDC_EDIT_DESC));
    config->SetLanguage(GetEditText(in_dialogHandle, IDC_EDIT_LNG));
    config->SetUser(GetEditText(in_dialogHandle, IDC_EDIT_USER));
    config->SetPassword(GetEditText(in_dialogHandle, IDC_EDIT_PWD));

    if (in_savePassword)
    {
        // Ensure the password is passed back to the connection.
        config->SetPassword(GetEditText(in_dialogHandle, IDC_EDIT_PWD));
    }
}
