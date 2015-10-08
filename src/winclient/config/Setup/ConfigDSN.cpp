//==================================================================================================
///  @file ConfigDSN.cpp
///
///  Implementation of ConfigDSN and ConfigDSNW
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
//==================================================================================================

#include "AutoPtr.h"
#include "DSIDriverFactory.h"
#include "DSIDriverSingleton.h"
#include "IDriver.h"
#include "SimbaSettingReader.h"
#include "DSNConfiguration.h"
#include "MainDialog.h"
#include "SetupException.h"

#include <odbcinst.h>

using namespace Ryft;
using namespace Simba::DSI;
using namespace std;

typedef void (*AddRequestMapCallback)(
    DSIConnSettingRequestMap*,
    const simba_wstring&,
    const simba_wstring&);

// =================================================================================================
/// @brief Show the current dialog in the chain, returning if the user OKed the dialog.
///
/// @param in_connSettingResponseMap    The map containing data taken into the dialog to populate 
///                                     it.
/// @param out_connSettingRequestMap    The map in which data is put into when the user OKs the
///                                     dialog.
/// @paramin_addCallback                A callback to populate out_connSettingRequestMap.
/// @param in_parentWindow              The parent window that is creating this dialog.
///
/// @return A pair of bools, with the first indicating if the user pressed OK, and the second 
/// indicating if there is another dialog in the chain.
// =================================================================================================
std::pair<bool, bool> INSTAPI ShowDialog(
    DSIConnSettingResponseMap * in_connSettingResponseMap, 
    DSIConnSettingRequestMap * out_connSettingRequestMap, 
    AddRequestMapCallback in_addCallback,
    HWND in_parentWindow)
{
#ifdef _DEBUG
    //MessageBox(in_parentWindow, L"Debug Me!", L"Debug", MB_OK);
#endif
    // Set the appropriate dialog properties.
    MainDialog dialog;
    DSNConfiguration dsnConfig("");
#if 0
    size_t siz = (*in_connSettingResponseMap).size();
    DSIConnSettingResponseMap::iterator itr;
    for(itr = in_connSettingResponseMap->begin(); itr != in_connSettingResponseMap->end(); itr++) {
        wstring first = itr->first.GetAsPlatformWString();
    }

    //dsnConfig.LoadConnectionSettings(*in_connSettingResponseMap);

    // Pop the dialog. False return code indicates the dialog was canceled.
    if (dialog.Show(in_parentWindow, dsnConfig, true))
    {
        dsnConfig.RetrieveConnectionSettings(*out_connSettingRequestMap);
        return std::pair<bool, bool> (true, false);
    }
#endif

    return std::pair<bool, bool> (true, false);
}

//==================================================================================================
/// @brief Implements logic common to ConfigDSN() and ConfigDSNW().
///
/// @param in_parentWindow      The parent window handle.
/// @param in_requestType       Contains the type of request, one of the following:
///                                 ODBC_ADD_DSN
///                                 ODBC_CONFIG_DSN
///                                 ODBC_REMOVE_DSN
///                                 ODBC_ADD_SYS_DSN
///                                 ODBC_CONFIG_SYS_DSN
///                                 ODBC_REMOVE_SYS_DSN
///                                 ODBC_REMOVE_DEFAULT_DSN
/// @param in_driverDescription The driver description (usually the name of the DBMS) presented to
///                             the user instead of the physical driver name.
/// @param in_attributes        A list of attributes in the form of key-value pairs. The pairs will
///                             take the form of <key>=<value> with \ used as the delimiter.
///                             An example would be "DSN=sample\UID=Simba\PWD=ODBC".
///
/// @return True if the function succeeds; false otherwise.
//==================================================================================================
BOOL INSTAPI ConfigDSNInternal(
    HWND in_parentWindow,
    WORD in_requestType,
    const simba_wstring& in_driverDescription,
    const simba_wstring& in_attributes)
{
#ifdef _DEBUG
    //MessageBox(in_parentWindow, L"Debug Me!", L"Debug", MB_OK);
#endif

    SimbaSettingReader::InitializeInstance(DRIVER_WINDOWS_BRANDING, L"Driver");

    try
    {
        DSNConfiguration dsnConfig(in_driverDescription);

        switch (in_requestType)
        {
            case ODBC_REMOVE_DSN:
            case ODBC_REMOVE_SYS_DSN:
            {
                dsnConfig.Load(in_attributes);

                simba_string dsnStr = dsnConfig.GetDSN().GetAsUTF8();

                // In case of failure, SQLRemoveDSNFromIniW posts the error message itself.
                wstring dsn = dsnConfig.GetDSN().GetAsPlatformWString();
                return SQLRemoveDSNFromIniW(dsn.c_str());
            }

            case ODBC_ADD_DSN:
            case ODBC_ADD_SYS_DSN:
            case ODBC_CONFIG_DSN:
            case ODBC_CONFIG_SYS_DSN:
            {
                if ((ODBC_ADD_DSN == in_requestType) ||
                    (ODBC_ADD_SYS_DSN == in_requestType))
                {
                    // Load the defaults for the configuration if adding a new DSN.
                    dsnConfig.LoadDefaults();
                }
                else
                {
                    // Load the settings from the registry as this DSN already existed.
                    dsnConfig.Load(in_attributes);

                    simba_string dsnStr = dsnConfig.GetDSN().GetAsUTF8();
                }

                MainDialog dialog;

                // Pop the dialog. False return code indicates the dialog was canceled.
                if (dialog.Show(in_parentWindow, dsnConfig))
                {
                    simba_string dsnStr = dsnConfig.GetDSN().GetAsUTF8();
                    dsnConfig.Save();
                }

                return TRUE;
            }

            default:
            {
                throw SetupException(SetupException::CFG_DLG_ERROR_INVALID_REQUEST_TYPE);
            }
        }
    }
    catch (ErrorException& driverError)
    {
        simba_wstring errorMsg;
        driverError.GetMessageKeyOrText(errorMsg);
        std::wstring errMsgWstr = errorMsg.GetAsPlatformWString();

        SQLPostInstallerErrorW(ODBC_ERROR_REQUEST_FAILED, errMsgWstr.c_str());
    }
    catch (SetupException& ex)
    {
        BaseDialog::ShowErrorDialog(in_parentWindow, ex.GetErrorMsg());

        std::wstring errMsgWstr = ex.GetErrorMsg().GetAsPlatformWString();
        DWORD errorCode = ex.GetErrorCode();

        // Ensure the error code is a valid ODBC error code.
        errorCode = simba_min(errorCode, ODBC_ERROR_REQUEST_FAILED);

        SQLPostInstallerErrorW(errorCode, errMsgWstr.c_str());
    }
    catch (...)
    {
        SetupException ex(SetupException::CFG_DLG_ERROR_REQUEST_FAILED);
        simba_wstring errorMsg = ex.GetErrorMsg();
        std::wstring errMsgWstr = errorMsg.GetAsPlatformWString();

        SQLPostInstallerErrorW(ex.GetErrorCode(), errMsgWstr.c_str());
    }

    return FALSE;
}

//==================================================================================================
/// @brief Adds, modifies, or deletes data sources from the system information. This is done through
/// a dialog which will prompt for user interaction.
///
/// @param in_parentWindow      The parent window handle.
/// @param in_requestType       Contains the type of request, one of the following:
///                                 ODBC_ADD_DSN
///                                 ODBC_CONFIG_DSN
///                                 ODBC_REMOVE_DSN
///                                 ODBC_ADD_SYS_DSN
///                                 ODBC_CONFIG_SYS_DSN
///                                 ODBC_REMOVE_SYS_DSN
///                                 ODBC_REMOVE_DEFAULT_DSN
/// @param in_driverDescription The driver description (usually the name of the DBMS) presented to
///                             the user instead of the physical driver name.
/// @param in_attributes        A list of attributes in the form of key-value pairs. The pairs will
///                             take the form of <key>=<value> with \ used as the delimiter.
///                             An example would be "DSN=sample\UID=Simba\PWD=ODBC".
///
/// @return True if the function succeeds; false otherwise.
//==================================================================================================
BOOL INSTAPI ConfigDSNW(
    HWND in_parentWindow,
    WORD in_requestType,
    LPCWSTR in_driverDescription,
    LPCWSTR in_attributes)
{
    simba_wstring driverDesc(in_driverDescription);
    simba_wstring attributes(in_attributes);

    return ConfigDSNInternal(in_parentWindow, in_requestType, driverDesc, attributes);
}

//==================================================================================================
/// @brief Adds, modifies, or deletes data sources from the system information. This is done through
/// a dialog which will prompt for user interaction.
///
/// @param in_parentWindow      The parent window handle.
/// @param in_requestType       Contains the type of request, one of the following:
///                                 ODBC_ADD_DSN
///                                 ODBC_CONFIG_DSN
///                                 ODBC_REMOVE_DSN
///                                 ODBC_ADD_SYS_DSN
///                                 ODBC_CONFIG_SYS_DSN
///                                 ODBC_REMOVE_SYS_DSN
///                                 ODBC_REMOVE_DEFAULT_DSN
/// @param in_driverDescription The driver description (usually the name of the DBMS) presented to
///                             the user instead of the physical driver name.
/// @param in_attributes        A list of attributes in the form of key-value pairs. The pairs will
///                             take the form of <key>=<value> with \ used as the delimiter.
///                             An example would be "DSN=sample\UID=Simba\PWD=ODBC".
///
/// @return True if the function succeeds; false otherwise.
//==================================================================================================
BOOL INSTAPI ConfigDSN(
    HWND in_parentWindow,
    WORD in_requestType,
    LPCSTR in_driverDescription,
    LPCSTR in_attributes)
{
    simba_wstring driverDesc(in_driverDescription);
    simba_wstring attributes(in_attributes);

    return ConfigDSNInternal(in_parentWindow, in_requestType, driverDesc, attributes);
}
