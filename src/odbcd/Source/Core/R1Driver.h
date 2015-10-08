// =================================================================================================
///  @file R1Driver.h
///
///  RyftOne DSIDriver implementation.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1DRIVER_H_
#define _RYFTONE_R1DRIVER_H_

#include "RyftOne.h"

#include "DSIDriver.h"
#include "DSIFileLogger.h"

namespace RyftOne
{
    /// @brief RyftOne DSIDriver implementation class.
    class R1Driver : public Simba::DSI::DSIDriver
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        R1Driver();

        /// @brief Destructor.
        virtual ~R1Driver();

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
