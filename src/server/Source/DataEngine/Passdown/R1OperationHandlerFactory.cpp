// =================================================================================================
///  @file R1OperationHandlerFactory.cpp
///
///  Implementation of the Class CBOperationHandlerFactory
///
///  Copyright (C) 2008-2011 Simba Technologies Incorporated.
// =================================================================================================

#include "R1OperationHandlerFactory.h"

#include "R1FilterHandler.h"
#include "R1Table.h"
#include "DSIExtResultSet.h"

using namespace RyftOne;
using namespace Simba::SQLEngine;

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1OperationHandlerFactory::R1OperationHandlerFactory() 
{
    ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AutoPtr<IBooleanExprHandler> R1OperationHandlerFactory::CreateFilterHandler(
    SharedPtr<DSIExtResultSet> in_table)
{
    SharedPtr<R1Table> table(static_cast<R1Table*> (in_table.Get()));
    return AutoPtr<IBooleanExprHandler>(new R1FilterHandler(table));
}

