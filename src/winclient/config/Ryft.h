// =================================================================================================
///  @file Ryft.h
///
///  Ryft constants
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
// =================================================================================================

#ifndef _RYFT_H_
#define _RYFT_H_

#include "Simba.h"
#include "ILogger.h"

#include "DiagState.h"
#include "ErrorException.h"

namespace Ryft
{
    /// Component identifier for Ryft errors.
    static const simba_int32 RYFT_ERROR = 101;

    // The error messages file to use.
    #define ERROR_MESSAGES_FILE "RyftMessages"

    // The Windows branding for the driver.
    #define DRIVER_WINDOWS_BRANDING "Ryft\\ODBC Driver"

    // The Linux branding for the driver.
    #define DRIVER_LINUX_BRANDING "ryft.odbcdriver.ini"

    // The Linux branding for the server.
    #define SERVER_LINUX_BRANDING "ryftserver.odbcdriver.ini"

    // The driver vendor name.
    #define DRIVER_VENDOR "Ryft"

    /// The connection key to use when looking up the DSN in the connection string.
    static const simba_wstring RYFT_DSN_KEY(L"DSN");

    /// The connection key to use when looking up the UID in the connection string.
    static const simba_wstring RYFT_UID_KEY(L"UID");

    /// The connection key to use when looking up the PWD in the connection string.
    static const simba_wstring RYFT_PWD_KEY(L"PWD");

    /// The connection key to use when looking up the URL in the connection string.
    static const simba_wstring RYFT_URL_KEY(L"URL");

    /// The connection key to use when looking up the PORT in the connection string.
    static const simba_wstring RYFT_PORT_KEY(L"PORT");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// MACRO Definitions
///
/// Macros to facilitate exception throwing in the Ryft DSII.
////////////////////////////////////////////////////////////////////////////////////////////////////

// Throw an ErrorException with state key DIAG_GENERAL_ERROR, component id RYFT_ERROR, and the given
// message id.  Row number and column number may also be specified.
#define RYFTTHROWGEN(...)                                                     \
{                                                                               \
    throw Simba::Support::ErrorException(Simba::Support::DIAG_GENERAL_ERROR, RYFT_ERROR, __VA_ARGS__);\
}

// Throw an ErrorException with state key DIAG_GENERAL_ERROR, component id RYFT_ERROR, and the given
// message id.
#define RYFTTHROWGEN1(id, param)                                              \
{                                                                               \
    std::vector<simba_wstring> msgParams;                                       \
    msgParams.push_back(param);                                                 \
    throw Simba::Support::ErrorException(Simba::Support::DIAG_GENERAL_ERROR, RYFT_ERROR, id, msgParams);\
}

// Throw an ErrorException with the given state key and component id RYFT_ERROR, with the
// given message id.
#define RYFTTHROW(key, id)                                                    \
{                                                                               \
    throw Simba::Support::ErrorException(key, RYFT_ERROR, id);                \
}

// Throw an ErrorException with the given state key, component id RYFT_ERROR, the given message
// id and the given message parameter.
#define RYFTTHROW1(key, id, param)                                            \
{                                                                               \
    std::vector<simba_wstring> msgParams;                                       \
    msgParams.push_back(param);                                                 \
    throw Simba::Support::ErrorException(key, RYFT_ERROR, id, msgParams);     \
}

// Throw an ErrorException with the given state key, component id RYFT_ERROR, the given message
// id and the given message parameter.
#define DERBYTHROW(state, params)                                                   \
{                                                                                           \
    std::vector<simba_wstring> __msgParams;                                                 \
    std::vector<string>::iterator __itr;                                                    \
    for(__itr=params.begin(); __itr != params.end(); __itr++)                               \
        __msgParams.push_back(__itr->c_str());                                              \
    throw Simba::Support::ErrorException(SQLState(state), RYFT_ERROR, state, __msgParams);\
}

#endif
