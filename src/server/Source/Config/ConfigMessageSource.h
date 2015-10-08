// =================================================================================================
///  @file ConfigMessageSource.h
///
///  CodeBase DSIMessageSource implementation.
///
///  Copyright (C) 2008-2011 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _SIMBA_R1TRALIGHT_CONFIG_CONFIGMESSAGESOURCE_H_
#define _SIMBA_R1TRALIGHT_CONFIG_CONFIGMESSAGESOURCE_H_

#include "DSIMessageSource.h"
#include "AutoPtr.h"

namespace Simba
{
namespace DSI
{
    class DSIXmlMessageReader;
}
}

namespace Simba
{
namespace RyftOne
{
    /// @brief CodeBase Config implementation class.
    class ConfigMessageSource : public Simba::DSI::DSIMessageSource
    {
    // Public ======================================================================================
    public:

        /// @brief Constructor.
        ConfigMessageSource();

        /// @brief Destructor.
        virtual ~ConfigMessageSource();

    // Protected ===================================================================================
    protected:

        /// @brief Retrieves the specified message.
        /// 
        /// @param in_messageID             Unique message identifier.
        /// @param in_sourceComponentID     Source component identifier.
        /// @param out_message              The message for the given message and component ID.
        /// @param out_nativeErrCode        The error code for the given message and component ID.
        void LoadCustomMessage(
            const simba_wstring& in_messageID,
            simba_int32 in_sourceComponentID,
            simba_wstring& out_message,
            simba_int32& out_nativeErrCode);

        /// @brief Retrieves the specified message.
        /// 
        /// @param in_messageID             Unique message identifier.
        /// @param in_sourceComponentID     Source component identifier.
        /// @param in_messageParams         Parameters used to fill in the message.
        /// @param out_message              The message for the given message and component ID.
        /// @param out_nativeErrCode        The error code for the given message and component ID.
        void LoadCustomMessage(
            const simba_wstring& in_messageID,
            simba_int32 in_sourceComponentID,
            const std::vector<simba_wstring>& in_messageParams,
            simba_wstring& out_message,
            simba_int32& out_nativeErrCode);

    // Private =====================================================================================
    private:

        /// @brief Loads the native error code and message for the specified message and component 
        /// ID into the associated member variables.
        /// 
        /// @param in_messageID             Unique message identifier.
        /// @param in_sourceComponentID     Source component identifier.
        /// @param out_message              The message for the given message and component ID.
        /// @param out_nativeErrCode        The error code for the given message and component ID.
        void GetCustomMessage(
            const simba_wstring& in_messageID,
            simba_int32 in_sourceComponentID,
            simba_wstring& out_message,
            simba_int32& out_nativeErrCode);

        /// The XML message reader. (OWN)
        AutoPtr<Simba::DSI::DSIXmlMessageReader> m_reader;
        Simba::DSI::DSIMessageCache m_cache;
    };
}
}

#endif
