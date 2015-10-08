// =================================================================================================
///  @file R1Statement.cpp
///
///  RyftOne DSIStatement implementation.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#include "R1Statement.h"

#include "IConnection.h"
#include "R1DataEngine.h"

using namespace RyftOne;
using namespace Simba::DSI;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1Statement::R1Statement(IConnection* in_connection, RyftOne_Database *ryft1) : DSIStatement(in_connection), m_ryft1(ryft1)
{
    ENTRANCE_LOG(GetLog(), "Simba::RyftOne", "R1Statement", "R1Statement");

    // Default statement properties are set by the DSIStatement parent class, however they can be
    // overridden here. See DSIStmtProperties.h for more information on default properties. 
    // Additionally, custom statement properties can be added, see GetCustomProperty(), 
    // GetCustomPropertyType(), IsCustomProperty(), and SetCustomProperty() on DSIStatement.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1Statement::~R1Statement()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDataEngine* R1Statement::CreateDataEngine()
{
    ENTRANCE_LOG(GetLog(), "Simba::RyftOne", "R1Statement", "CreateDataEngine");
    return new R1DataEngine(this, m_ryft1);
}
