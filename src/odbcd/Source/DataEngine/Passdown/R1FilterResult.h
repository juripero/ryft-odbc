// =================================================================================================
///  @file R1FilterResult.h
///
///  Definition of the class R1FilterResult
///
///  Copyright (C) 2008-2011 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1FILTERRESULT_H_
#define _RYFTONE_R1FILTERRESULT_H_

#include "RyftOne.h"
#include "R1Table.h"
#include "DSIExtSimpleResultSet.h"

namespace RyftOne
{
    /// @brief A result set representing filtered result.
    ///
    /// WARNING:
    /// For testing only.
    class R1FilterResult : public Simba::SQLEngine::DSIExtSimpleResultSet
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        /// 
        /// @param in_table         The table on which to apply filters. Cannot be NULL.
        /// @param in_filter        The string representing the dbase expression for the filter.
        R1FilterResult(
            Simba::Support::SharedPtr<R1Table> in_table, 
            const simba_wstring& in_filter, int in_hamming);

        /// @brief Destructor.
        virtual ~R1FilterResult();

        /// @brief Append an empty row to the end of the result set.
        ///
        /// The cursor should be positioned on the newly appended row when the function returns.
        ///
        /// @exception SEInvalidOperationException if the table does not support updating.
        /// @exception SEInvalidOperationException if the table was opened as read-only.
        virtual void AppendRow();

        /// @brief Indicate whether the table has the row count information.
        ///
        /// Without this information, certain query optimization (for example, join optimization) 
        /// can not be performed.
        ///
        /// @return true if the row count is known for this result set.
        virtual bool HasRowCount();

        /// @brief Returns the row count.
        ///
        /// @return The row count, or Simba::DSI::IResult::ROW_COUNT_UNKNOWN if the row count is 
        ///         unknown.
        virtual simba_unsigned_native GetRowCount();

        /// @brief Retrieves an IColumns* which can provide access to column metadata for each 
        /// columns in the result. (NOT OWN)
        ///
        /// May return NULL if there are no columns associated with the result.
        ///
        /// This method may be called after query preparation and before execution. If the 
        /// implementation is unable to return column metadata at this time, it may return an 
        /// IColumns holding no columns. This method will be called again after execution to 
        /// properly populate the IRD. Note that if implemented this way, ODBC functionality 
        /// related to columns (eg. the IRD descriptor, SQLDescribeCol(), SQLColAttribute())
        /// will not work properly between calling SQLPrepare() and SQLExecute().
        ///
        /// @return IColumns reference that provides access to column metadata. (NOT OWN)
        virtual Simba::DSI::IColumns* GetSelectColumns();

        /// @brief Get the catalog name of the table. 
        /// 
        /// If catalog is not supported, out_catalogName will be empty after the method is 
        /// returned.
        ///
        /// @param out_catalogName  The catalog name.
        virtual void GetCatalogName(simba_wstring& out_catalogName);

        /// @brief Get the schema name of the table. 
        /// 
        /// If catalog is not supported, out_schemaName will be empty after the method is returned.
        ///
        /// @param out_schemaName   The schema name.
        virtual void GetSchemaName(simba_wstring& out_schemaName);

        /// @brief Get the name of the table.
        ///
        /// @param out_tableName    The name of the table.
        virtual void GetTableName(simba_wstring& out_tableName);

        /// @brief Indicate that the current row is finished having data written to it.
        virtual void OnFinishRowUpdate();

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
        ///             - this length is the number of bytes copied
        ///             - if there's only room for a partial element at the end, it does not 
        ///               need to be copied, and should not be included in the SetLength() length
        ///             - calling SetLength() must be done before copying data in, because it 
        ///               modifies the size of the data buffer
        ///         - the chunk of data starting at in_offset which is at a maximum in_size 
        ///           bytes long should be copied into the pointer returned by 
        ///           in_data->GetBuffer(). 
        ///             - null termination is not necessary
        ///
        /// @param in_column    A column index. The first column uses index 0.
        /// @param in_data      Holds a buffer to store the requested data. (NOT OWN)
        /// @param in_offset    Number of bytes in the data to skip before copying into in_data.
        /// @param in_maxSize   Maximum number of bytes of data to return in in in_data.
        /// 
        /// @exception DSIException if an error occurs.
        ///
        /// @return True if there is more data; false otherwise.
        virtual bool RetrieveData(
            simba_uint16 in_column,
            Simba::Support::SqlData* in_data,
            simba_signed_native in_offset,
            simba_signed_native in_maxSize);

        /// @brief Write data to a column in the current row and specified column.
        ///
        /// Note that if in_isDefault is true, then in_sqlData will be NULL, and in_offset should
        /// be ignored.
        ///
        /// @param in_column        The column to write data to.
        /// @param in_sqlData       The container for the data to write to the column. (NOT OWN)
        /// @param in_offset        The offset into the column to start writing data at.
        /// @param in_isDefault     Flag indicating that the default value should be used for the
        ///                         column.
        ///
        /// @exception SEInvalidArgumentException if the column is invalid.
        /// @exception SEInvalidArgumentException if the offset is invalid for the column, or it 
        ///            will result in the given data overflowing the boundaries of the column.
        /// @exception SEInvalidOperationException if the table does not support updating.
        /// @exception SEInvalidOperationException if the table was opened as read-only.
        ///
        /// @return true if data is truncated; false otherwise.
        virtual bool WriteData(
            simba_uint16 in_column, 
            Simba::Support::SqlData* in_sqlData,
            simba_signed_native in_offset,
            bool in_isDefault);

    // Protected =====================================================================================
    protected:
               
        /// @brief Called from CloseCursor() to ensure that the ResultSet performs any special 
        /// handling and that it clears associated memory.
        virtual void DoCloseCursor();

        /// @brief Called from Move() to indicate that the cursor should now be moved to the next 
        /// row.
        ///
        /// @return True if there are more rows in the ResultSet; false otherwise.
        virtual bool MoveToNextRow();

        /// @brief Resets the cursor to before the first row.
        virtual void MoveToBeforeFirstRow();
 
    // Private =====================================================================================
    private:
        /// The table to operate on.
        Simba::Support::SharedPtr<R1Table> m_table;

        /// The dbase expression filter string
        simba_wstring m_filter;

        /// Flag indicating if Move() has been called yet.
        bool m_hasStartedFetch;
    };
}

#endif