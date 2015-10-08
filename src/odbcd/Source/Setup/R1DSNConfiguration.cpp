//==================================================================================================
///  @file R1DSNConfiguration.cpp
///
///  Implementation of the class R1DSNConfiguration.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
//==================================================================================================

#include "R1DSNConfiguration.h"

#include "AutoArrayPtr.h"
#include "ConfigurationReader.h"
#include "DSIFileLogger.h"
#include "NumberConverter.h"
#include "SimbaSettingReaderConstants.h"
#include "R1DialogConsts.h"
#include "R1SetupException.h"

#include <odbcinst.h>

using namespace RyftOne;
using namespace Simba::DSI;
using namespace std;

// Static =========================================================================================
static const simba_wstring DSN_CONFIG_KEY_DSN(L"DSN");
static const simba_wstring DSN_CONFIG_KEY_DRIVER(L"Driver");
#define BYTES_PER_MB 1048576U

// Macros ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Retrieve the specified value from the configurations.
///
/// @param key      The key to look up in the configurations.
////////////////////////////////////////////////////////////////////////////////////////////////////
#define GET_VALUE(key)                                                                             \
{                                                                                                  \
    ConfigurationMap::const_iterator itr = m_configuration.find(key);                              \
    if (itr == m_configuration.end())                                                              \
    {                                                                                              \
        return s_emptyVal;                                                                         \
    }                                                                                              \
    return itr->second;                                                                            \
}

// Helpers =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Get a driver registry value.
///
/// @param in_key                   The key of the value to get. (NOT OWN)
/// @param in_default               The default value to use if the key isn't present. 
///                                 (NOT OWN)
///
/// @return The value fetched from the driver registry.
////////////////////////////////////////////////////////////////////////////////////////////////////
simba_wstring GetDriverRegistryValue(
    const simba_char* in_key, 
    const simba_char* in_default)
{
    simba_wstring path(L"SOFTWARE\\");
    path += DRIVER_WINDOWS_BRANDING;
    path += L"\\Driver";

    // Load the driver registry value via the Configuration reader.
    ConfigurationReader reader;
    SectionConfigMap configMap;
    reader.LoadConfiguration(configMap, L"HKEY_LOCAL_MACHINE", path);

    SectionConfigMap::iterator itr = configMap.find(in_key);
    if (configMap.end() == itr)
    {
        // Return the default value if the value isn't found.
        return in_default;
    }

    return itr->second.GetWStringValue();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Set a driver value in the registry.
///
/// @param in_key                   The key of the value to set. (NOT OWN)
/// @param in_value                 The value to set in the registry.
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetDriverRegistryValue(const simba_char* in_key, const simba_wstring& in_value)
{
    simba_wstring path(L"SOFTWARE\\");
    path += DRIVER_WINDOWS_BRANDING;
    path += L"\\Driver";
    std::wstring wPath = path.GetAsPlatformWString();

    // Open the registry key.
    HKEY key;
    if (RegCreateKey(HKEY_LOCAL_MACHINE, wPath.c_str(), &key))
    {
        throw R1SetupException(R1SetupException::CFG_DLG_ERROR_REGISTRY_WRITE);
    }

    // Write the value.
    std::wstring setting = simba_wstring(in_key).GetAsPlatformWString();
    std::wstring value = in_value.GetAsPlatformWString();
    DWORD valueLength = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    if (RegSetValueEx(
            key, 
            setting.c_str(), 
            0, 
            REG_SZ, 
            reinterpret_cast<const BYTE*>(value.c_str()), 
            valueLength))
    {
        RegCloseKey(key);
        throw R1SetupException(R1SetupException::CFG_DLG_ERROR_REGISTRY_WRITE);
    }

    RegCloseKey(key);
}

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1DSNConfiguration::R1DSNConfiguration(
    const simba_wstring& in_driverDescription,
    ILogger* in_logger) : 
        m_driver(in_driverDescription),
        m_logger(in_logger),
        m_configType(CT_NEW_DSN)
{
    assert(m_logger);
    m_logger->LogFunctionEntrance("Simba::RyftOne", "R1DSNConfiguration", "R1DSNConfiguration");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigType R1DSNConfiguration::GetConfigType() const
{
    return m_configType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetDescription() const
{
    GET_VALUE(DESC_KEY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetDriver() const
{
    return m_driver;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetDSN() const
{
    GET_VALUE(DSN_CONFIG_KEY_DSN);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetLanguage() const
{
    GET_VALUE(R1_LNG_KEY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetLogLevel() const
{
    GET_VALUE(SETTING_LOGLEVEL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetLogMaxNum() const
{
    GET_VALUE(SETTING_LOGFILECOUNT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetLogMaxSize() const
{
    GET_VALUE(SETTING_LOGFILESIZE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetLogNamespace() const
{
    GET_VALUE(SETTING_LOGNAMESPACE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetLogPath() const
{
    GET_VALUE(SETTING_LOGPATH);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetPassword() const
{
    GET_VALUE(R1_PWD_KEY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& R1DSNConfiguration::GetUser() const
{
    GET_VALUE(R1_UID_KEY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::Load(const simba_wstring& in_attributeString)
{
    m_logger->LogFunctionEntrance("Simba::RyftOne", "R1DSNConfiguration", "Load");

    m_configType = CT_EXISTING_DSN;

    // Loading the defaults will also load the logging properties.
    LoadDefaults();

    // NOTE: only DSN attribute is set.
    ParseDSNAttribute(in_attributeString);
    wstring dsn = GetDSN().GetAsPlatformWString();

    if (FALSE == SQLValidDSNW(dsn.c_str()))
    {
        throw R1SetupException(R1SetupException::CFG_DLG_ERROR_INVALID_KEYWORD_VALUE);
    }

    if (!dsn.empty())
    {
        wstring defaultValue;
        AutoArrayPtr<wchar_t> keyBuf(4096);
        simba_int32 keyBufLen = 0;

        // Choose 32KB as max size to ensure the loop will always terminate.
        while (keyBuf.GetLength() < SIMBA_INT16_MAX)
        {
            keyBufLen = SQLGetPrivateProfileStringW(
                dsn.c_str(),            // The DSN or Driver to check.
                NULL,                   // NULL means get key names in this DSN or Driver.
                defaultValue.c_str(),   // Default value of empty string is unused for the case where getting key names.
                keyBuf.Get(),           // Buffer to store key names in.
                static_cast<simba_int32>(keyBuf.GetLength()),// Length of the buffer.
                L"odbc.ini");           // Section/filename to get the keys/values from (ODBC.INI or ODBCINST.INI).

            if (keyBufLen >= static_cast<simba_int32>(keyBuf.GetLength() - 1))
            {
                // SQLGetPrivateProfileString returns bufLen - 1 if it fills the buffer completely. 
                // In this case, loop and retry with a larger buffer to see if it has more keys.
                keyBuf.Attach(new wchar_t[keyBuf.GetLength() * 2], keyBuf.GetLength() * 2);
            }
            else
            {
                break;
            }
        }

        if (keyBufLen > 0)
        {
            // Use 4096 as initial buffer length.
            AutoArrayPtr<wchar_t> valBuf(4096);
            simba_int32 valBufLen = 0;

            wchar_t* keyNamePtr = keyBuf.Get();

            while (*keyNamePtr)
            {
                // Null terminate the empty buffer in case no characters are copied into it.
                *valBuf.Get() = '\0';

                while (valBuf.GetLength() < SIMBA_INT16_MAX)
                {
                    valBufLen = SQLGetPrivateProfileStringW(
                        dsn.c_str(),            // The DSN to check.
                        keyNamePtr,             // Get a value for a particular key
                        defaultValue.c_str(),   // Default value of empty string is unused because we know the key must exist.
                        valBuf.Get(),           // Buffer to store the value in.
                        static_cast<simba_int32>(valBuf.GetLength()),// Length of the buffer.
                        L"odbc.ini");           // Section/filename to get the keys/values from (ODBC.INI or ODBCINST.INI).

                    if (valBufLen >= static_cast<simba_int32>(valBuf.GetLength() - 1))
                    {
                        // SQLGetPrivateProfileString returns bufLen - 1 if it fills the buffer 
                        // completely. In this case, loop and retry with a larger buffer
                        valBuf.Attach(new wchar_t[valBuf.GetLength() * 2], valBuf.GetLength() * 2);
                    }
                    else
                    {
                        break;
                    }
                }

                simba_wstring keyName(keyNamePtr);
                m_configuration[keyName] = simba_wstring(valBuf.Get());
                keyNamePtr += keyName.GetLength() + 1;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::LoadConnectionSettings(
    const DSIConnSettingRequestMap& in_connectionSettings)
{
    ENTRANCE_LOG(m_logger, "Simba::RyftOne", "R1DSNConfiguration", "LoadConnectionSettings");

    m_configuration.clear();

    for (DSIConnSettingRequestMap::const_iterator itr = in_connectionSettings.begin();
         itr != in_connectionSettings.end();
         ++itr)
    {
        m_configuration.insert(ConfigurationMap::value_type(
            itr->first, 
            itr->second.GetWStringValue()));
    }

    m_configType = CT_CONNECTION;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::LoadDefaults()
{
    m_logger->LogFunctionEntrance("Simba::RyftOne", "R1DSNConfiguration", "LoadDefaults");

    m_configuration[DSN_CONFIG_KEY_DRIVER] = m_driver;

    // Load the proper logging properties even if this is new.
    LoadLoggingProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::RetrieveConnectionSettings(
    DSIConnSettingRequestMap& out_connectionSettings)
{
    ENTRANCE_LOG(m_logger, "Simba::RyftOne", "R1DSNConfiguration", "RetrieveConnectionSettings");

    out_connectionSettings.clear();

    for (ConfigurationMap::const_iterator itr = m_configuration.begin();
         itr != m_configuration.end();
         ++itr)
    {
        out_connectionSettings.insert(DSIConnSettingRequestMap::value_type(
            itr->first, 
            itr->second));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::Save()
{
    m_logger->LogFunctionEntrance("Simba::RyftOne", "R1DSNConfiguration", "Save");

    // Add data source to the system information.
    // If the data source specification section already exists, SQLWriteDSNToIni removes the old 
    // section before creating the new one.
    const wstring dsn = GetDSN().GetAsPlatformWString();
    const wstring driver = GetDriver().GetAsPlatformWString();

    if (SQLWriteDSNToIniW(dsn.c_str(), driver.c_str()))
    {
        // Write the configuration settings to odbc.ini.
        for (ConfigurationMap::iterator itr = m_configuration.begin();
             itr != m_configuration.end();
             ++itr)
        {
            if (itr->first.IsEqual(SETTING_LOGLEVEL, false) ||
                itr->first.IsEqual(SETTING_LOGNAMESPACE, false) ||
                itr->first.IsEqual(SETTING_LOGPATH, false) ||
                itr->first.IsEqual(SETTING_LOGFILECOUNT, false) ||
                itr->first.IsEqual(SETTING_LOGFILESIZE, false) ||
                itr->first.IsEqual(DSN_CONFIG_KEY_DSN, false))
            {
                // Don't write the DSN, or logging settings.
                continue;
            }

            wstring key = itr->first.GetAsPlatformWString();
            if (key.empty())
            {
                // Remove this value from odbc.ini.
                SQLWritePrivateProfileStringW(dsn.c_str(), key.c_str(), NULL, L"ODBC.INI");

                continue;
            }

            wstring value = itr->second.GetAsPlatformWString();

            if (!SQLWritePrivateProfileStringW(
                    dsn.c_str(),
                    key.c_str(),
                    value.c_str(),
                    L"ODBC.INI"))
            {
                // An error occurred.
                throw R1SetupException(R1SetupException::CFG_DLG_ERROR_REQUEST_FAILED);
            }
        }

        // Write out the logging settings.
        SetDriverRegistryValue(SETTING_LOGLEVEL, GetLogLevel());
        SetDriverRegistryValue(SETTING_LOGNAMESPACE, GetLogNamespace());
        SetDriverRegistryValue(SETTING_LOGPATH, GetLogPath());
        SetDriverRegistryValue(SETTING_LOGFILECOUNT, GetLogMaxNum());

        // Convert from MB to bytes.
        simba_uint32 size = NumberConverter::ConvertWStringToUInt32(GetLogMaxSize());
        SetDriverRegistryValue(
            SETTING_LOGFILESIZE,
            NumberConverter::ConvertToWString(size * BYTES_PER_MB));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::SetDescription(const simba_wstring& in_description)
{
    m_configuration[DESC_KEY] = in_description;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::SetDSN(const simba_wstring& in_dsn)
{
    if ((CT_CONNECTION == m_configType) &&
        in_dsn.IsEmpty())
    {
        // Don't do any validation if only doing a connection dialog as the DSN doesn't matter.
        return;
    }

    wstring dsnWstr = in_dsn.GetAsPlatformWString();   

    if (SQLValidDSNW(dsnWstr.c_str()))
    {
        m_configuration[DSN_CONFIG_KEY_DSN] = in_dsn;
    }
    else
    {
        throw R1SetupException(R1SetupException::CFG_DLG_ERROR_REQUEST_FAILED);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::SetLanguage(const simba_wstring& in_language)
{
    m_configuration[R1_LNG_KEY] = in_language;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::SetLogLevel(const simba_wstring& in_level)
{
    m_configuration[SETTING_LOGLEVEL] = in_level;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::SetLogMaxNum(const simba_wstring& in_maxNum)
{
    try
    {
        if (!in_maxNum.IsEmpty())
        {
            NumberConverter::ConvertWStringToUInt32(in_maxNum);
        }

        m_configuration[SETTING_LOGFILECOUNT] = in_maxNum;
    }
    catch (ErrorException&)
    {
        throw R1SetupException(R1SetupException::CFG_DLG_ERROR_INVALID_MAX_LOG_NUM);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::SetLogMaxSize(const simba_wstring& in_maxSize)
{
    try
    {
        if (!in_maxSize.IsEmpty() && 
            (4096 < NumberConverter::ConvertWStringToUInt16(in_maxSize)))
        {
            // Restrict the size of the log files to 4096 MB, since the log rotation code will 
            // simply clamp to this size anyway.
            throw R1SetupException(R1SetupException::CFG_DLG_ERROR_INVALID_MAX_LOG_SIZE);
        }

        m_configuration[SETTING_LOGFILESIZE] = in_maxSize;
    }
    catch (ErrorException&)
    {
        throw R1SetupException(R1SetupException::CFG_DLG_ERROR_INVALID_MAX_LOG_SIZE);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::SetLogNamespace(const simba_wstring& in_namespace)
{
    m_configuration[SETTING_LOGNAMESPACE] = in_namespace;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::SetLogPath(const simba_wstring& in_path)
{
    m_configuration[SETTING_LOGPATH] = in_path;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::SetPassword(const simba_wstring& in_pwd)
{
    m_configuration[R1_PWD_KEY] = in_pwd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::SetUser(const simba_wstring& in_user)
{
    m_configuration[R1_UID_KEY] = in_user;
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::LoadLoggingProperties()
{
    m_logger->LogFunctionEntrance("Simba::RyftOne", "R1DSNConfiguration", "LoadLoggingProperties");

    // Get the logging level, and ensure it fits one of the expected values.
    simba_wstring logLevel = GetDriverRegistryValue(SETTING_LOGLEVEL, "0");
    logLevel = NumberConverter::ConvertInt32ToWString(
        Simba::DSI::DSIFileLogger::ConvertStringToLogLevel(logLevel.GetAsUTF8()));

    m_configuration[SETTING_LOGLEVEL] = logLevel;
    m_configuration[SETTING_LOGNAMESPACE] = GetDriverRegistryValue(SETTING_LOGNAMESPACE, "");
    m_configuration[SETTING_LOGPATH] = GetDriverRegistryValue(SETTING_LOGPATH, "");
    m_configuration[SETTING_LOGFILECOUNT] = GetDriverRegistryValue(
        SETTING_LOGFILECOUNT,
        NumberConverter::ConvertToString(DSI_FILELOGGER_DEFAR1T_MAXFILECOUNT).c_str());

    simba_wstring size = GetDriverRegistryValue(
        SETTING_LOGFILESIZE,
        NumberConverter::ConvertToString(DSI_FILELOGGER_DEFAR1T_MAXFILESIZE).c_str());
    try
    {
        // Convert from bytes to MB.
        simba_uint32 intSize = NumberConverter::ConvertWStringToUInt32(size);
        size = NumberConverter::ConvertToWString(intSize / BYTES_PER_MB);
    }
    catch (...)
    {
        // Use the default if something goes wrong.
        size = NumberConverter::ConvertToWString(DSI_FILELOGGER_DEFAR1T_MAXFILESIZE / BYTES_PER_MB);
    }

    m_configuration[SETTING_LOGFILESIZE] = size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1DSNConfiguration::ParseDSNAttribute(const simba_wstring& in_attributeString)
{
    m_logger->LogFunctionEntrance("Simba::RyftOne", "R1DSNConfiguration", "ParseDSNAttribute");
    simba_string attributeStr = in_attributeString.GetAsUTF8();
    m_logger->LogInfo(
        "Simba::RyftOne", 
        "R1DSNConfiguration", 
        "ParseDSNAttribute",
        "%s",
        attributeStr.c_str());

    if (!in_attributeString.IsEmpty())
    {
        static const wstring pairDelimiter = L"\\";
        static const wstring entryDelimiter = L"=";

        wstring attributeString = in_attributeString.GetAsPlatformWString();

        // Split the attribute string into key/value pairs, so each entry will have a single 
        // key/value.
        simba_size_t beginPairSeparator = 0;
        simba_size_t endPairSeparator = attributeString.find(pairDelimiter);

        while (beginPairSeparator != endPairSeparator)
        {
            wstring entry = attributeString.substr(beginPairSeparator, endPairSeparator);

            // Split this pair into a key and value.
            simba_size_t entrySeparator = entry.find(entryDelimiter);

            wstring key;

            if (wstring::npos != entrySeparator)
            {
                key = entry.substr(0, entrySeparator);
                if (key == DSN_CONFIG_KEY_DSN.GetAsPlatformWString())
                {
                    wstring dsn(entry.substr(entrySeparator + 1));
                    SetDSN(dsn.c_str());
                    break;
                }
            }

            // Advance to the next pair.
            beginPairSeparator = endPairSeparator;
            endPairSeparator = attributeString.find(pairDelimiter, endPairSeparator + 1);
        }
    }
}
