// =================================================================================================
///  @file R1Environment.h
///
///  RyftOne DSIEnvironment implementation.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1ENVIRONMENT_H_
#define _RYFTONE_R1ENVIRONMENT_H_

#include "RyftOne.h"
#include "DSIEnvironment.h"

namespace RyftOne
{
    /// @brief RyftOne DSIEnvironment implementation class.
    class R1Environment : public Simba::DSI::DSIEnvironment
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        ///
        /// @param in_driver        The IDriver associated with this environment instance. (NOT OWN)
        R1Environment(Simba::DSI::IDriver* in_driver);

        /// @brief Destructor.
        ~R1Environment();

        /// @brief Creates and returns a new IConnection instance.
        ///
        /// @return New IConnection instance. (OWN)
        Simba::DSI::IConnection* CreateConnection();
    };
}

#endif
