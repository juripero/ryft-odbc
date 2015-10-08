//==================================================================================================
///  @file LoggingDialog.cpp
///
///  Implementation of the class LoggingDialog.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
//==================================================================================================

#include "LoggingDialog.h"

#include "DSIFileLogger.h"
#include "NumberConverter.h"
#include "resource.h"
#include "DialogConsts.h"
#include "ResourceHelper.h"
#include "SetupException.h"

#include <atlstr.h>
#include <windowsx.h>

using namespace Ryft;
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
bool LoggingDialog::Show(HWND in_parentWindow, DSNConfiguration& in_configSettings)
{
    // Use the windows functions to show a dialog box which returns true if the user OK'ed it, 
    // false otherwise.

    // Show the dialog box and get if the user pressed OK or cancel.
    // MAINTENANCE NOTE: "false !=" is added to avoid compiler warning:
    // warning C4800: 'INT_PTR' : forcing value to bool 'true' or 'false' (performance warning)
    return (false != DialogBoxParam(
        ResourceHelper::GetModuleInstance(),
        MAKEINTRESOURCE(IDD_DIALOG_LOGGING),
        in_parentWindow,
        reinterpret_cast<DLGPROC>(ActionProc),
        reinterpret_cast<LPARAM>(&in_configSettings)));
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR LoggingDialog::ActionProc(
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
                        // OK button pressed.
                        DoOkAction(hwndDlg);
                        break;
                    case IDCANCEL:
                        // Cancel button pressed.
                        DoCancelAction(hwndDlg);
                        break;
                    case IDC_ENABLELOGGING:
                        CheckEnableButtons(hwndDlg);
                        if((BN_CLICKED == HIWORD(wParam)) && (Button_GetState((HWND)lParam) & BST_CHECKED))
                            SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_LOGPATH));
                        break;
                    case IDC_BUTTONFOLDER:
                        CreateThread(NULL, 0, BrowseUIThread, hwndDlg, 0, NULL);
                        break;
                    case IDC_EDIT_LOGPATH: {
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

#include <shlobj.h>
DWORD WINAPI LoggingDialog::BrowseUIThread( LPVOID lp )
{
    DoFolderAction((HWND)lp);
    return 0;
}

INT CALLBACK LoggingDialog::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM in_dialogHandle) 
{
   WCHAR szDir[MAX_PATH];

   switch(uMsg) {
   case BFFM_INITIALIZED:
       if(!IsEditEmpty((HWND)in_dialogHandle, IDC_EDIT_LOGPATH)) {
           simba_wstring strDir = GetEditText((HWND)in_dialogHandle, IDC_EDIT_LOGPATH);
           SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)strDir.GetAsPlatformWString().c_str());
       }
       else if (GetCurrentDirectory(sizeof(szDir)/sizeof(char), szDir)) { 
         SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)szDir);
       }
      break;
   }
   return 0;
}

void LoggingDialog::DoFolderAction(HWND in_dialogHandle)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    simba_wstring title = ResourceHelper::LoadStringResource(IDS_PICK_A_FOLDER);

    BROWSEINFO bi = { 0 };
    bi.lpszTitle = title.GetAsPlatformWString().c_str();
    bi.hwndOwner = in_dialogHandle;
    bi.ulFlags = BIF_USENEWUI | BIF_VALIDATE;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)in_dialogHandle;
    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );
    if ( pidl != 0 ) {
        // get the name of the folder
        WCHAR path[MAX_PATH];
        if ( SHGetPathFromIDList ( pidl, path ) )
            SetDlgItemText(in_dialogHandle, IDC_EDIT_LOGPATH, path);

        // free memory used
        IMalloc * imalloc = 0;
        if ( SUCCEEDED( SHGetMalloc ( &imalloc )) ) {
            imalloc->Free ( pidl );
            imalloc->Release ( );
        }
    }
    CoUninitialize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LoggingDialog::CheckEnableButtons(HWND in_dialogHandle)
{
    bool isLoggingEnabled = Button_GetState(GetDlgItem(in_dialogHandle, IDC_ENABLELOGGING)) & BST_CHECKED;
    bool enableOK = true;

    EnableWindow(GetDlgItem(in_dialogHandle, IDC_BUTTONFOLDER), isLoggingEnabled);
    EnableWindow(GetDlgItem(in_dialogHandle, IDC_EDIT_LOGPATH), isLoggingEnabled);

    if (isLoggingEnabled) 
        enableOK = !IsEditEmpty(in_dialogHandle, IDC_EDIT_LOGPATH);

    EnableWindow(GetDlgItem(in_dialogHandle, IDOK), enableOK);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LoggingDialog::DoCancelAction(HWND in_dialogHandle)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    EndDialog(in_dialogHandle, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LoggingDialog::DoOkAction(HWND in_dialogHandle)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    // Record the logging level
    bool isLoggingEnabled = Button_GetState(GetDlgItem(in_dialogHandle, IDC_ENABLELOGGING)) & BST_CHECKED;
    config->SetLogLevel(
        NumberConverter::ConvertInt32ToWString(static_cast<simba_int32>(isLoggingEnabled ? LOG_MAX : LOG_MIN)));
    config->SetLogPath(GetEditText(in_dialogHandle, IDC_EDIT_LOGPATH));

    EndDialog(in_dialogHandle, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LoggingDialog::Initialize(HWND in_dialogHandle, LPARAM in_lParam)
{
    // Store handle to connect information.
    DSNConfiguration* settings = reinterpret_cast<DSNConfiguration*>(in_lParam);
    assert(settings);
    SetProp(in_dialogHandle, CONF_PROP, reinterpret_cast<HANDLE>(settings));

    BaseDialog::CenterDialog(in_dialogHandle);
    BaseDialog::SetIcon(in_dialogHandle, IDI_ICON1);
    InitializeText(in_dialogHandle);

    if(settings->GetLogLevel().Compare(NumberConverter::ConvertInt32ToWString(static_cast<simba_int32>(LOG_MIN)))) {
        SetEditText(in_dialogHandle, IDC_EDIT_LOGPATH, settings->GetLogPath());
        Button_SetCheck(GetDlgItem(in_dialogHandle, IDC_ENABLELOGGING), BST_CHECKED);
        SetFocus(GetDlgItem(in_dialogHandle, IDC_EDIT_LOGPATH));
    }
    else
        SetFocus(GetDlgItem(in_dialogHandle, IDC_ENABLELOGGING));

    // Set the log file textbox.
    SetEditText(in_dialogHandle, IDC_EDIT_LOGPATH, settings->GetLogPath());

    CheckEnableButtons(in_dialogHandle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LoggingDialog::InitializeText(HWND in_dialogHandle)
{
}
