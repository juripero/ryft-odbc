// =================================================================================================
///  @file R1ProcedureFactory.h
///
///  Definition of the Class R1ProcedureFactory
///
///  Copyright (C) 2010-2011 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _R1PROCEDUREFACTORY_H_
#define _R1PROCEDUREFACTORY_H_

#include "RyftOne.h"
#include "R1Catalog.h"
#include "SharedPtr.h"
#include "DSIExtProcedure.h"
#include <list>

namespace Simba
{
namespace DSI
{
    class IStatement;
}
}

namespace RyftOne
{
    /// @brief CodeBase stored procedure factory.
    class R1ProcedureFactory
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        /// 
        /// @param in_codebaseSettings  The settings for the connection (NOT OWN).
        /// @param in_statement         The parent statement. (NOT OWN)    
        R1ProcedureFactory(
            RyftOne_Database *in_ryft1,
            Simba::DSI::IStatement* in_statement);

        /// @brief Destructor.
        ~R1ProcedureFactory();

        /// @brief Create and open a stored procedure.
        /// 
        /// This method will be called during the preparation of a SQL statement.
        ///
        /// Once the stored procedure is opened, it should allow retrieval of metadata. That is, 
        /// calling GetResults() on the returned procedure should return results that provide column 
        /// metadata, if any, and calling GetParameters() on the returned procedure should return 
        /// parameter metadata, if any.
        /// 
        /// If a result set is returned, before data can be retrieved from the table SetCursorType() 
        /// will have to called. Since this is done at the execution time, the DSII should _NOT_ try 
        /// to make the data ready for retrieval until Execute() is called.
        ///
        /// The DSII decides how catalog and schema are interpreted or supported.
        /// 
        /// @param in_catalogName   The name of the catalog in which the stored procedure resides.
        /// @param in_schemaName    The name of the schema in which the stored procedure resides.
        /// @param in_procName      The name of the stored procedure to create..
        ///
        /// @return the created procedure, NULL if the table does not exist.
        SharedPtr<Simba::SQLEngine::DSIExtProcedure> CreateProcedure(
            const simba_wstring& in_procName);

        /// @brief Get a list of supported stored procedures.
        /// 
        /// @return The list of supported stored procedures (NOT OWN).
        const std::list<Simba::SQLEngine::DSIExtProcedure*>& GetProcedures();

    // Private =====================================================================================
    private:
        /// @brief Helper to initialize the list of supported stored procedures.
        void InitializeProcedures();

        // The list of supported stored procedures.
        std::list<Simba::SQLEngine::DSIExtProcedure*> m_procedures;

        // The parent statement (NOT OWN)
        Simba::DSI::IStatement* m_statement;

        RyftOne_Database *m_ryft1;
    };
}
#endif
