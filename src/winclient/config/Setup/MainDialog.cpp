//==================================================================================================
///  @file MainDialog.cpp
///
///  Implementation of the class MainDialog.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
//==================================================================================================

#include "MainDialog.h"

#include "NumberConverter.h"
#include "ProductVersion.h"
#include "resource.h"
#include "DialogConsts.h"
#include "LoggingDialog.h"
#include "AboutDialog.h"
#include "ResourceHelper.h"
#include "SetupException.h"

using namespace Ryft;
using namespace std; 

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
bool MainDialog::Show(
    HWND in_parentWindow, 
    DSNConfiguration& in_configSettings, 
    bool in_onlyRequired /* = false */)
{
    DialogSettings settings(in_configSettings, in_onlyRequired);

    // Use the windows functions to show a dialog box which returns true if the user OK'ed it, 
    // false otherwise.

    // Show the dialog box and get if the user pressed OK or cancel.
    // MAINTENANCE NOTE: "false !=" is added to avoid compiler warning:
    // warning C4800: 'INT_PTR' : forcing value to bool 'true' or 'false' (performance warning)
    if(settings.m_configSettings.GetConfigType() == CT_CONNECTION) {
        return (false != DialogBoxParam(
            ResourceHelper::GetModuleInstance(),
            MAKEINTRESOURCE(IDD_DIALOG_CONNECT),
            in_parentWindow,
            reinterpret_cast<DLGPROC>(ActionProc),
            reinterpret_cast<LPARAM>(&settings)));
    }
    else
        return (false != DialogBoxParam(
            ResourceHelper::GetModuleInstance(),
            MAKEINTRESOURCE(IDD_DIALOG_DSN_CONFIG),
            in_parentWindow,
            reinterpret_cast<DLGPROC>(ActionProc),
            reinterpret_cast<LPARAM>(&settings)));
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR MainDialog::ActionProc(
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
        switch (message) { 
        case WM_INITDIALOG:
            Initialize(hwndDlg, lParam);
            break;

        case WM_COMMAND: {
            // The user has done some action on the dialog box.
            switch (LOWORD(wParam)) {
            case IDOK:
                // OK button pressed.
                DoOkAction(hwndDlg);
                break;
            case IDCANCEL:
                // Cancel button pressed.
                DoCancelAction(hwndDlg);
                break;
            case IDC_ABOUT:
                DoAboutAction(hwndDlg);
                break;
            case IDC_BUTTON_LOGGING:
                DoLoggingDialogAction(hwndDlg);
                break;
            case IDC_BUTTON_TEST:
                DoTestDialogAction(hwndDlg);
                break;
            case IDC_EDIT_DSN:
            case IDC_EDIT_URL:
            case IDC_EDIT_PWD:
            case IDC_EDIT_USER:
            case IDC_EDIT_PORT:
                if (EN_CHANGE == HIWORD(wParam)) {
                    // Enable/Disable the OK button if required fields are filled/empty.
                    CheckEnableButtons(hwndDlg);
                }
                break;
            default:
                // Unknown command.
                returnValue = static_cast<INT_PTR> (false);
                break;
            }
            break;
        }
        default:
            // Unrecognized message.
            returnValue = static_cast<INT_PTR> (false);
            break;
        }
    }
    catch (SetupException& e)
    {
        const simba_wstring wErrMsg = e.GetErrorMsg();
        const simba_string errMsg = wErrMsg.GetAsUTF8();

        DSNConfiguration* config = GET_CONFIG(hwndDlg, DSNConfiguration);
        assert(config);
        ShowErrorDialog(hwndDlg, wErrMsg);
    }
    catch (ErrorException& e)
    {
        DSNConfiguration* config = GET_CONFIG(hwndDlg, DSNConfiguration);
        assert(config);
        ShowErrorDialog(hwndDlg, e);
    }

    return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::CheckEnableButtons(HWND in_dialogHandle)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    bool enableOK = !IsEditEmpty(in_dialogHandle, IDC_EDIT_URL);
    enableOK = enableOK && !IsEditEmpty(in_dialogHandle, IDC_EDIT_PORT);
    enableOK = enableOK && !IsEditEmpty(in_dialogHandle, IDC_EDIT_USER);

    bool enableTest = enableOK;
    enableTest = enableTest && !IsEditEmpty(in_dialogHandle, IDC_EDIT_PWD);
    // Password not required in DSN create or edit case
    if(config->GetConfigType() == CT_CONNECTION)
        enableOK = enableOK && !IsEditEmpty(in_dialogHandle, IDC_EDIT_PWD);

    if(config->GetConfigType() == CT_NEW_DSN)  
        enableOK = enableOK && !IsEditEmpty(in_dialogHandle, IDC_EDIT_DSN);

    // If any required fields are empty, then disable the OK and test button.
    EnableWindow(GetDlgItem(in_dialogHandle, IDOK), enableOK);
    EnableWindow(GetDlgItem(in_dialogHandle, IDC_BUTTON_TEST), enableTest);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::DoCancelAction(HWND in_dialogHandle)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    EndDialog(in_dialogHandle, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::DoLoggingDialogAction(HWND in_dialogHandle)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    LoggingDialog loggingDialog;
    loggingDialog.Show(in_dialogHandle, *config);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::DoTestDialogAction(HWND in_dialogHandle)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    simba_wstring Driver = config->GetDriver();
    simba_wstring URL = GetEditText(in_dialogHandle, IDC_EDIT_URL);
    simba_wstring Port = GetEditText(in_dialogHandle, IDC_EDIT_PORT);
    simba_wstring UID = GetEditText(in_dialogHandle, IDC_EDIT_USER);
    simba_wstring PWD = GetEditText(in_dialogHandle, IDC_EDIT_PWD);

    WCHAR ConnStr[512];
    wsprintf(ConnStr, L"Driver=%s;SERVERLIST=%s %s;UID=%s;PWD=%s;", Driver.GetAsPlatformWString().c_str(), URL.GetAsPlatformWString().c_str(),
        Port.GetAsPlatformWString().c_str(), UID.GetAsPlatformWString().c_str(), PWD.GetAsPlatformWString().c_str());
    
    CString header;
    CString message;

    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    if(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) == SQL_ERROR)
        ShowErrorDialog(in_dialogHandle, L"Unable to establish connection with ODBC driver.");

    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if(SQLDriverConnectW(hDbc, in_dialogHandle, (SQLWCHAR *)ConnStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT) != SQL_SUCCESS) {
        SQLSMALLINT iRec = 0; 
        SQLINTEGER  iError; 
        SQLWCHAR    szMessage[1000]; 
        SQLWCHAR    szState[SQL_SQLSTATE_SIZE+1]; 

        SetCursor(LoadCursor(NULL, IDC_ARROW));
        if(SQLGetDiagRecW(SQL_HANDLE_DBC, hDbc, ++iRec, szState, &iError, szMessage, 
            (SQLSMALLINT)(sizeof(szMessage) / sizeof(SQLCHAR)), (SQLSMALLINT *)NULL) == SQL_SUCCESS) {
                ShowErrorDialog(in_dialogHandle, szMessage);
        } 
        else
            ShowErrorDialog(in_dialogHandle, L"Unable to establish connection with ODBC driver.");
    }
    else {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        header.LoadString(IDS_CONNECT_TEST);
        message.LoadString(IDS_CONNECT_SUCCESS);
        MessageBox(in_dialogHandle, message, header, MB_OK | MB_ICONEXCLAMATION);
    }

    if (hDbc) { 
        SQLDisconnect(hDbc); 
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc); 
    } 
 
    if (hEnv) 
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::DoOkAction(HWND in_dialogHandle)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    if(CT_CONNECTION == config->GetConfigType()) {
        try
        {
            SaveSettings(in_dialogHandle, true);
            EndDialog(in_dialogHandle, true);
        }
        catch (SetupException& e)
        {
            const simba_wstring wErrMsg = e.GetErrorMsg();
            const simba_string errMsg = wErrMsg.GetAsUTF8();

            ShowErrorDialog(in_dialogHandle, wErrMsg);
        }
        catch (ErrorException& e)
        {
            ShowErrorDialog(in_dialogHandle, e);
        }
    }
    else {
        SaveSettings(in_dialogHandle, false);
        EndDialog(in_dialogHandle, true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::DoAboutAction(HWND in_dialogHandle)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    AboutDialog aboutDialog;
    aboutDialog.Show( in_dialogHandle, *config );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::Initialize(HWND in_dialogHandle, LPARAM in_lParam)
{
    // Store handle to connect information.
    DialogSettings* dialogSettings = reinterpret_cast<DialogSettings*>(in_lParam);
    assert(dialogSettings);

    DSNConfiguration* settings = &dialogSettings->m_configSettings;
    SetProp(in_dialogHandle, CONF_PROP, reinterpret_cast<HANDLE>(settings));

    BaseDialog::CenterDialog(in_dialogHandle);
    BaseDialog::SetIcon(in_dialogHandle, IDI_ICON1);
    InitializeText(in_dialogHandle, (CT_CONNECTION == settings->GetConfigType()));

    // Restrict the length of the DSN edit box.
    SendDlgItemMessage(
        in_dialogHandle,
        IDC_EDIT_DSN,
        EM_SETLIMITTEXT,
        DialogConsts::MaxDsnLength,
        0);

    // Restrict the length of the description edit box.
    SendDlgItemMessage(
        in_dialogHandle,
        IDC_EDIT_DESC,
        EM_SETLIMITTEXT,
        DialogConsts::MaxDescLength,
        0);

    // Set the text boxes.
    SetEditText(in_dialogHandle, IDC_EDIT_DSN, settings->GetDSN());
    SetEditText(in_dialogHandle, IDC_EDIT_DESC, settings->GetDescription());
    SetEditText(in_dialogHandle, IDC_EDIT_PWD, settings->GetPassword());
    SetEditText(in_dialogHandle, IDC_EDIT_USER, settings->GetUser());
    SetEditText(in_dialogHandle, IDC_EDIT_URL, settings->GetURL());
    SetEditText(in_dialogHandle, IDC_EDIT_PORT, settings->GetPort());

    if (CT_EXISTING_DSN == settings->GetConfigType())
    {
        // Disable editing the DSN name when not creating a new DSN.
        EnableWindow(GetDlgItem(in_dialogHandle, IDC_EDIT_DSN), FALSE);
        SetFocus(GetDlgItem(in_dialogHandle, IDC_EDIT_DESC));
    }
    else if (CT_EXISTING_DSN == settings->GetConfigType())
    {
        SetFocus(GetDlgItem(in_dialogHandle, IDC_EDIT_DSN));
    }
    else
    {
        if(settings->GetUser().IsEmpty()) {
            SetFocus(GetDlgItem(in_dialogHandle, IDC_EDIT_USER));
        }
        else
            SetFocus(GetDlgItem(in_dialogHandle, IDC_EDIT_PWD));
    }

    // Check to make sure that the required fields are filled.
    CheckEnableButtons(in_dialogHandle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::InitializeText(HWND in_dialogHandle, bool in_isConnDialog)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::SaveSettings(HWND in_dialogHandle, bool in_savePassword)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    config->SetDSN(GetEditText(in_dialogHandle, IDC_EDIT_DSN));
    config->SetDescription(GetEditText(in_dialogHandle, IDC_EDIT_DESC));
    config->SetUser(GetEditText(in_dialogHandle, IDC_EDIT_USER));
    if(in_savePassword)
        config->SetPassword(GetEditText(in_dialogHandle, IDC_EDIT_PWD));
    config->SetURL(GetEditText(in_dialogHandle, IDC_EDIT_URL));
    config->SetPort(GetEditText(in_dialogHandle, IDC_EDIT_PORT));
}
