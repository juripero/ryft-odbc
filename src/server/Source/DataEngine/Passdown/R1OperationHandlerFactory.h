// =================================================================================================
///  @file R1OperationHandlerFactory.h
///
///  Definition of the Class R1OperationHandlerFactory
///
///  Copyright (C) 2008-2011 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1OPERATIONALHANDLERFACTORY_H_
#define _RYFTONE_R1OPERATIONALHANDLERFACTORY_H_

#include "RyftOne.h"
#include "DSIExtOperationHandlerFactory.h"

namespace RyftOne
{
    /// @brief A "factory" class for handling passing down operations in Codebase.
    class R1OperationHandlerFactory : public Simba::SQLEngine::DSIExtOperationHandlerFactory
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        R1OperationHandlerFactory();

        /// @brief Destructor.
        virtual ~R1OperationHandlerFactory() {}

        /// @brief Create a filter handler to handle passing down filters onto the table.
        ///
        /// @param in_table         The table on which to apply filters. Cannot be NULL.
        ///
        /// @return a filter handler to handle passing down filters if the feature is supported, 
        ///         NULL otherwise. (OWN)
        virtual AutoPtr<Simba::SQLEngine::IBooleanExprHandler> CreateFilterHandler(
            SharedPtr<Simba::SQLEngine::DSIExtResultSet> in_table);

    // Private =====================================================================================
    private:
    };
}

#endif
