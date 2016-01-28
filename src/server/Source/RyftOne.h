// =================================================================================================
///  @file RyftOne.h
///
///  RyftOne constants
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _SIMBA_R1TRALIGHT_R1TRALIGHT_H_
#define _SIMBA_R1TRALIGHT_R1TRALIGHT_H_

#include "Simba.h"
#include "ILogger.h"

#include "DiagState.h"
#include "ErrorException.h"

namespace RyftOne
{
    /// Component identifier for RyftOne errors.
    static const simba_int32 R1_ERROR = 101;

    // The error messages file to use.
    #define ERROR_MESSAGES_FILE "R1Messages"

    // The Windows branding for the driver.
    #define DRIVER_WINDOWS_BRANDING "RyftOne"

    // The Linux branding for the driver.
    #define DRIVER_LINUX_BRANDING ".ryftone.ini"

    // The Linux branding for the server.
    #define SERVER_LINUX_BRANDING ".ryftone.server.ini"

    // The driver vendor name.
    #define DRIVER_VENDOR "Ryft"

    /// The connection key to use when looking up the DSN in the connection string.
    static const simba_wstring R1_DSN_KEY(L"DSN");

    /// The connection key to use when looking up the UID in the connection string.
    static const simba_wstring R1_UID_KEY(L"UID");

    /// The connection key to use when looking up the PWD in the connection string.
    static const simba_wstring R1_PWD_KEY(L"PWD");

    /// The connection key to use when looking up the LNG in the connection string.
    static const simba_wstring R1_LNG_KEY(L"LNG");

    /// The faked catalog for the hardcoded data.
    static const simba_wstring R1_CATALOG(L"RyftOne");

    /// The faked schema for the hardcoded data.
    static const simba_wstring R1_SCHEMA(L"RyftOne");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// MACRO Definitions
///
/// Macros to facilitate exception throwing in the RyftOne DSII.
////////////////////////////////////////////////////////////////////////////////////////////////////

// Throw an ErrorException with state key DIAG_GENERAL_ERROR, component id R1_ERROR, and the given
// message id.  Row number and column number may also be specified.
#define R1THROWGEN(...)                                                         \
{                                                                               \
    throw Simba::Support::ErrorException(Simba::Support::DIAG_GENERAL_ERROR, R1_ERROR, __VA_ARGS__);\
}

// Throw an ErrorException with state key DIAG_GENERAL_ERROR, component id R1_ERROR, and the given
// message id.
#define R1THROWGEN1(id, param)                                                  \
{                                                                               \
    std::vector<simba_wstring> msgParams;                                       \
    msgParams.push_back(param);                                                 \
    throw Simba::Support::ErrorException(Simba::Support::DIAG_GENERAL_ERROR, R1_ERROR, id, msgParams);\
}

// Throw an ErrorException with the given state key and component id R1_ERROR, with the
// given message id.
#define R1THROW(key, id)                                                        \
{                                                                               \
    throw Simba::Support::ErrorException(key, R1_ERROR, id);                    \
}

// Throw an ErrorException with the given state key, component id R1_ERROR, the given message
// id and the given message parameter.
#define R1THROW1(key, id, param)                                                \
{                                                                               \
    std::vector<simba_wstring> msgParams;                                       \
    msgParams.push_back(param);                                                 \
    throw Simba::Support::ErrorException(key, R1_ERROR, id, msgParams);         \
}

// Throw an ErrorException with the given state key, component id R1_ERROR, the given message
// id and the two given message parameters.
#define R1THROW2(key, id, param1, param2)                                       \
{                                                                               \
    std::vector<simba_wstring> msgParams;                                       \
    msgParams.push_back(param1);                                                \
    msgParams.push_back(param2);                                                \
    throw Simba::Support::ErrorException(key, R1_ERROR, id, msgParams);         \
}

#endif
