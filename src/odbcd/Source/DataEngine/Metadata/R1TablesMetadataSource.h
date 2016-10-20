// =================================================================================================
///  @file R1TablesMetadataSource.h
///
///  Definition of the class R1TablesMetadataSource
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1TABLESMETADATASOURCE_H_
#define _RYFTONE_R1TABLESMETADATASOURCE_H_

#include "RyftOne.h"
#include "R1Catalog.h"
#include "DSIMetadataSource.h"

namespace RyftOne
{
    /// @brief RyftOne sample metadata table for table metadata.
    ///
    /// This source contains the following output columns as defined by SimbaEngine:
    ///     DSI_CATALOG_NAME_COLUMN_TAG 
    ///     DSI_SCHEMA_NAME_COLUMN_TAG
    ///     DSI_TABLE_NAME_COLUMN_TAG
    ///     DSI_TABLE_TYPE_COLUMN_TAG
    ///     DSI_REMARKS_COLUMN_TAG
    ///
    /// The implementation of Move() must respond to the above mentioned columns.
    class R1TablesMetadataSource : public Simba::DSI::DSIMetadataSource
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        /// 
        /// @param in_restrictions      Restrictions that may be applied to the metadata table
        R1TablesMetadataSource(Simba::DSI::DSIMetadataRestrictions& in_restrictions, RyftOne_Database *in_ryft1);

        /// @brief Destructor.
        virtual ~R1TablesMetadataSource();

        /// @brief Repositions the cursor to before the start of the table.
        void CloseCursor();

        /// @brief Fills in in_data with data for the given column in the current row. 
        /// 
        /// The target column is identified by in_columnTag which can be found in 
        /// DSIMetadataColumnIdentifierDefns.h
        ///
        /// @param in_columnTag         Identifier that identifies a column.
        /// @param in_data              Holds a buffer to store the requested data. (NOT OWN)
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
        // Indicates whether Move() has been called yet.
        bool m_hasStartedFetch;

        RyftOne_Tables m_tables;
        RyftOne_Tables::iterator m_tablesItr;
    };
}

#endif

