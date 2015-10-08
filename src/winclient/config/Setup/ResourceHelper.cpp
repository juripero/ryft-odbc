//==================================================================================================
///  @file ResourceHelper.cpp
///
///  Implementation of the class ResourceHelper.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
//==================================================================================================

#include "ResourceHelper.h"
#include <gdiplus.h>
using namespace Gdiplus;

#include "NumberConverter.h"

using namespace Ryft;

extern simba_handle s_moduleId;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
simba_wstring ResourceHelper::LoadStringResource(UINT in_resourceStringId)
{
    static const simba_wstring loadError(L"Error loading the string.");

    // Try a buffer size of 2048, then increase the size until the entire string is retrieved.
    const std::vector<TCHAR>::size_type increment(2048);
    std::vector<TCHAR> buffer(increment);

    for (;;)
    {
        simba_int32 length = ::LoadString(
            GetModuleInstance(),
            in_resourceStringId,
            &buffer[0],
            static_cast<simba_int32>(buffer.size()));
        if (0 == length)
        {
            return loadError;
        }

        if (1 >= (buffer.size() - length))
        {
            buffer.resize(buffer.size() + increment);
        }
        else
        {
            return simba_wstring(&buffer[0], length);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ResourceHelper::LoadStringResource(UINT in_resourceStringId, CString& out_string)
{
    if (!out_string.LoadString(in_resourceStringId))
    {
        // The resource could not be found.
        RYFTTHROWGEN1(
            L"StringResourceError", 
            NumberConverter::ConvertUInt32ToWString(in_resourceStringId));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
HINSTANCE ResourceHelper::GetModuleInstance()
{
    return reinterpret_cast<HINSTANCE>(s_moduleId);
}

void ResourceHelper::GetFileVersion(DWORD *pdwMS, DWORD *pdwLS)
{
    DWORD   verHandle = NULL;
    UINT    size      = 0;
    LPBYTE  lpBuffer  = NULL;
    WCHAR   szFilename[260];
    DWORD   verSize;  
    LPSTR   verData;

    *pdwMS = 0;
    *pdwLS = 0;

    GetModuleFileNameW(ResourceHelper::GetModuleInstance(), szFilename, 260);
    verSize = GetFileVersionInfoSize(szFilename, &verHandle);
    if (verSize != 0) {
        verData = new char[verSize];
        if (GetFileVersionInfo(szFilename, verHandle, verSize, verData)) {
            if (VerQueryValue(verData, L"\\", (VOID FAR* FAR*)&lpBuffer, &size)) {
                if (size) {
                    VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
                    if (verInfo->dwSignature == 0xfeef04bd) {
                        *pdwMS = verInfo->dwFileVersionMS;
                        *pdwLS = verInfo->dwFileVersionLS;
                    }
                }
            }
        }
        delete[] verData;
    }
}

HBITMAP ResourceHelper::LoadResourceHBITMAP(LPCTSTR pName)
{
    HGLOBAL hBuffer;
    void *  pBuffer;
    Bitmap *bitmap;
    HBITMAP hbitmap = NULL;
    COLORREF cr = GetSysColor(COLOR_3DFACE);

    HRSRC hResource = FindResource(GetModuleInstance(), pName, RT_RCDATA);
    if (!hResource)
        return NULL;
    
    DWORD imageSize = ::SizeofResource(GetModuleInstance(), hResource);
    if (!imageSize)
        return NULL;

    const void* pResourceData = ::LockResource(LoadResource(GetModuleInstance(), hResource));
    if (!pResourceData)
        return NULL;

    ULONG_PTR m_gdiplusToken;
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

    hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
    if (hBuffer)  {
        pBuffer = ::GlobalLock(hBuffer);
        if (pBuffer) {
            CopyMemory(pBuffer, pResourceData, imageSize);

            IStream* pStream = NULL;
            if (CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK) {
                bitmap = Bitmap::FromStream(pStream);
                pStream->Release();
                if (bitmap) { 
                    bitmap->GetHBITMAP(Color(GetRValue(cr), GetGValue(cr), GetBValue(cr)), &hbitmap);
                    delete bitmap;
                }
            }
            GlobalUnlock(hBuffer);
        }
        GlobalFree(hBuffer);
    }

    Gdiplus::GdiplusShutdown(m_gdiplusToken);
    return hbitmap;
}
