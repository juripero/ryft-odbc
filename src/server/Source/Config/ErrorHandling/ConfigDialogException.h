// ================================================================================================
///  @file ConfigDialogException.h
///
///  Definition of the class ConfigDialogException.
///
///  Copyright (C) 2009-2011 Simba Technologies Incorporated.
// ================================================================================================

#ifndef _SIMBA_R1TRALIGHT_CONFIGDIALOGEXCEPTION_H_
#define _SIMBA_R1TRALIGHT_CONFIGDIALOGEXCEPTION_H_

#include "Simba.h"
#include "ErrorException.h"


namespace Simba
{
namespace RyftOne
{
    /// @brief Exception for configuration dialog errors.
    /// 
    /// This exception class uses error code CONFIG_ERROR, row number NO_ROW_NUMBER, and column
    /// number NO_COLUMN_NUMBER.
	class ConfigDialogException : public Simba::Support::ErrorException
    {
    // Public =====================================================================================
    public:

        /// @brief Constructor.
        /// 
        /// @param in_msgKey        Key into IMessageSource for the error message.
        /// @param in_diagState     The specific diagnostic state corresponding to the occurred 
        ///                         error.
        ConfigDialogException(
			const simba_wstring& in_msgKey,
            const Simba::Support::DiagState in_diagState);

        /// @brief Constructor.
        /// 
        /// @param in_msgKey        Key into IMessageSource for the error message.
        ConfigDialogException(
			const simba_wstring& in_msgKey);

        /// @brief Constructor.
        /// 
        /// @param in_msgKey        Key into IMessageSource for the error message.
        /// @param in_msgParams     Parameters to be used to construct the error message.
        ConfigDialogException(
			const simba_wstring& in_msgKey, 
			const std::vector<simba_wstring>& in_msgParams);

        /// @brief Constructor.
        /// 
        /// @param in_msgKey        Key into IMessageSource for the error message.
        /// @param in_diagState     The specific diagnostic state corresponding to the occurred
        ///                         error.
        /// @param in_paramMsgs     Parameters to be used to construct the error message.
        ConfigDialogException(
			const simba_wstring& in_msgKey,
            const Simba::Support::DiagState in_diagState,
			const std::vector<simba_wstring>& in_msgParams);
    };
}
}


#endif // ifndef _SIMBA_CODEBASE_CONFIGDIALOGEXCEPTION_H_
