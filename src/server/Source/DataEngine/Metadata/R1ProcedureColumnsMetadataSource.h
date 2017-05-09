// =================================================================================================
///  @file R1ProcedureColumnsMetadataSource.h
///
///  Definition of the class R1ProcedureColumnsMetadataSource
///
///  Copyright (C) 2010-2011 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _R1PROCEDURECOLUMNSMETADATASOURCE_H_
#define _R1PROCEDURECOLUMNSMETADATASOURCE_H_

#include "RyftOne.h"
#include "R1ProcedureFactory.h"
#include "DSIMetadataSource.h"
#include "DSIExtProcedure.h"  // For ParameterMetadataList
#include <list>

namespace Simba
{
namespace DSI
{
    class IResult;
    class IColumn;
}
}

namespace RyftOne
{
    /// @brief CodeBase sample metadata table for procedure columns metadata.
    /// 
    /// SQLProcedureColumns returns rows in the following order, for each of the supported
    /// stored procedures.
    ///     - return value (if there is one)
    ///     - each parameter for the current stored procedure
    ///     - the columns in the result set(s) returned by the current stored procedure.
    /// http://msdn.microsoft.com/en-us/library/ms711701(VS.85).aspx
    class R1ProcedureColumnsMetadataSource : public Simba::DSI::DSIMetadataSource
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        /// 
        /// @param in_restrictions      Restrictions that may be applied to the metadata table.
        /// @param in_codeBaseSettings  The CodeBase settings. (NOT OWN)
        /// @param in_procedureFactory  The procedure factory to use. (NOT OWN)
        R1ProcedureColumnsMetadataSource(
            Simba::DSI::DSIMetadataRestrictions& in_restrictions,
            R1ProcedureFactory* in_procedureFactory);

        /// @brief Destructor.
        virtual ~R1ProcedureColumnsMetadataSource();

        /// @brief Closes the DSI's internal result cursor and clears associated memory.
        void CloseCursor();

        /// @brief Fills in in_data with data for the given column in the current row. 
        /// 
        /// The target column is identified by in_columnTag which can be found in 
        /// DSIMetadataColumnIdentifierDefns.h
        ///
        /// @param in_columnTag         Identifier that identifies a column.
        /// @param in_data              Holds a buffer to store the requested data.
        /// @param in_offset            Number of bytes in the data to skip before copying into in_data.
        /// @param in_maxSize           Maximum number of bytes of data to return in in_data.
        ///
        /// @return True if there is more data; false otherwise.
        bool GetMetadata(
            Simba::DSI::DSIOutputMetadataColumnTag in_columnTag,
            Simba::Support::SqlData* in_data, 
            simba_signed_native in_offset,
            simba_signed_native in_maxSize);

        /// @brief Traverses the result set, moving on to the next row.
        ///
        /// @param in_direction         Direction to traverse through a result set
        /// @param in_offset            Number of bytes in the data to skip
        /// 
        /// @return True if there are more rows to traverse; false if there are no more rows left.
        bool Move(Simba::DSI::DSIDirection in_direction, simba_signed_native in_offset);

    // Private =====================================================================================
    private:
        /// @brief Helper method to get the current column for the current stored procedure.
        /// 
        /// NOTE: Each column for each stored procedure represents a row in the result set for
        /// SQLProcedureColumns.
        /// 
        /// @return True if there are more columns to process; false if there are no more columns.
        bool GetCurrentColumn();

        /// @brief Helper method to get the current parameter column for the current stored
        /// procedure.
        /// 
        /// @return True if there are more parameters to process; false otherwise.
        bool GetCurrentParameterColumn();

        /// @brief Helper method to get the current column for the current result set of the current
        /// stored procedure.
        /// 
        /// @return True if there are more columns in the current result set; false otherwise.
        bool GetCurrentResultSetColumn();

        // The procedure factory to use for retrieving information about the supported stored
        // procedures. (NOT OWN)
        R1ProcedureFactory* m_procedureFactory;

        // Iterator to the current procedure.
        std::list<Simba::SQLEngine::DSIExtProcedure*>::const_iterator m_procItr;

        // Iterator to the current parameter column of the current procedure.
        Simba::SQLEngine::ParameterMetadataList::iterator m_paramItr;

        // The current result of the current procedure. (NOT OWN)
        Simba::DSI::IResult* m_currentResult;

        // The current column of the current result for the current procedure. (NOT OWN)
        Simba::DSI::IColumn* m_currentResultCol;

        // Counter for the columns of the current result for the current procedure.
        simba_size_t m_resultColumnCounter;

        // Indicates whether Move() has been called yet.
        bool m_hasStartedFetch;

        // Indicates whether the cursor is on a Parameter row, or a result row.
        bool m_isOnParameter;

        // Indicates whether the current procedure has a return value.
        bool m_hasReturnValue;

        // Indicates whether begin() has been called for m_paramItr.
        bool m_hasStartedParamItr;

        // Indicates whether Reset() has been called yet on the current result of the current
        // procedure.
        bool m_hasCalledResetOnResult;
    };
}
#endif
