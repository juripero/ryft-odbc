// =================================================================================================
///  @file ConfigDialogException.cpp
///
///  Implementation of the class ConfigDialogException
///
///  Copyright (C) 2009-2011 Simba Technologies Incorporated.
// =================================================================================================

#include "ConfigDialogException.h"

using namespace RyftOne;
using namespace Simba::Support;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigDialogException::ConfigDialogException(
    const simba_wstring& in_msgKey,
    const DiagState in_diagState) :
        ErrorException(
            in_diagState,
            1000,
            in_msgKey,
            NO_ROW_NUMBER,
            NO_COLUMN_NUMBER)
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigDialogException::ConfigDialogException(
    const simba_wstring& in_msgKey) :
        ErrorException(
            DIAG_GENERAL_ERROR,
            1000,
            in_msgKey,
            NO_ROW_NUMBER,
            NO_COLUMN_NUMBER)
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigDialogException::ConfigDialogException(
    const simba_wstring& in_msgKey, 
    const std::vector<simba_wstring>& in_msgParams):
        ErrorException(
            DIAG_GENERAL_ERROR,
            1000,
            in_msgKey,
            in_msgParams,
            NO_ROW_NUMBER,
            NO_COLUMN_NUMBER)
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigDialogException::ConfigDialogException(
    const simba_wstring& in_msgKey, 
    const DiagState in_diagState,
    const std::vector<simba_wstring>& in_msgParams) :
        ErrorException(
            in_diagState,
            1000,
            in_msgKey,
            in_msgParams,
            NO_ROW_NUMBER,
            NO_COLUMN_NUMBER)
{
    ; // Do nothing.
}
