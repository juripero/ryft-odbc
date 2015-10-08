// =================================================================================================
///  @file R1Environment.cpp
///
///  RyftOne DSIEnvironment implementation.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1Environment.h"

#include "IDriver.h"
#include "R1Connection.h"

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1Environment::R1Environment(IDriver* in_driver) : DSIEnvironment(in_driver)
{
    ENTRANCE_LOG(GetLog(), "Simba::RyftOne", "R1Environment", "R1Environment");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1Environment::~R1Environment()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IConnection* R1Environment::CreateConnection()
{
    ENTRANCE_LOG(GetLog(), "Simba::RyftOne", "R1Environment", "CreateConnection");
    return new R1Connection(this);
}
