// =================================================================================================
///  @file R1TypeInfoMetadataSource.h
///
///  Definition of the class R1TypeInfoMetadataSource
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1TYPEINFOMETADATASOURCE_H_
#define _RYFTONE_R1TYPEINFOMETADATASOURCE_H_

#include "RyftOne.h"
#include "DSIMetadataSource.h"

#include <vector>

namespace RyftOne
{
    /// @brief RyftOne sample metadata table for types supported by the DSI implementation.
    ///
    /// This source contains the following output columns as defined by SimbaEngine:
    ///     DSI_DATA_TYPE_NAME_COLUMN_TAG
    ///     DSI_DATA_TYPE_COLUMN_TAG
    ///     DSI_COLUMN_SIZE_COLUMN_TAG
    ///     DSI_LITERAL_PREFIX_COLUMN_TAG
    ///     DSI_LITERAL_SUFFIX_COLUMN_TAG
    ///     DSI_CREATE_PARAM_COLUMN_TAG
    ///     DSI_NULLABLE_COLUMN_TAG
    ///     DSI_CASE_SENSITIVE_COLUMN_TAG
    ///     DSI_SEARCHABLE_COLUMN_TAG
    ///     DSI_UNSIGNED_ATTRIBUTE_COLUMN_TAG
    ///     DSI_FIXED_PREC_SCALE_COLUMN_TAG
    ///     DSI_AUTO_UNIQUE_COLUMN_TAG
    ///     DSI_LOCAL_TYPE_NAME_COLUMN_TAG
    ///     DSI_MINIMUM_SCALE_COLUMN_TAG
    ///     DSI_MAXIMUM_SCALE_COLUMN_TAG
    ///     DSI_SQL_DATA_TYPE_COLUMN_TAG
    ///     DSI_SQL_DATETIME_SUB_COLUMN_TAG
    ///     DSI_NUM_PREC_RADIX_COLUMN_TAG
    ///     DSI_INTERVAL_PRECISION_COLUMN_TAG
    ///     DSI_USER_DATA_TYPE_COLUMN_TAG
    ///
    /// The implementation of Move() must respond to the above mentioned columns.
    class R1TypeInfoMetadataSource : public Simba::DSI::DSIMetadataSource
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        /// 
        /// @param in_restrictions      Restrictions that may be applied to the metadata table.
        /// @param in_isODBCV3          Whether ODBC 3.X is in use.
        R1TypeInfoMetadataSource(
            Simba::DSI::DSIMetadataRestrictions& in_restrictions,
            bool in_isODBCV3);

        /// @brief Destructor.
        virtual ~R1TypeInfoMetadataSource();

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
        /// @brief Initializes the data storage for the table.
        ///
        /// This function is required because RyftOne does not retrieve data from an actual data
        /// source. This function hard-codes data that is to be returned when requested.
        ///
        /// @param in_isODBCV3          Whether ODBC 3.X is in use.
        void InitializeData(bool in_isODBCV3);

        /// @brief struct describing a single SQL type.
        typedef struct _SQLTypeInfo
        {
            simba_int16 m_dataType;
            simba_int32 m_columnSize;
            simba_wstring m_literalPrefix;
            simba_wstring m_literalSuffix;
            simba_wstring m_createParams;
            simba_int16 m_nullable;
            simba_int16 m_caseSensitive;
            simba_int16 m_searchable;
            simba_int16 m_unsignedAttr;
            simba_int16 m_fixedPrecScale;
            simba_int16 m_autoUnique;
            simba_int16 m_minScale;
            simba_int16 m_maxScale;
            simba_int16 m_intervalPrecision;

            /// @brief Reset the type information to a default state.
            void Reset()
            {
                m_dataType = SQL_CHAR;
                m_columnSize = 1;
                m_literalPrefix.Clear();
                m_literalSuffix .Clear();
                m_createParams.Clear();
                m_nullable = SQL_NULLABLE;
                m_caseSensitive = SQL_FALSE;
                m_searchable = SQL_SEARCHABLE;
                m_unsignedAttr = SQL_FALSE;
                m_fixedPrecScale = SQL_FALSE;
                m_autoUnique = SQL_NULL_DATA;
                m_minScale = 0;
                m_maxScale = 0;
                m_intervalPrecision = SQL_NULL_DATA;
            }
        } SQLTypeInfo;

        // The collection of data type information.
        std::vector<SQLTypeInfo> m_dataTypes;

        // The current row information.
        std::vector<SQLTypeInfo>::iterator m_rowItr;

        // Indicates whether Move() has been called yet.
        bool m_hasStartedFetch;
    };
}

#endif
