// =================================================================================================
///  @file R1DataEngine.h
///
///  Definition of the Class R1DataEngine
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1DATAENGINE_H_
#define _RYFTONE_R1DATAENGINE_H_

#include "RyftOne.h"
#include "R1Catalog.h"
#include "DSIExtSqlDataEngine.h"

namespace Simba
{
namespace DSI
{
    class IStatement;
}
}
namespace RyftOne
{
    class R1ProcedureFactory;
    class R1QueryExecutor;
}

namespace RyftOne
{
    /// @brief RyftOne DataEngine implementation class.
    class R1DataEngine : public Simba::SQLEngine::DSIExtSqlDataEngine
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        /// 
        /// @param in_statement             The parent statement. (NOT OWN)
        R1DataEngine(Simba::DSI::IStatement* in_statement, RyftOne_Database *ryft1);

        /// @brief Destructor.
        virtual ~R1DataEngine();

        /// @brief Create a "handler" object to pass down operations such as filters, joins and
        /// aggregations.
        ///
        /// The default implementation in this class returns NULL. That is, no operation
        /// pass-down is supported.
        ///
        /// @return A "handler" object to pass down operations if supported, NULL otherwise.
        virtual AutoPtr<Simba::SQLEngine::DSIExtOperationHandlerFactory>
            CreateOperationHandlerFactory();

        /// @brief Create a table based on the given TableSpecification.
        ///
        /// The table name described in in_tableSpecification will already have been verified to not
        /// exist in the data source by calling DoesTableExist().
        /// The columns described in in_tableSpecification are generated using the IColumnFactory 
        /// retrieved from a DSIExtCustomBehaviorProvider generated by calling 
        /// CreateCustomBehaviorProvider().
        /// No validation is performed on the table constraints described in in_tableSpecification.
        ///
        /// @param in_specification The specification for the table.
        virtual void CreateTable(const SharedPtr<Simba::SQLEngine::TableSpecification> in_specification);

        /// @brief Determine if the given table exists.
        ///
        /// The base class provides a default implementation that uses OpenTable to determine if a
        /// table exists. Customers can override this method if they have a more efficient way of
        /// determining this. Customers should not call this method within the implementation of
        /// OpenTable unless they have provided an implementation.
        ///
        /// @param in_catalogName   The name of the catalog in which the table resides.
        /// @param in_schemaName    The name of the schema in which the table resides.
        /// @param in_tableName     The name of the table.
        ///
        /// @return True if the table exists in the data source.
        virtual bool DoesTableExist(
            const simba_wstring& in_catalogName,
            const simba_wstring& in_schemaName,
            const simba_wstring& in_tableName);

        /// @brief Drop the given table
        ///
        /// The existence of the table will be verified by calling DoesTableExist() before this 
        /// method is called.
        ///
        /// @param in_catalogName   The name of the catalog in which the table resides.
        /// @param in_schemaName    The name of the schema in which the table resides.
        /// @param in_tableName     The name of the table.
        /// @param in_dropOption    An enum of how the table should be dropped.
        ///
        /// Note: Currently, in_dropOption will always be TABLE_DROP_UNSPECIFIED
        virtual void DropTable(
            const simba_wstring& in_catalogName,
            const simba_wstring& in_schemaName,
            const simba_wstring& in_tableName,
            Simba::SQLEngine::DSIExtTableDropOption in_dropOption);

        /// @brief Makes a new metadata source that contains raw metadata.  
        /// 
        /// A map of column tags to literal string restrictions will be passed into the metadata 
        /// source, which enables the metadata source cut down on the size or the raw table.
        ///
        /// If SDK metadata filtering is enabled (via the DSI_FILTER_METADATA_SOURCE driver
        /// property), then the passed in restrictions are only a suggestion for the metadata
        /// source, in order to allow it to restrict what it returns before SDK filtering is
        /// performed.
        /// 
        /// Note that wildcard restrictions will _not_ be passed down when SDK metadata filtering is
        /// enabled as they require more involved filtering. If wildcard filters can be handled by 
        /// the data source, they can be passed down by setting the IDriver property 
        /// DSI_FILTER_METADATA_SOURCE to DSI_FMS_FALSE, in which case the SDK will do no filtering 
        /// of the results and pass all the restrictions. When this is done, your DSII will be 
        /// responsible for correctly  filtering and returning the results.
        /// 
        /// @param in_metadataTableID       Identifier to create the appropriate metadata table
        /// @param in_restrictions          Restrictions that may be applied to the metadata table.
        ///                                 Some restrictions may not be passed to the DSII if there
        ///                                 are wildcards. The wildcards are '_' for a single 
        ///                                 character and '%' for multiple characters. These are not
        ///                                 passed down to avoid problems with filtering the 
        ///                                 wildcards literally, and are instead handled by the SDK.
        ///                                 Note that if you wish to have full filtering, please
        ///                                 refer to DSI_FILTER_METADATA_SOURCE.
        /// @param in_escapeChar            Escape character used in filtering.
        /// @param in_identifierQuoteChar   Quote identifier, which is the quotation mark that this 
        ///                                 filter recognizes.
        /// @param in_filterAsIdentifier    Indicates if string filters are treated as identifiers.
        ///                                 If in_filterAsIdentifier is true, string filters are 
        ///                                 treated as identifiers.  Otherwise, filters are not
        ///                                 treated as identifiers.
        ///
        /// @return New metadata source that contains raw metadata, or NULL if not supported. (OWN)
        Simba::DSI::DSIMetadataSource* MakeNewMetadataTable(
            Simba::DSI::DSIMetadataTableID in_metadataTableID, 
            Simba::DSI::DSIMetadataRestrictions& in_restrictions,
            const simba_wstring& in_escapeChar, 
            const simba_wstring& in_identifierQuoteChar, 
            bool in_filterAsIdentifier);

        /// @brief Open a stored procedure.
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
        /// @param in_procName      The name of the stored procedure to open.
        ///
        /// @return the opened procedure, NULL if the procedure does not exist.
        virtual SharedPtr<Simba::SQLEngine::DSIExtProcedure> OpenProcedure(
            const simba_wstring& in_catalogName,
            const simba_wstring& in_schemaName,
            const simba_wstring& in_procName);

        /// @brief Open a physical table/view.
        /// 
        /// This method will be called during the preparation of a SQL statement.
        ///
        /// Once the table is opened, it should allow retrieving of metadata. That is, calling
        /// GetSelectColumns() on the returned table should return column metadata. SimbaEngine
        /// needs the table metadata to infer the column metadata of the result set if the 
        /// SQL statement is a query.
        /// 
        /// Before data can be retrieved from the table, SetCursorType() will have to called.
        /// Since this is done at the execution time, the DSII should _NOT_ try to make the data 
        /// ready for retrieval until SetCursorType() is called.
        ///
        /// The DSII decides how catalog and schema are interpreted or supported. If the same
        /// table is asked to open twice (that is, OpenTable() is called twice), the DSII _MUST_
        /// return two separate DSIExtResultSet instances since two cursors will be needed.
        ///
        /// SimbaEngine will ensure this method is called only once if the table is referenced only
        /// once in a SQL statement.
        /// 
        /// @param in_catalogName   The name of the catalog in which the table resides.
        /// @param in_schemaName    The name of the schema in which the table resides.
        /// @param in_tableName     The name of the table to open.
        /// @param in_openType      An enum indicating how the table should be opened.
        ///
        /// @return the opened table, NULL if the table does not exist.
        virtual Simba::Support::SharedPtr<Simba::SQLEngine::DSIExtResultSet> OpenTable(
            const simba_wstring& in_catalogName,
            const simba_wstring& in_schemaName,
            const simba_wstring& in_tableName,
            Simba::SQLEngine::DSIExtTableOpenType in_openType);

    protected:
        RyftOne_Database *m_ryft1;

        /// The R1ProcedureFactory instance (OWN).
        AutoPtr<R1ProcedureFactory> m_procedureFactory;
    };
}

#endif
