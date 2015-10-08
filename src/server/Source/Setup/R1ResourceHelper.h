//==================================================================================================
///  @file R1ResourceHelper.h
///
///  Definition of the class R1ResourceHelper.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
//==================================================================================================

#ifndef _RYFTONE_R1RESOURCEHELPER_H_
#define _RYFTONE_R1RESOURCEHELPER_H_

#include "RyftOne.h"

#include <atlstr.h>

namespace RyftOne
{
    /// @brief Helper class to load resource strings from the String table.
    class R1ResourceHelper
    {
    // Public ======================================================================================
    public:
        /// @brief Loads the specified string from the resource's String table.
        ///
        /// @param in_resourceStringId      The integer identifier of the string to be loaded.
        /// 
        /// @return The string corresponding to the given identifier.
        static simba_wstring LoadStringResource(UINT in_resourceStringId);

        /// @brief Loads the specified string from the resource's String table.
        ///
        /// @param in_resourceStringId      The integer identifier of the string to be loaded.
        /// @param out_string               The string corresponding to the given identifier.
        static void LoadStringResource(UINT in_resourceStringId, CString& out_string);

        /// @brief Gets the handle of this DLL module.
        /// 
        /// @return The handle of this DLL module.
        static HINSTANCE GetModuleInstance();
    };
}

#endif
