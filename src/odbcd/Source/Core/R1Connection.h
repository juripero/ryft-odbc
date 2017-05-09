// =================================================================================================
///  @file R1Connection.h
///
///  RyftOne DSIConnection implementation.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1CONNECTION_H_
#define _RYFTONE_R1CONNECTION_H_

#include "RyftOne.h"
#include "R1Catalog.h"
#include "DSIConnection.h"

namespace RyftOne
{
    /// @brief RyftOne DSIConnection implementation class.
    class R1Connection : public Simba::DSI::DSIConnection
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        ///
        /// @param in_environment           The parent IEnvironment. (NOT OWN)
        R1Connection(Simba::DSI::IEnvironment* in_environment);

        /// @brief Destructor.
        ~R1Connection();

        /// @brief Attempts to connect to the data source, using connection settings generated by a
        /// call to UpdateConnectionSettings(). 
        ///
        /// A call to Connect() occurs when the ODBC connection is formed, not when the connection 
        /// handle is allocated. If a Disconnect() call occurs, 
        /// Connect() may be called again if a new connection is made against the same handle.
        ///
        /// @param in_connectionSettings    Connection settings generated by a call to 
        ///                                 UpdateConnectionSettings().
        ///
        /// @exception ErrorException if there is a problem making a connection.
        /// @exception ErrorException with key DIAG_INVALID_AUTH_SPEC if there is a problem with 
        ///            authorization.
        virtual void Connect(const Simba::DSI::DSIConnSettingRequestMap& in_connectionSettings);

        /// @brief Creates and returns a new IStatement instance.
        ///
        /// @return New IStatement instance. (OWN)
        virtual Simba::DSI::IStatement* CreateStatement();

        /// @brief Disconnect from the data store.
        /// 
        /// This call is a request to the DSII layer to free up all memory that it has allocated
        /// for the data store, and to close any tables that are currently open for the data store.
        virtual void Disconnect();

        /// @brief Gets the ILogger reference for the connection.
        /// 
        /// @return ILogger instance. (NOT OWN)
        virtual Simba::Support::ILogger* GetLog();

        /// @brief Displays a dialog box that prompts the user for settings for this connection. 
        ///
        /// The connection settings from io_connectionSettings are presented as key-value string 
        /// pairs. The input connection settings map is the initial state of the dialog box.  The 
        /// input connection settings map will be modified to reflect the user's input to the 
        /// dialog box. All the key-value pairs returned in io_connectionSettings will be put 
        /// into the output connection string (when the called connection function was either 
        /// SQLDriverConnect or SQLBrowseConnect)
        /// 
        /// io_connResponseMap contains values that are still needed and were not supplied
        /// by the user. This map is returned from DSII layer of the driver when 
        /// UpdateConnectionSettings function of an IConnection object is called. The driver
        /// may use this map, for example, to indicate that some setting is missing or
        /// optional.
        ///
        /// The return value for this method indicates if the user completed the process by 
        /// clicking OK on the dialog box (return true), or if the user aborts the process by 
        /// clicking CANCEL on the dialog box (return false).
        ///
        /// The function will be called multiple times if the user clicked OK button but the 
        /// setting user input are incomplete. Before each time the function is called,
        /// UpdateConnectionSettings function mentioned above will be called to check whether
        /// there are still required settings not being set, and the response map will be 
        /// updated accordingly to reflect such information.
        /// 
        /// @param io_connResponseMap       The connection settings map updated with settings that
        ///                                 are still needed and were not supplied.
        /// @param io_connectionSettings    The connection settings map updated to reflect the
        ///                                 user's input.
        /// @param in_parentWindow          Handle to the parent window to which this dialog
        ///                                 belongs
        /// @param in_promptType            Indicates what type of connection settings to request -
        ///                                 either both required and optional settings or just
        ///                                 required settings.
        ///
        /// @return True if the user clicks OK on the dialog box; false if the user clicks CANCEL.
        virtual bool PromptDialog(
            Simba::DSI::DSIConnSettingResponseMap& in_connResponseMap,
            Simba::DSI::DSIConnSettingRequestMap& io_connectionSettings,
            HWND in_parentWindow,
            Simba::DSI::PromptType in_promptType);

        /// @brief Takes in an ODBC-formatted SQL query and returns it customized and optimized for 
        /// the data source.
        ///
        /// @param in_string                ODBC-formatted SQL query.
        /// @param out_string               Gets filled in with return value.
        virtual void ToNativeSQL(const simba_wstring& in_string, simba_wstring& out_string);

        /// @brief Updates the connection settings by checking if required settings are present.
        ///
        /// When connecting to a data source, SimbaODBC calls this function to check and update 
        /// settings for the connection. The in_connectionSettings parameter includes the settings 
        /// specified in the connection string presented as key-value string pairs. This method 
        /// should return any modified and additional requested connection settings in 
        /// out_connectionSettings.
        /// 
        /// If any connection settings have been modified, then an OPT_VAL_CHANGED warning must 
        /// be posted. If a key in the map is unrecognized or does not belong in this stage of 
        /// connection, then an INVALID_CONN_STR_ATTR warning must be posted in this method.
        /// 
        /// @param in_connectionSettings    The Connection Settings map containing the connection
        ///                                 settings that were passed into the connection string.
        /// @param out_connectionSettings   Gets filled in with connection settings that are still
        ///                                 needed and were not supplied in the connection string.
        virtual void UpdateConnectionSettings(
            const Simba::DSI::DSIConnSettingRequestMap& in_connectionSettings,
            Simba::DSI::DSIConnSettingResponseMap& out_connectionSettings);

    // Private =====================================================================================
    private:

        virtual void DoReset();

        /// @brief Set the driver's default property values.
        /// 
        /// This function overrides some of the default connection properties specified in
        /// DSIConnProperties.h.
        void SetConnectionPropertyValues();

        // The driver log. (NOT OWN)
        ILogger* m_log;

        RyftOne_Database m_ryft1;

        // True if this Connection is connected; false otherwise.
        bool m_isConnected;
    };
}

#endif
