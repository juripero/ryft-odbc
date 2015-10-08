//==================================================================================================
///  @file R1SetupException.cpp
///
///  Implementation of the class R1SetupException.cpp
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
//==================================================================================================

#include "R1SetupException.h"

#include "R1ResourceHelper.h"
#include "resource.h"

#include <odbcinst.h>

using namespace RyftOne;

// Static ==========================================================================================
static const UINT RESOURCE_CODES[R1SetupException::CFG_DLG_ERROR_LAST] = 
{ 
    IDS_ERROR_INVALID_HWND, 
    IDS_ERROR_INVALID_KEYWORD_VALUE, 
    IDS_ERROR_INVALID_NAME, 
    IDS_ERROR_INVALID_REQUEST_TYPE, 
    IDS_ERROR_REQUEST_FAILED, 
    IDS_ERROR_DRIVER_SPECIFIC,
    IDS_ERROR_REGISTRY_WRITE,
    IDS_ERROR_INVALID_MAX_LOG_NUM,
    IDS_ERROR_INVALID_MAX_LOG_SIZE
};

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
simba_wstring R1SetupException::GetErrorMsg() const
{
    return R1ResourceHelper::LoadStringResource(RESOURCE_CODES[m_errorCode]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD R1SetupException::GetErrorCode() const
{
    switch(m_errorCode)
    {
        case CFG_DLG_ERROR_INVALID_HWND:
        {
            return ODBC_ERROR_INVALID_HWND;
        }

        case CFG_DLG_ERROR_INVALID_KEYWORD_VALUE:
        {
            return ODBC_ERROR_INVALID_KEYWORD_VALUE;
        }

        case CFG_DLG_ERROR_INVALID_NAME:
        {
            return ODBC_ERROR_INVALID_NAME;
        }

        case CFG_DLG_ERROR_INVALID_REQUEST_TYPE:
        {
            return ODBC_ERROR_INVALID_REQUEST_TYPE;
        }

        case CFG_DLG_ERROR_REQUEST_FAILED:
        {
            return ODBC_ERROR_REQUEST_FAILED;
        }

        case CFG_DLG_ERROR_DRIVER_SPECIFIC:
        {
            return ODBC_ERROR_REQUEST_FAILED;
        }

        default:
        {
            assert(false);
            return ODBC_ERROR_REQUEST_FAILED;
        }
    }
}
