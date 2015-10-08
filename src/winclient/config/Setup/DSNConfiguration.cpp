//==================================================================================================
///  @file DSNConfiguration.cpp
///
///  Implementation of the class DSNConfiguration.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
//==================================================================================================

#include "DSNConfiguration.h"

#include "AutoArrayPtr.h"
#include "ConfigurationReader.h"
#include "DSIFileLogger.h"
#include "NumberConverter.h"
#include "SimbaSettingReaderConstants.h"
#include "DialogConsts.h"
#include "SetupException.h"

#include <odbcinst.h>

using namespace Ryft;
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
        throw SetupException(SetupException::CFG_DLG_ERROR_REGISTRY_WRITE);
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
        throw SetupException(SetupException::CFG_DLG_ERROR_REGISTRY_WRITE);
    }

    RegCloseKey(key);
}

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
DSNConfiguration::DSNConfiguration(
    const simba_wstring& in_driverDescription ) : 
        m_driver(in_driverDescription),
        m_configType(CT_NEW_DSN)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigType DSNConfiguration::GetConfigType() const
{
    return m_configType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& DSNConfiguration::GetDescription() const
{
    GET_VALUE(DESC_KEY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& DSNConfiguration::GetDriver() const
{
    return m_driver;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& DSNConfiguration::GetDSN() const
{
    GET_VALUE(DSN_CONFIG_KEY_DSN);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& DSNConfiguration::GetLogLevel() const
{
    GET_VALUE(SETTING_LOGLEVEL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& DSNConfiguration::GetLogNamespace() const
{
    GET_VALUE(SETTING_LOGNAMESPACE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& DSNConfiguration::GetLogPath() const
{
    GET_VALUE(SETTING_LOGPATH);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& DSNConfiguration::GetPassword() const
{
    GET_VALUE(RYFT_PWD_KEY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& DSNConfiguration::GetUser() const
{
    GET_VALUE(RYFT_UID_KEY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& DSNConfiguration::GetURL() const
{
    GET_VALUE(RYFT_URL_KEY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const simba_wstring& DSNConfiguration::GetPort() const
{
    GET_VALUE(RYFT_PORT_KEY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::Load(const simba_wstring& in_attributeString)
{
    m_configType = CT_EXISTING_DSN;

    // Loading the defaults will also load the logging properties.
    LoadDefaults();

    // NOTE: only DSN attribute is set.
    ParseDSNAttribute(in_attributeString);
    wstring dsn = GetDSN().GetAsPlatformWString();

    if (FALSE == SQLValidDSNW(dsn.c_str()))
    {
        throw SetupException(SetupException::CFG_DLG_ERROR_INVALID_KEYWORD_VALUE);
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
void DSNConfiguration::LoadConnectionSettings(
    const DSIConnSettingRequestMap& in_connectionSettings)
{
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
void DSNConfiguration::LoadDefaults()
{
    m_configuration[DSN_CONFIG_KEY_DRIVER] = m_driver;

    // Load the proper logging properties even if this is new.
    LoadLoggingProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::RetrieveConnectionSettings(
    DSIConnSettingRequestMap& out_connectionSettings)
{
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
void DSNConfiguration::Save()
{
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
                throw SetupException(SetupException::CFG_DLG_ERROR_REQUEST_FAILED);
            }
        }

        // Write the Servers registry key for Simba
        wchar_t servers[512];
        _snwprintf(servers, 512, L"%s %s,", GetURL().GetAsPlatformWString().c_str(), GetPort().GetAsPlatformWString().c_str());
        SQLWritePrivateProfileStringW(dsn.c_str(), L"SERVERS", servers, L"ODBC.INI");

        // Write out the logging settings.
        SetDriverRegistryValue(SETTING_LOGLEVEL, GetLogLevel());
        SetDriverRegistryValue(SETTING_LOGNAMESPACE, GetLogNamespace());
        SetDriverRegistryValue(SETTING_LOGPATH, GetLogPath());
        SetDriverRegistryValue(SETTING_LOGFILECOUNT, "3");
        SetDriverRegistryValue(SETTING_LOGFILESIZE, "20971520");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::SetDescription(const simba_wstring& in_description)
{
    m_configuration[DESC_KEY] = in_description;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::SetDSN(const simba_wstring& in_dsn)
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
        throw SetupException(SetupException::CFG_DLG_ERROR_REQUEST_FAILED);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::SetLogLevel(const simba_wstring& in_level)
{
    m_configuration[SETTING_LOGLEVEL] = in_level;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::SetLogNamespace(const simba_wstring& in_namespace)
{
    m_configuration[SETTING_LOGNAMESPACE] = in_namespace;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::SetLogPath(const simba_wstring& in_path)
{
    m_configuration[SETTING_LOGPATH] = in_path;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::SetPassword(const simba_wstring& in_pwd)
{
    m_configuration[RYFT_PWD_KEY] = in_pwd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::SetUser(const simba_wstring& in_user)
{
    m_configuration[RYFT_UID_KEY] = in_user;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::SetURL(const simba_wstring& in_URL)
{
    m_configuration[RYFT_URL_KEY] = in_URL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::SetPort(const simba_wstring& in_Port)
{
    m_configuration[RYFT_PORT_KEY] = in_Port;
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::LoadLoggingProperties()
{
    // Get the logging level, and ensure it fits one of the expected values.
    simba_wstring logLevel = GetDriverRegistryValue(SETTING_LOGLEVEL, "0");
    logLevel = NumberConverter::ConvertInt32ToWString(
        Simba::DSI::DSIFileLogger::ConvertStringToLogLevel(logLevel.GetAsUTF8()));

    m_configuration[SETTING_LOGLEVEL] = logLevel;
    m_configuration[SETTING_LOGNAMESPACE] = GetDriverRegistryValue(SETTING_LOGNAMESPACE, "");
    m_configuration[SETTING_LOGPATH] = GetDriverRegistryValue(SETTING_LOGPATH, "");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void DSNConfiguration::ParseDSNAttribute(const simba_wstring& in_attributeString)
{
    simba_string attributeStr = in_attributeString.GetAsUTF8();

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
