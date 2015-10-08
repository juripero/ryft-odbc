// =================================================================================================
///  @file R1PersonTable.h
///
///  Definition of the class R1PersonTable
///
///  Copyright (C) 2005-2011 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _SIMBA_R1TRALIGHT_R1PERSONTABLE_H_
#define _SIMBA_R1TRALIGHT_R1PERSONTABLE_H_

#include "RyftOne.h"
#include "TypedDataWrapperDefns.h"
#include "DSISimpleResultSet.h"
#include "DSIResultSetColumns.h"

namespace Simba
{
namespace Support
{
    class ILogger;
    class SqlTypeMetadata;
}
}

namespace RyftOne
{
    // Number of rows.
    const simba_uint32 PERSON_ROW_COUNT = 7;

    /// @brief RyftOne sample data table "Person".
    class R1PersonTable : public Simba::DSI::DSISimpleResultSet
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        ///
        /// @param in_log               The logger to use for this object. (NOT OWN)
        R1PersonTable(ILogger* in_log);

        /// @brief Destructor.
        ~R1PersonTable();

        /// @brief Returns the row count.
        ///
        /// @return The row count, or Simba::DSI::IResult::ROW_COUNT_UNKNOWN if the row count is 
        ///         unknown.
        simba_unsigned_native GetRowCount();

        /// @brief Retrieves an IColumns* which can provide access to column metadata for each 
        /// columns in the result. (NOT OWN)
        ///
        /// May return NULL if there are no columns associated with the result.
        ///
        /// @return IColumns reference that provides access to column metadata. (NOT OWN)
        Simba::DSI::IColumns* GetSelectColumns();

        /// @brief Determine if the number of rows in the result set is known.
        ///
        /// @return True if the number of rows in the result set is known; false otherwise.
        bool HasRowCount();

        /// @brief Fills in in_data with a chunk of data for the given column in the current row. 
        /// 
        /// The SqlTypeMetadata* used by in_data is the same SqlTypeMetadata* exposed by 
        /// the IColumn describing the column.
        ///
        /// The following procedure should be implemented by this method:
        ///
        ///     - if the data is null, call in_data->SetNull(true). in_offset and in_maxSize can 
        ///       be ignored
        ///
        ///     - if the data is not of a character or binary type, then the value should be 
        ///       copied into the pointer returned by in_data->GetBuffer(). in_offset and 
        ///       in_maxSize can be ignored
        ///
        ///     - if the data is of a character or binary type:
        ///         - in_offset specifies the starting point to copy data from, in # of bytes
        ///           from the start of that piece of data
        ///             - in_offset must be aligned properly to the start of a data element
        ///         - in_maxSize indicates the maximum number of bytes to copy
        ///             - if in_maxSize is RETRIEVE_ALL_DATA, it means that the whole piece of 
        ///               data should be copied
        ///         - the size of the data chunk being copied should be set with 
        ///             in_data->SetLength()
        ///             - this length is the number of full elements copied
        ///             - if there's only room for a partial element at the end, it does not 
        ///               need to be copied, and should not be included in the SetLength() length
        ///             - calling SetLength() must be done before copying data in, because it 
        ///               modifies the size of the data buffer
        ///         - the chunk of data starting at in_offset which is at a maximum in_size 
        ///           bytes long should be copied into the pointer returned by 
        ///           in_data->GetBuffer(). 
        ///             - null termination is not necessary, though for character data the buffer 
        ///               has extra room available at the end for a null terminator
        ///
        /// @param in_column        A column index. The first column uses index 0.
        /// @param in_data          Holds a buffer to store the requested data. (NOT OWN)
        /// @param in_offset        Number of bytes in the data to skip before copying into in_data.
        /// @param in_maxSize       Maximum number of bytes of data to returning in in_data.
        ///
        /// @return True if there is more data; false otherwise.
        bool RetrieveData(
            simba_uint16 in_column,
            SqlData* in_data,
            simba_signed_native in_offset,
            simba_signed_native in_maxSize);
        
    // Protected ===================================================================================
    protected:
        /// @brief Clean up memory by closing the cursor.
        /// 
        /// Called from CloseCursor() to ensure that the ResultSet performs any special 
        /// handling and that it clears associated memory.
        void DoCloseCursor();

        /// @brief Move to the next row.
        /// 
        /// Called from Move() to indicate that the cursor should now be moved to the next row.
        ///
        /// @return True if there are more rows in the ResultSet; false otherwise.
        bool MoveToNextRow();

    // Private =====================================================================================
    private:
        /// @brief Initializes the column metadata for the ResultSet.
        void InitializeColumns();

        /// @brief Initializes the data storage for the table.
        void InitializeData();

        /// A struct to hold data for each row.
        struct RowData
        {
            simba_wstring column1;
            simba_int32 column2;
            TDWExactNumericType column3;
        };

        // Log. (NOT OWN)
        ILogger* m_log;

        // Column metadata. (OWN)
        Simba::DSI::DSIResultSetColumns m_columns;

        // Table data.
        std::vector<RowData> m_data;

        // The iterator to the current row in the data.
        std::vector<RowData>::iterator m_rowItr;
    };
}

#endif
