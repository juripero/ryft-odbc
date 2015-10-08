//==================================================================================================
///  @file DialogConsts.h
///
///  Definition of the class DialogConsts.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
//==================================================================================================

#ifndef _RYFT_DIALOGCONSTS_H_
#define _RYFT_DIALOGCONSTS_H_

#include "Simba.h"

namespace Ryft
{
    /// Identifier used to store and retrieve the configuration settings.
    #define CONF_PROP L"InputMap"
    
    /// @brief Get the configuration properties from the dialog.
    ///
    /// @param in_handle    The dialog handle.
    /// @param in_type      The type of the stored property.
    #define GET_CONFIG(in_handle, in_type)                                                         \
        reinterpret_cast<in_type*>(GetProp((in_handle), CONF_PROP));

    /// @brief The connection key to use when looking up the Description.
    #define DESC_KEY L"DESCRIPTION"

    /// @brief DialogConsts defines shared constants.
    class DialogConsts
    {
    // Public ======================================================================================
    public:
        /// @brief Max length of DSN name.
        static const simba_int32 MaxDsnLength = SQL_MAX_DSN_LENGTH;

        /// @brief Max length of Description attribute.
        static const simba_int32 MaxDescLength = MAX_PATH;
    };
}

#endif
