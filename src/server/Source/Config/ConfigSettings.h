// =================================================================================================
///  @file ConfigSettings.h
///
///  Definition of the Class ConfigSettings
///
///  Copyright (C) 2009-2011 Simba Technologies Incorporated
// =================================================================================================

#ifndef _SIMBA_R1TRALIGHT_CONFIG_CONFIGSETTINGS_H_
#define _SIMBA_R1TRALIGHT_CONFIG_CONFIGSETTINGS_H_

#include "Simba.h"
#include <map>
#include <vector>

namespace Simba
{
namespace RyftOne
{

/// Odbc.ini subkeys.
/// NOTE: These keys must be inserted into the vector of configuration keys during construction,
///       so add new configuration keys to the vector via InitializeConfigurationKeys routine.
#define SETTING_DRIVER          "Driver"
#define SETTING_DSN             "DSN"
#define SETTING_DESCRIPTION     "DESCRIPTION"
#define SETTING_UID				"UID"
#define SETTING_PWD				"PWD"
#define SETTING_SETUP           "Setup"

    /// Configuration Map
    typedef std::map<simba_wstring, simba_wstring> ConfigurationMap;

    /// @brief This class encapsulates the settings for the configuration dialog.
    class ConfigSettings
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        ConfigSettings();

        /// @brief Destructor.
        ///
        /// Release locally held resources.
        ~ConfigSettings();

        /// @brief Get the DSN name.
        ///
        /// @return DSN name configured.
        const simba_wstring& GetDSN();

        /// @brief Get the description.
        ///
        /// @return Description configured.
        const simba_wstring& GetDescription();

		/// @brief Get the password.
		/// 
		/// @return Password configured.
		const simba_wstring& GetPWD();

		/// @brief Get the user name.
		/// 
		/// @return User name configured.
		const simba_wstring& GetUID();

        /// @brief Set the DSN name.
        ///
        /// @param in_dsn                       DSN name to configure.
        ///
        /// @exception ErrorException
        void SetDSN(const simba_wstring& in_dsn);

        /// @brief Set the description.
        ///
        /// @param in_description               Description to configure.
        void SetDescription(const simba_wstring& in_description);

		/// @brief Set the PWD.
		/// 
		/// @param in_pwd						Password to configure.
		void SetPWD(const simba_wstring& in_pwd);

		/// @brief Set the UID.
		/// 
		/// @param in_uid						The user id to configure.
		void SetUID(const simba_wstring& in_uid);

        /// @brief Updates the member variables and configuration map with odbc.ini configurations.
        ///
        /// The configuration map will contain the delta, whereas the custom configuration map 
        /// will be completely overwritten.
        void ReadConfigurationDriver();

        /// @brief Updates the member variables and configuration map with odbc.ini configurations.
        ///
        /// The configuration map will contain the delta, whereas the custom configuration map 
        /// will be completely overwritten.
        void ReadConfigurationDSN();

        /// @brief Updates the odbc.ini configurations from the configuration map.
        void WriteConfiguration();

        /// @brief Clears current configuration, resets all private data members to their default
        /// values, then updates the configuration map with those configurations specified in the
        /// in_dsnConfigurationMap.
        ///
        /// (No implicit update to odbc.ini)
        ///
        /// @param in_dsnConfigurationMap       This will at least hold the DSN entry, which can be 
        ///                                     used to load the other settings.
        void UpdateConfiguration(ConfigurationMap& in_dsnConfigurationMap);

    // Private =====================================================================================
    private:
        /// Configuration map of odbc.ini subkeys and values
        ConfigurationMap m_configuration;

        /// The driver for this DSN.
        simba_wstring m_driver;

        /// The DSN name.
        simba_wstring m_dsn;

        /// The original DSN name; used to determine if the DSN name has changed.
        simba_wstring m_dsnOrig;

        /// @brief Reads all the settings for the given section
        ///
        /// @param sectionName                  The DSN or Driver section name
        /// @param filename                     The filename to read from, should be odbc.ini or odbcinst.ini
        ///
        /// @return A map of all settings in the section
        ConfigurationMap ReadSettings(const std::wstring& sectionName, const std::wstring& filename);
    };
}
}

#endif
