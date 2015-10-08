// =================================================================================================
///  @file Driver.h
///
///  Ryft DSIDriver implementation.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
///  Copyright (C) 2015 Ryft Incorporated.
// =================================================================================================

#ifndef _RYFT_DRIVER_H_
#define _RYFT_DRIVER_H_

#include "Ryft.h"

#include "DSIDriver.h"
#include "DSIFileLogger.h"

namespace Ryft
{
    /// @brief Ryft DSIDriver implementation class.
    class Driver : public Simba::DSI::DSIDriver
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        Driver();

        /// @brief Destructor.
        virtual ~Driver();

        /// @brief Factory method for creating IEnvironments.
        ///
        /// @return The newly created IEnvironment. (OWN)
        virtual Simba::DSI::IEnvironment* CreateEnvironment();

        /// @brief Gets the driver-wide logging interface.
        ///
        /// @return The driver-wide logging interface. (NOT OWN)
        virtual Simba::Support::ILogger* GetDriverLog();

    // Private =====================================================================================
    private:
        /// @brief Overrides some of the default driver properties as defined in 
        /// DSIDriverProperties.h.
        void SetDriverPropertyValues();

        // Driver-wide ILogger.
        Simba::DSI::DSIFileLogger m_driverlog;
    };
}

#endif
