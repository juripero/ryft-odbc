//==================================================================================================
///  @file R1DialogConsts.h
///
///  Definition of the class R1DialogConsts.
///
///  Copyright (C) 2014 Simba Technologies Incorporated.
//==================================================================================================

#ifndef _RYFTONE_R1DIALOGCONSTS_H_
#define _RYFTONE_R1DIALOGCONSTS_H_

#include "Simba.h"

namespace RyftOne
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

    /// @brief R1DialogConsts defines shared constants.
    class R1DialogConsts
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
