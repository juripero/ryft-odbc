// =================================================================================================
///  @file R1Connection.cpp
///
///  RyftOne DSIConnection implementation.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1Connection.h"

#include "DSIPropertyUtilities.h"
#include "IDriver.h"
#include "IEnvironment.h"
#include "ILogger.h"
#include "R1Statement.h"

#if !defined(SERVERTARGET) && (defined(WIN32) || defined(_WIN64))
    #include "R1ConnectionDialog.h"
#endif

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1Connection::R1Connection(IEnvironment* in_environment) :
    DSIConnection(in_environment),
    m_log(in_environment->GetLog()),
    m_isConnected(false),
    m_ryft1(m_log)
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1Connection", "R1Connection");

    // Default connection properties are set by the DSIConnection parent class, however they can be
    // overridden as in SetConnectionPropertyValues. See DSIConnProperties.h for more information on
    // default properties. Additionally, custom connection properties can be added, see 
    // GetCustomProperty(), GetCustomPropertyType(), IsCustomProperty(), and SetCustomProperty() on 
    // DSIConnection.
    SetConnectionPropertyValues();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1Connection::~R1Connection()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Connection::Connect(const DSIConnSettingRequestMap& in_connectionSettings)
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1Connection", "Connect");

    // The SimbaEngine SDK will call UpdateConnectionSettings() prior to calling this function.
    // This will ensure that all valid keys required to create a connection have been specified
    // and are stored within in_connectionSettings. When this function has has been called, all of
    // the required keys that were marked in UpdateConnectionSettings() are present in 
    // in_connectionSettings map, while the optional keys may or may not be present. Use
    // GetRequiredSetting()/GetOptionalSetting() to easily access key values in the map.
    //
    // This method should validate each of the incoming values and throw an error in the event
    // of an invalid value.

    // Store the DSN settings for use later, such as in retrieving the DSN that has been connected
    // to, if one has been used.
    m_connectionSettingsMap = in_connectionSettings;

    const Variant& uidVar = GetRequiredSetting(R1_UID_KEY, in_connectionSettings);
    string uid = uidVar.GetStringValue();

    string pwd;
    const Variant* pwdVar;
    if(GetOptionalSetting(R1_PWD_KEY, in_connectionSettings, &pwdVar))
        pwd = pwdVar->GetStringValue();

    if(!m_ryft1.logon(uid, pwd))
        R1THROW(Simba::Support::DIAG_INVALID_AUTH_SPEC, L"AuthorizationFailed");

    SetProperty(DSI_CONN_USER_NAME, AttributeData::MakeNewWStringAttributeData(uidVar.GetWStringValue()));

    m_isConnected = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IStatement* R1Connection::CreateStatement()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1Connection", "CreateStatement");
    return new R1Statement(this, &m_ryft1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Connection::Disconnect()
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1Connection", "Disconnect");
    m_ryft1.logoff();
    m_isConnected = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ILogger* R1Connection::GetLog()
{
#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #4: Set the connection-wide logging details.")
    // This implementation uses one log for the entire driver, however it's possible to also use a
    // separate log for each connection, thus enabling better debugging of multiple connections
    // at once.
    return m_log;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1Connection::PromptDialog(
    DSIConnSettingResponseMap& in_connResponseMap,
    DSIConnSettingRequestMap& io_connectionSettings,
    HWND in_parentWindow,
    PromptType in_promptType)
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1Connection", "PromptDialog");

    // Avoid compiler warnings.
    UNUSED(in_connResponseMap);

#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #6: Customize DriverPrompt Dialog.")
    // This driver implements a sample driver connect dialog which asks for the UID, PWD and LNG.
    // Modify this dialog to suit your needs. Keep in mind that changing the dialog will also affect 
    // the code in UpdateConnectionSettings.

#if !defined(SERVERTARGET) && (defined(WIN32) || defined(_WIN64))

    return R1ConnectionDialog::Prompt(
        io_connectionSettings,
        m_log,
        in_parentWindow,
        (PROMPT_REQUIRED == in_promptType));

#else

    UNUSED(in_parentWindow);
    UNUSED(in_promptType);

    io_connectionSettings[R1_UID_KEY] = Variant(simba_wstring(""));
    io_connectionSettings[R1_PWD_KEY] = Variant(simba_wstring(""));

    return false;

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Connection::UpdateConnectionSettings(
    const DSIConnSettingRequestMap& in_connectionSettings,
    DSIConnSettingResponseMap& out_connectionSettings)
{
    ENTRANCE_LOG(m_log, "Simba::RyftOne", "R1Connection", "UpdateConnectionSettings");

#pragma message (__FILE__ "(" MACRO_TO_STRING(__LINE__) ") : TODO #5: Check Connection Settings.")
    // This function will receive all incoming connection settings as specified in the DSN used to 
    // establish the connection.  Use the VerifyRequiredSetting()/VerifyOptionalSetting() utility
    // functions to ensure that all required and optional keys are present to establish a 
    // connection. If any required values are missing, then the driver will fail to connect or call 
    // PromptDialog(), depending on the connection settings. If all required values are present,
    // then Connect() will be called.
    //
    // If a key is missing from the DSN, then add it to the out_connectionSettings.
    //
    // Data validation for the keys should be done in Connect()
    //
    // This driver has the following settings:
    //      Required Key: UID - represents a name of a user, could be anything.
    //      Required Key: PWD - represents the password, could be anything.
    VerifyRequiredSetting(R1_UID_KEY, in_connectionSettings, out_connectionSettings);
    if(m_ryft1.getAuthRequired()) {
        VerifyRequiredSetting(R1_PWD_KEY, in_connectionSettings, out_connectionSettings);
    }
    else 
        VerifyOptionalSetting(R1_PWD_KEY, in_connectionSettings, out_connectionSettings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Connection::ToNativeSQL(const simba_wstring& in_string, simba_wstring& out_string)
{
    INFO_LOG(m_log, "Simba::RyftOne", "R1Connection", "ToNativeSQL", "Execute SQL=%s", in_string.GetAsPlatformString().c_str());
    out_string = in_string;
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void R1Connection::SetConnectionPropertyValues()
{
	DSIPropertyUtilities::SetReadOnly(this, false);
    DSIPropertyUtilities::SetSchemaSupport(this, false);

    // Note that DSI_CONN_SERVER_NAME and DSI_CONN_USER_NAME should be updated after connection to 
    // reflect the server that was connected to and the user that connected.
	SetProperty(DSI_CONN_DBMS_NAME, AttributeData::MakeNewWStringAttributeData("RyftOne"));
    SetProperty(DSI_CONN_SERVER_NAME, AttributeData::MakeNewWStringAttributeData("RyftOne"));
    SetProperty(DSI_CONN_USER_NAME, AttributeData::MakeNewWStringAttributeData("User"));
    SetProperty(DSI_CONN_CURRENT_CATALOG, AttributeData::MakeNewWStringAttributeData(R1_CATALOG));

    SetProperty(DSI_CONN_CREATE_TABLE, AttributeData::MakeNewUInt32AttributeData(DSI_CT_CREATE_TABLE));
    SetProperty(DSI_CONN_DROP_TABLE, AttributeData::MakeNewUInt32AttributeData(DSI_DT_DROP_TABLE));
}
