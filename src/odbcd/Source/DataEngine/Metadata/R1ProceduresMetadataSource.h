// =================================================================================================
///  @file R1ProceduresMetadataSource.h
///
///  Definition of the class R1ProceduresMetadataSource
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1PROCEDURESMETADATASOURCE_H_
#define _RYFTONE_R1PROCEDURESMETADATASOURCE_H_

#include "RyftOne.h"
#include "R1Catalog.h"
#include "DSIMetadataSource.h"
#include "R1ProcedureFactory.h"
#include <list>

namespace Simba
{
namespace SQLEngine
{
    class DSIExtProcedure;
}
}

namespace RyftOne
{
    /// @brief CodeBase sample metadata table for procedures metadata.
    class R1ProceduresMetadataSource : public Simba::DSI::DSIMetadataSource
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        /// 
        /// @param in_restrictions      Restrictions that may be applied to the metadata table.
        /// @param in_codeBaseSettings  The CodeBase settings (NOT OWN).
        /// @param in_procedureFactory  The procedure factory to use (NOT OWN).
        R1ProceduresMetadataSource(
            Simba::DSI::DSIMetadataRestrictions& in_restrictions,
            R1ProcedureFactory* in_procedureFactory);

        /// @brief Destructor.
        virtual ~R1ProceduresMetadataSource();

        /// @brief Closes the DSI's internal result cursor and clears associated memory.
        void CloseCursor();

        /// @brief Fills in in_data with data for the given column in the current row. 
        /// 
        /// The target column is identified by in_columnTag which can be found in 
        /// DSIMetadataColumnIdentifierDefns.h
        ///
        /// @param  in_columnTag        Identifier that identifies a column.
        /// @param  in_data             Holds a buffer to store the requested data.
        /// @param  in_offset           Number of bytes in the data to skip before copying into in_data.
        /// @param  in_maxSize          Maximum number of bytes of data to return in in_data.
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
        /// @brief Get the procedure type for the current procedure.
        /// 
        /// @return The procedure type for the current procedure: Either SQL_PT_PROCEDURE or
        /// SQL_PT_FUNCTION.
        simba_int16 GetProcedureType();

        // Indicates whether Move() has been called yet.
        bool m_hasStartedFetch;

        // Iterator to the current procedure.
        std::list<Simba::SQLEngine::DSIExtProcedure*>::const_iterator m_procItr;

        // The procedure factory to use for retrieving information about the supported stored
        // procedures (NOT OWN).
        R1ProcedureFactory* m_procedureFactory;
    };
}
#endif