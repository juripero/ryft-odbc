//==================================================================================================
///  @file R1Table.h
///
///  Definition of the class R1Table.
///
///  Copyright (C) 2008-2014 Simba Technologies Incorporated.
//==================================================================================================

#ifndef _RYFTONE_R1TABLE_H_
#define _RYFTONE_R1TABLE_H_

#include "RyftOne.h"
#include "R1Catalog.h"

#include "AutoPtr.h"
#include "DSIExtSimpleResultSet.h"
#include "IColumns.h"

namespace Simba
{
namespace Support
{
    class ILogger;
    class IWarningListener;
}
}

namespace RyftOne
{
    /// @brief Quickstart DSII implementation of IResult.
    ///
    /// This class sub-classes from DSIExtSimpleResultSet.
    class R1Table : public Simba::SQLEngine::DSIExtSimpleResultSet
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        ///
        /// @param in_settings              The connection settings. (NOT OWN)
        /// @param in_log                   The log to use for logging. (NOT OWN)
        /// @param in_tableName             The name of the table to open.
        /// @param in_warningListener       The warning listener used to post warnings. (NOT OWN)
        /// @param in_isODBCV3              Whether ODBC 3.X is in use
        ///
        /// @exception ErrorException if there is an error opening the table.
        R1Table(
            ILogger *in_log,
            const simba_wstring& in_tableName,
            RyftOne_Database *ryft1,
            IWarningListener* in_warningListener,
            bool in_isODBCV3);

        void AppendFilter(simba_wstring &in_filter);

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
        bool HasRowCount();

        /// @brief Returns the row count.
        ///
        /// @return The row count, or Simba::DSI::IResult::ROW_COUNT_UNKNOWN if the row count is
        ///         unknown.
        simba_unsigned_native GetRowCount();

        /// @brief Retrieves an IColumns* for each column in the result.
        ///
        /// An IColumns has functions that provide access to column metadata.
        ///
        /// @return The IColumns for each column in the result (NOT OWN).
        Simba::DSI::IColumns* GetSelectColumns();

        /// @brief Get the catalog name of the table.
        ///
        /// If catalog is not supported, out_catalogName will be 'null' after the method returns.
        ///
        /// @param out_catalogName  The catalog name.
        virtual void GetCatalogName(simba_wstring& out_catalogName);

        /// @brief Get the schema name of the table.
        ///
        /// If schemas are not supported, out_schemaName will be 'null' after the method returns.
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
        ///         be ignored
        ///
        ///     - if the data is not of a character or binary type, then the value should be
        ///         copied into the pointer returned by in_data->GetBuffer(). in_offset and
        ///         in_maxSize can be ignored
        ///
        ///     - if the data is of a character or binary type:
        ///         - in_offset specifies the starting point to copy data from, in # of bytes
        ///              from the start of that piece of data
        ///             - in_offset must be aligned properly to the start of a data element
        ///         - in_maxSize indicates the maximum number of bytes to copy
        ///             - if in_maxSize is RETRIEVE_ALL_DATA, it means that the whole piece of
        ///              data should be copied
        ///         - the size of the data chunk being copied should be set with
        ///              in_data->SetLength()
        ///             - this length is the number of bytes copied
        ///             - if there's only room for a partial element at the end, it does not
        ///              need to be copied, and should not be included in the SetLength() length
        ///             - calling SetLength() must be done before copying data in, because it
        ///              modifies the size of the data buffer
        ///         - the chunk of data starting at in_offset which is at a maximum in_size
        ///              bytes long should be copied into the pointer returned by
        ///              in_data->GetBuffer().
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
        bool RetrieveData(
            simba_uint16 in_column,
            SqlData* in_data,
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

        void GetTypeFormatSpecifier(
            simba_uint16 in_column,
            unsigned *out_dtType,
            string& out_formatSpec);

        bool IsStructuredType();


        // PCAP

        bool IsPCAPDatabase();

        bool HasResultThinner(simba_wstring& columnName);
        string GetResultThinnerQuery(simba_wstring& columnName, simba_int16 type);

    // Protected ===================================================================================
    public:
        /// @brief Destructor.
        ///
        /// The destructor is protected to prevent client code from explicitly using delete instead
        /// of calling Release() to release the resources.
        ~R1Table();

        /// @brief Called from CloseCursor() to ensure that the ResultSet performs any special
        /// handling and that it clears associated memory.
        void DoCloseCursor();

        /// @brief Resets the cursor to before the first row.
        ///
        /// After Reset() is called, Move() must be called prior to the first call to
        /// RetrieveData() to position the cursor on the first row.
        void MoveToBeforeFirstRow();

        /// @brief Called from Move() to indicate that the cursor should now be moved to the next
        /// row.
        ///
        /// @return True if there are more rows in the ResultSet; false otherwise.
        bool MoveToNextRow();

    // Private =====================================================================================
    private:
        /// @brief Convert the data from the unicode string format stored in text files to the
        /// expected SQL format.
        ///
        /// @param in_column    A column index. The first column uses index 0.
        /// @param in_data      Holds a buffer to store the requested data. (NOT OWN)
        /// @param in_offset    Number of bytes in the data to skip before copying into in_data.
        /// @param in_maxSize   Maximum number of bytes of data to return in in_data.
        ///
        /// @return True if there is more data left after the retrieved chunk, false otherwise.
        bool ConvertData(
            simba_uint16 in_column,
            SqlData* in_data,
            simba_signed_native in_offset,
            simba_signed_native in_maxSize);

        /// @brief Create the IColumn classes for each of the columns in the table.
        ///
        /// @param in_isODBCV3  Whether ODBC 3.X is in use
        void InitializeColumns(bool in_isODBCV3);

        /// @brief Reads the value of the column into a string
        ///
        /// @param in_column   A column index. The first column uses index 0.
        ///
        /// @return the value of the column
        //simba_wstring ReadWholeColumnAsString(simba_uint16 in_column);

        // Reference to the ILogger. (NOT OWN)
        ILogger* m_log;

        // The columns in this table. (OWN)
        AutoPtr<Simba::DSI::IColumns> m_columns;

        // The name of the table. Cannot be empty.
        simba_wstring m_tableName;

        RyftOne_Database *m_ryft1;

        IQueryResult *m_result;

        // Warning listener for posting warnings. (NOT OWN)
        IWarningListener* m_warningListener;

        // Flag indicating if Move() has been called yet.
        bool m_hasStartedFetch;

        bool m_hasInsertedRecords;
        bool m_isAppendingRow;
    };
}
#endif
