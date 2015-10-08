//==================================================================================================
///  @file ConfigDSN.cpp
///
///  Implementation of ConfigDSN and ConfigDSNW
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
//==================================================================================================

#include "AutoPtr.h"
#include "DSIDriverFactory.h"
#include "DSIDriverSingleton.h"
#include "IDriver.h"
#include "SimbaSettingReader.h"
#include "R1DSNConfiguration.h"
#include "R1MainDialog.h"
#include "R1SetupException.h"

#include <odbcinst.h>

using namespace RyftOne;
using namespace Simba::DSI;
using namespace std;

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
///                                 ODBC_REMOVE_DEFAR1T_DSN
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
    SimbaSettingReader::InitializeInstance(DRIVER_WINDOWS_BRANDING, L"Driver");
    simba_handle handle = 0;
    IDriver* driver = DSIDriverFactory(handle);
    DSIDriverSingleton::SetInstance(driver, handle);

    ILogger* driverLog = driver->GetDriverLog();
    driverLog->LogFunctionEntrance("Simba::RyftOne", "ConfigDSN", "ConfigDSNInternal");

    try
    {
        R1DSNConfiguration dsnConfig(in_driverDescription, driverLog);

        switch (in_requestType)
        {
            case ODBC_REMOVE_DSN:
            case ODBC_REMOVE_SYS_DSN:
            {
                dsnConfig.Load(in_attributes);

                simba_string dsnStr = dsnConfig.GetDSN().GetAsUTF8();
                driverLog->LogInfo(
                    "Simba::RyftOne", 
                    "ConfigDSN", 
                    "ConfigDSNInternal", 
                    "Removing DSN: %s",
                    dsnStr.c_str());

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
                    driverLog->LogInfo(
                        "Simba::RyftOne", 
                        "ConfigDSN", 
                        "ConfigDSNInternal", 
                        "Adding new DSN.");

                    // Load the defaults for the configuration if adding a new DSN.
                    dsnConfig.LoadDefaults();
                }
                else
                {
                    driverLog->LogInfo(
                        "Simba::RyftOne", 
                        "ConfigDSN", 
                        "ConfigDSNInternal", 
                        "Modifying existing DSN.");

                    // Load the settings from the registry as this DSN already existed.
                    dsnConfig.Load(in_attributes);

                    simba_string dsnStr = dsnConfig.GetDSN().GetAsUTF8();
                    driverLog->LogInfo(
                        "Simba::RyftOne", 
                        "ConfigDSN", 
                        "ConfigDSNInternal", 
                        "Loaded settings for DSN: %s",
                        dsnStr.c_str());
                }

                R1MainDialog dialog;

                // Pop the dialog. False return code indicates the dialog was canceled.
                if (dialog.Show(in_parentWindow, dsnConfig))
                {
                    simba_string dsnStr = dsnConfig.GetDSN().GetAsUTF8();
                    driverLog->LogInfo(
                        "Simba::RyftOne", 
                        "ConfigDSN", 
                        "ConfigDSNInternal", 
                        "Saving settings for DSN: %s",
                        dsnStr.c_str());
                    dsnConfig.Save();
                }

                return TRUE;
            }

            default:
            {
                throw R1SetupException(R1SetupException::CFG_DLG_ERROR_INVALID_REQUEST_TYPE);
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
    catch (R1SetupException& ex)
    {
        R1BaseDialog::ShowErrorDialog(in_parentWindow, ex.GetErrorMsg());

        std::wstring errMsgWstr = ex.GetErrorMsg().GetAsPlatformWString();
        DWORD errorCode = ex.GetErrorCode();

        // Ensure the error code is a valid ODBC error code.
        errorCode = simba_min(errorCode, ODBC_ERROR_REQUEST_FAILED);

        SQLPostInstallerErrorW(errorCode, errMsgWstr.c_str());
    }
    catch (...)
    {
        R1SetupException ex(R1SetupException::CFG_DLG_ERROR_REQUEST_FAILED);
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
///                                 ODBC_REMOVE_DEFAR1T_DSN
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
///                                 ODBC_REMOVE_DEFAR1T_DSN
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
