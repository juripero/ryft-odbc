// =================================================================================================
///  @file ConfigMessageSource.cpp
///
///  Ultralight configuration DSIMessageSource implementation.
///
///  Copyright (C) 2008-2011 Simba Technologies Incorporated.
// =================================================================================================

#include "ConfigMessageSource.h"
#include "DSIXmlMessageReader.h"

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigMessageSource::ConfigMessageSource() :
    DSIMessageSource(),
    m_reader(new DSIXmlMessageReader("ConfigErrorMessages.xml", DEFAR1T_LOCALE_STR))
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigMessageSource::~ConfigMessageSource()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigMessageSource::LoadCustomMessage(
    const simba_wstring& in_messageID,
    simba_int32 in_sourceComponentID,
    simba_wstring& out_message,
    simba_int32& out_nativeErrCode)
{
    GetCustomMessage(in_messageID, in_sourceComponentID, out_message, out_nativeErrCode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigMessageSource::LoadCustomMessage(
    const simba_wstring& in_messageID,
    simba_int32 in_sourceComponentID,
    const std::vector<simba_wstring>& in_messageParams,
    simba_wstring& out_message,
    simba_int32& out_nativeErrCode)
{
    GetCustomMessage(in_messageID, in_sourceComponentID, out_message, out_nativeErrCode);

    if (DSI_INVALID_ERRORCODE != out_nativeErrCode)
    {
        out_message = GetParameterizedMessage(out_message, in_messageParams);
    }
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigMessageSource::GetCustomMessage(
    const simba_wstring& in_messageID,
    simba_int32 in_sourceComponentID,
    simba_wstring& out_message,
    simba_int32& out_nativeErrCode)
{
    if (m_cache.GetErrorMessage(DEFAR1T_LOCALE_STR, in_messageID, in_sourceComponentID, out_message, out_nativeErrCode))
    {
        // The message was stored in the cache, just return.
        return;
    }

    // CONFIG_ERROR should be the only package in CodeBase configuration.
    if (1000 != in_sourceComponentID)
    {
        assert(false);
    }

    m_reader->GetErrorMessage(DEFAR1T_LOCALE_STR, in_messageID, in_sourceComponentID, out_message, out_nativeErrCode);

    if (DSI_INVALID_ERRORCODE != out_nativeErrCode)
    {
        // If the message was found, add it to the cache.
        m_cache.AddErrorMessage(DEFAR1T_LOCALE_STR, in_messageID, in_sourceComponentID, out_message, out_nativeErrCode);
    }
}
