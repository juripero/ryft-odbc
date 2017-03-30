// =================================================================================================
///  @file R1UnloadProcedure.h
///
///  Definition of the Class R1UnloadProcedure
///
///  Copyright (C) 2010-2011 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _R1UnloadProcedure_H_
#define _R1UnloadProcedure_H_

#include "RyftOne.h"
#include "R1Catalog.h"
#include "DSIExtProcedure.h"
#include "AutoPtr.h"
#include "DSIResults.h"

namespace Simba
{
namespace DSI
{
    class IStatement;
}
namespace Support
{
    class ILogger;
}
}

namespace RyftOne
{
    static const simba_wstring R1_PROC_UNLOAD(L"R1Unload");

    /// @brief CodeBase sample stored procedure that has a return value, parameters,
    /// and a result set.
    class R1UnloadProcedure : public Simba::SQLEngine::DSIExtProcedure
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        ///
        /// @param in_codebaseSettings  The settings for the connection. (NOT OWN)
        /// @param in_statement         The parent statement. (NOT OWN)  
        R1UnloadProcedure(
            RyftOne_Database *in_ryft1,
            Simba::DSI::IStatement* in_statement);

        /// @brief Executes the stored procedure, using the given parameters.
        /// 
        /// The parameters used for executing the stored procedure will be provided in the form of
        /// an ETValueList containing ETParameters. The parameters will correspond to the parameter
        /// metadata provided by GetParameters(), and in the case of return, output, and 
        /// input/output parameters should have their output values set.
        ///
        /// After execution, results should be accessible via GetResults().
        ///
        /// @param in_parameters    The parameters to use when executing the stored procedure.
        ///                         (NOT OWN)
        virtual void Execute(Simba::SQLEngine::ParameterValues* in_parameters);

        /// @brief Retrieves the parameters for the stored procedure which can provide metadata
        /// for each parameter.
        ///
        /// May return NULL if there are no parameters associated with the stored procedure.
        ///
        /// This method may be called after query preparation and before execution.
        ///
        /// @return Reference to a vector of parameters that provides access to metadata. (NOT OWN)
        virtual Simba::SQLEngine::ParameterMetadataList* GetParameters();

    // Protected ===================================================================================
    protected:
        /// @brief Get a pointer reference to the results of the stored procedure.
        ///
        /// This function will be called to retrieve the metadata about any result sets that are
        /// returned. If no result set is returned but a row count is returned, then a result with
        /// that row count should be returned.
        ///
        /// If no results are generated, NULL should be returned.
        ///
        /// @return The results of the stored procedure. (NOT OWN).
        virtual Simba::DSI::IResults* DoGetResults();

    // Private =====================================================================================
    private:
        // The parameters for the stored procedure.
        Simba::SQLEngine::ParameterMetadataList m_parameters;

        // Log. (NOT OWN)
        Simba::Support::ILogger* m_log;

        // The parent statement. (NOT OWN)
        Simba::DSI::IStatement* m_statement;

        RyftOne_Database *m_ryft1;
        
        // The results that are returned from the procedure.
        AutoPtr<Simba::DSI::DSIResults> m_results;

        string __table;
        string __search_term;
    };
}
#endif
