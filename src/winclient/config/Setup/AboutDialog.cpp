//==================================================================================================
///  @file AboutDialog.cpp
///
///  Implementation of the class BAboutDialog.
///
///  Copyright (C) 2015 Ryft Incorporated.
//==================================================================================================

#include "AboutDialog.h"

//#include "NumberConverter.h"
#include "DialogConsts.h"
#include "VersionResource.h"
#include "LoggingDialog.h"
#include "ResourceHelper.h"
#include "SetupException.h"
#include "resource.h"

#include <shlobj.h>
#include <shellapi.h>

using namespace Ryft;
using namespace std; 

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
bool AboutDialog::Show(HWND in_parentWindow, DSNConfiguration& in_configSettings)
{
    // Use the windows functions to show a dialog box which returns true if the user OK'ed it, 
    // false otherwise.

    // Show the dialog box and get if the user pressed OK or cancel.
    // MAINTENANCE NOTE: "false !=" is added to avoid compiler warning:
    // warning C4800: 'INT_PTR' : forcing value to bool 'true' or 'false' (performance warning)
    return (false != DialogBoxParam(
        ResourceHelper::GetModuleInstance(),
        MAKEINTRESOURCE(IDD_ABOUT),
        in_parentWindow,
        reinterpret_cast<DLGPROC>(ActionProc),
        reinterpret_cast<LPARAM>(&in_configSettings)));
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR AboutDialog::ActionProc(
    HWND hwndDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    INT_PTR returnValue = static_cast<INT_PTR> (true);
    HBITMAP hbitmap;
    simba_wstring url;

    try {
        // Switch through the different messages that can be sent to the dialog by Windows and take 
        // the appropriate action.
        switch (message) { 
            case WM_INITDIALOG: 
                Initialize(hwndDlg, lParam);
                break;

            case WM_NOTIFY: // this is notification from the syslink
                switch (((LPNMHDR)lParam)->code) {
                case NM_CLICK: 
                case NM_RETURN:
                    url = ResourceHelper::LoadStringResource(IDS_RYFT_URL);
                    ShellExecute(0, 0, url.GetAsPlatformWString().c_str(), 0, 0, SW_SHOW);
                    break;
                }
                break;

            case WM_COMMAND: 
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
                    default:
                        // Unknown command.
                        returnValue = static_cast<INT_PTR> (false);
                        break;
                }
                break;

            case WM_DESTROY:
                hbitmap = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_LOGO, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, 0L);
                if(hbitmap)
                    DeleteObject(hbitmap);
                returnValue = static_cast<INT_PTR> (false);
                break;

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
void AboutDialog::DoCancelAction(HWND in_dialogHandle)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    EndDialog(in_dialogHandle, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AboutDialog::DoOkAction(HWND in_dialogHandle)
{
    DSNConfiguration* config = GET_CONFIG(in_dialogHandle, DSNConfiguration);
    assert(config);

    EndDialog(in_dialogHandle, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AboutDialog::Initialize(HWND in_dialogHandle, LPARAM in_lParam)
{
    // Store handle to connect information.
    DSNConfiguration* settings = reinterpret_cast<DSNConfiguration*>(in_lParam);
    assert(settings);

    SetProp(in_dialogHandle, CONF_PROP, reinterpret_cast<HANDLE>(settings));

    BaseDialog::CenterDialog(in_dialogHandle);
    BaseDialog::SetIcon(in_dialogHandle, IDI_ICON1);

    WCHAR   szVersion[64];
    DWORD   dwFileVerMS;
    DWORD   dwFileVerLS;

    ResourceHelper::GetFileVersion(&dwFileVerMS, &dwFileVerLS);
    unsigned major = HIWORD(dwFileVerMS);
    unsigned minor = LOWORD(dwFileVerMS);
    unsigned build = HIWORD(dwFileVerLS);
    simba_wstring version = GetEditText(in_dialogHandle, IDC_STATIC_VERSION);
#if (1 == PLATFORM_IS_64_BIT)
    wsprintf(szVersion, version.GetAsPlatformWString().c_str(), major, minor, build, 64);
#else
    wsprintf(szVersion, version.GetAsPlatformWString().c_str(), major, minor, build, 32);
#endif
    SetEditText(in_dialogHandle, IDC_STATIC_VERSION, szVersion);

    //
    // load the png into the dialog
    RECT    rectLogo;
    BITMAP  bitmapLogo;
    HDC     hdc;
    HDC     hdcMem;
    HDC     hdcResized;
    HBITMAP hbitmapLogo;
    HBITMAP hbitmapResized;
    INT     height;
    INT     width;

    hdc = GetDC(in_dialogHandle);
    GetClientRect(GetDlgItem(in_dialogHandle, IDC_LOGO), &rectLogo);

    hdcMem = CreateCompatibleDC(hdc);
    hbitmapLogo = ResourceHelper::LoadResourceHBITMAP(MAKEINTRESOURCE(IDB_LOGO));
    GetObject(hbitmapLogo, sizeof(BITMAP), &bitmapLogo);

    // maximum size that fits in window
    if(((rectLogo.right * 100) / bitmapLogo.bmWidth) >
        (rectLogo.bottom * 100) / bitmapLogo.bmHeight) {
            width = (bitmapLogo.bmWidth * rectLogo.bottom) / bitmapLogo.bmHeight;
            height = rectLogo.bottom;
    }
    else {
        width = rectLogo.right;
        height = (bitmapLogo.bmHeight * rectLogo.right) / bitmapLogo.bmWidth;
    }

    hdcResized = CreateCompatibleDC(hdc);
    hbitmapResized = CreateBitmap( width, height, bitmapLogo.bmPlanes, bitmapLogo.bmBitsPixel, NULL);

    hbitmapLogo = (HBITMAP)SelectObject(hdcMem, hbitmapLogo);
    hbitmapResized = (HBITMAP)SelectObject(hdcResized, hbitmapResized);
    
    SetStretchBltMode(hdcResized, HALFTONE);
    StretchBlt(hdcResized, 0, 0, width, height, hdcMem, 0, 0, bitmapLogo.bmWidth, bitmapLogo.bmHeight, SRCCOPY);

    hbitmapLogo = (HBITMAP)SelectObject(hdcMem, hbitmapLogo);
    hbitmapResized = (HBITMAP)SelectObject(hdcResized, hbitmapResized);
    SendDlgItemMessage(in_dialogHandle, IDC_LOGO, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitmapResized);
    
    DeleteObject(hbitmapLogo);
    DeleteDC(hdcMem);
    DeleteDC(hdcResized);
    ReleaseDC(in_dialogHandle, hdc);
}

