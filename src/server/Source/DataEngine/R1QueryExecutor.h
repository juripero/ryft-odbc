// =================================================================================================
///  @file R1QueryExecutor.h
///
///  Definition of the Class R1QueryExecutor
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1QUERYEXECUTOR_H_
#define _RYFTONE_R1QUERYEXECUTOR_H_

#include "RyftOne.h"
#include "ryft1_library.h"
#include "IQueryExecutor.h"
#include "AutoPtr.h"

namespace Simba
{
namespace DSI
{
    class DSIResults;
}
namespace Support
{
    class ILogger;
}
}

namespace RyftOne
{
    /// @brief Implementation of a query executor object, which represents a prepared query.
    ///
    /// This object is used to get parameter metadata, result set objects, and perform actual 
    /// execution one or more times.
    class R1QueryExecutor : public Simba::DSI::IQueryExecutor
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        /// 
        /// @param in_log               The logger to use for this object. (NOT OWN)
        /// @param in_isSelect          True if doing a SELECT query; false otherwise.
        /// @param in_isParameterized   True if a parameter exists in the query; false otherwise.
        R1QueryExecutor(
            Simba::Support::ILogger* in_log, 
            RyftOne_Statement *ryft1stmt, const simba_wstring& in_sqlQuery
            );

        /// @brief Destructor.
        virtual ~R1QueryExecutor();

        /// @brief Clears parameter data that has been pushed down and cancels the current 
        /// execution.
        ///
        /// Things that may be canceled concurrently by this include:
        ///  - Execute()
        ///  - PopulateParameters()
        ///  - PushParamData()
        ///  - FinalizePushedParamData()
        ///  - GetNumParams()
        ///  - IResults::Next()
        ///  - IResult::Move()
        ///  - IResult::RetrieveData()
        ///
        /// It is not required that any of the above be canceled if the implementation is unable
        /// to perform the cancellation. However, that is not an error and CancelExecute() should
        /// not throw an exception when it was unable to cancel.
        /// 
        /// If any of the above are canceled concurrently, the thread being canceled 
        /// (not the thread calling CancelExecute()) should throw an OperationCanceledException [HY008].
        void CancelExecute();

        /// @brief Clears any cancellation flags set if CancelExecute was called.
        virtual void ClearCancel();

        /// @brief Provides the ODBC layer with metadata about parameters found in a query 
        /// prepared using Prepare().
        ///
        /// From the perspective of ODBC, this method implements IPD auto-population during 
        /// a call to SQLPrepare(). This function is called during SQLPrepare() if the statement 
        /// property DSI_CONN_AUTO_IPD is set to DSI_PROP_AUTO_IPD_ON. 
        ///
        /// The implementation of this method should register each parameter found in a query
        /// by calling in_parameterManager->RegisterParameter(uint16)
        ///
        /// After registering a parameter, the implementation of this method should set 
        /// metadata information about the parameter using the IParameterSource* returned 
        /// by IParameterManager::RegisterParameter(). IParameterSource*s accessed in this way 
        /// are deleted when PopulateParameters() is finished, so they should not be cached 
        /// and accessed outside of this method.
        ///
        /// An attempt to cache an IParameterManager* and register parameters outside of a call
        /// to SQLPrepare() will throw a CallbackException.
        ///
        /// If parameters are to be registered, they must contain all integers from 1 to the
        /// highest parameter index. For example, if the highest parameter index is 5, the 
        /// parameters {1, 2, 3, 4, 5} must be registered. They can be registered in any order 
        /// and any parameter can be registered multiple times. Parameter index 0 cannot be used.
        /// If the query in question has no parameters, then no parameters should be registered. 
        /// 
        /// IParameterSource->SetSQLType() should be called before setting the precision, length,
        /// interval length, and scale fields. Setting the SQLType implicitly sets those fields 
        /// to default values, so they should only be set afterwards.
        /// 
        /// @param in_paramManager      Register for the parameters. (NOT OWN)
        ///
        /// @exception CallbackException if it attempts to cache an IParameterManager* and register
        /// parameters outside of a call to SQLPrepare().
        void PopulateParameters(Simba::DSI::IParameterManager* in_paramManager);

        /// @brief Pushes part of an input parameter value before executing in an 
        /// IParameterSource*. 
        ///
        /// This value should be stored for use later during execution.
        ///
        /// This method will only be called a once, at a maximum, for any parameter set/parameter 
        /// combination where the parameter has a non-character/binary 
        /// data type.
        ///
        /// For parameters with character or binary data types, this method may be 
        /// called multiple times for the same parameter set/parameter combination. The multiple 
        /// parts should be concatenated together to get the complete value.
        ///
        /// Calling SetOutputValue() on in_paramSrc will trigger a BadStateException, even if the 
        /// parameter is an input/output parameter. Output values should only be set during 
        /// IQueryExecutor::Execute().
        ///
        /// @param in_paramSet          A parameter set.
        /// @param in_paramSrc          An input parameter source. (NOT OWN)
        ///
        /// @exception BadDefaultParamException if a "default" parameter value for a parameter where
        /// "default" has no meaning is passed in.
        void PushParamData(
            simba_unsigned_native in_paramSet,
            Simba::DSI::IParameterSource* in_paramSrc);

        /// @brief Informs the IQueryExecutor that all parameter values which will be pushed have 
        /// been pushed prior to query execution. 
        ///
        /// After the next Execute() call has finished with this data, it may be cleared from 
        /// memory, even if the Execute() call results in an exception being thrown. The first 
        /// subsequent call to PushParamData() should behave as if the executor has a clear cache 
        /// of pushed parameter values.
        void FinalizePushedParamData();

        /// @brief Executes a query, using the given input and output IParameterSetIter.
        /// 
        /// Quick definitions:
        /// IParameterSetIter:  A forward-only iterator through an input or output parameter set.
        /// IParameterSet:      A parameter set obtained from an IParameterSetIter.
        /// IParameterSource:   A single parameter obtained from an IParameterSet.
        /// 
        /// Rules for using an IParameterSetIter:
        /// 1) Each IParameterSetIter is initially positioned before the first parameter set.
        /// 2) Before operating on an IParameterSetIter, the data source may check the 
        ///     IParameterSetIter to see if it is empty (no parameter sets available).
        /// 3) Next(), which positions the iterator at the first parameter set (if calling Next() 
        ///     for the first time) or at the next parameter set, must be called before using the 
        ///     IParameterSet* obtained through IParameterSetIter.  Otherwise, the IParameterSet* 
        ///     is in an undefined state.
        /// 4) The IParameterSet* obtained from the IParameterSetIter will not change thoughout the 
        ///     lifetime of the IParameterSetIter.  This means that it is sufficient to get the 
        ///     IParameterSet* only once, and asking the IParameterSetIter for the IParameterSet* 
        ///     at any time will give you the same IParameterSet*. There will always be an 
        ///     IParameterSet* associated with an IParameterSetIter.  Please see IParameterSet and 
        ///     IParameterSetIter documentation for more details.
        /// 5) Next() will position the iterator to the next parameter set.  This affects the 
        ///     contents and attributes of the IParameterSet* obtained through the IParameterSetIter.
        ///     However, the same IParameterSet* object will always be returned from a
        ///     particular IParameterSetIter.
        /// 6) For the input IParameterSetIter, Next() must be called to position the iterator at the 
        ///     parameter set of interest before trying to obtain input data buffers.
        /// 7) For the output IParameterSetIter, Next() must be called after setting the output value 
        ///     on the IParameterSource*.  Next() will commit the output value set on the 
        ///     IParameterSource* and transfer the output value to the application.  If Next() is 
        ///     not called on an output IParameterSetIter, the output value set on the 
        ///     IParameterSource* will not be committed nor transferred to the application.
        /// 8) The index of the current parameter set accessed through an iterator may be 
        ///     requested from that iterator.
        /// 
        /// Rules for using IParameterSet:
        /// 1) The state of the IParameterSet and its contents is dependent on the position of 
        ///     the parent IParameterSetIter.  Please see IParameterSetIter for details.
        /// 2) A list of IParameterSource* can be obtained from the IParameterSet.  The list of 
        ///     IParameterSource* may be empty.  This may happen when: a) no input parameters in 
        ///     the SQL statement for input IParameterSet, or b) no output parameters in the SQL 
        ///     statement for output IParameterSet.
        /// 3) The data source may set the status of the current parameter set execution, whether 
        ///     the execute was successful, if there were warnings (success with info), etc.  
        ///     Setting the status is recommended, and this status information is reflected on 
        ///     the application side.  The status codes can be found in IParameterSet.h.
        /// 4) The parameter set index maybe obtained from the IParameterSet.
        /// 
        /// Rules for using IParameterSource:
        /// 1) Please see IParameterSource.h for details on what methods are allowed during 
        ///     executing.
        /// 2) Data source is only allowed to get the data buffer (thus the input data) for 
        ///     IParameterSource obtained through the input IParameterSetIter and IParameterSet.
        /// 3) Data source is only allowed to set output value on IParameterSource obtained 
        ///     through the output IParameterSetIter and IParameterSet.  If the data source does 
        ///     not set an output value on the IParameterSource, then IParameterSource defaults 
        ///     the output value to null.
        /// 
        /// This method should do the following:
        /// 1) Get the input IParameterSet* from the input IParameterSetIter*.  This can be done 
        ///     at any time.
        ///     Call Next() on the input IParameterSetIter* to position the iterator at the first 
        ///     parameter set (if calling Next() for the first time), or at the next parameter 
        ///     set.  Next() must be called on the input IParameterSetIter* before using the input 
        ///     IParameterSet*.
        ///     Get input data from the list of IParameterSource* (if not empty) obtained from 
        ///     the input IParameterSet*. Input parameters may represent NULL, or default 
        ///     parameters for stored procedures.
        ///     Set the status of the current parameter set.
        /// 2) Execute using data from the input parameter set, as well as any pushed 
        ///     parameter values.
        /// 3) Get the output IParameterSet* from the output IParameterSetIter*.  This can be done 
        ///     at any time.
        ///     Call Next() on the output IParameterSetIter* to position the iterator at the first 
        ///     parameter set (if calling Next() for the first time), or at the next parameter set.
        ///     Next() must be called on the output IParameterSetIter* before using the output 
        ///     IParameterSet*.
        ///     Set output value on the list of IParameterSource* (if not empty) obtained from 
        ///     the output IParameterSet*.  Either a valid buffer with appropriate length or a 
        ///     null buffer is required.  If the contents of the input buffer is longer than the 
        ///     size of the IParameterSource* buffer, the contents will be truncated without 
        ///     throwing any exceptions.
        ///     Set the status of the current parameter set.
        /// 
        /// Some input parameter values may have already pushed down through calls to 
        /// IQueryExecutor::PushParamData(). Call IParameterSource->IsPushedValue() to find out 
        /// if the data has already been pushed. If it has, calling IParameterSource->GetInputData()
        /// will throw a NoDataInputParamException.
        ///
        /// If there are any warnings that need to be posted for the application, those can be posted 
        /// through the in_warningListener.
        ///
        /// After execution, the IQueryExecutor's internal cursor should be positioned 
        /// before the start of the result set.
        ///
        /// This method should throw a BadDefaultParamException if a "default" parameter 
        /// value for a parameter where "default" has no meaning is passed in.
        ///
        /// At the completion of this method, any pushed-down parameter values may be 
        /// cleared from memory, even if this method throws an exception.
        ///
        /// @param in_warningListener      Warning listener to post warnings against. (NOT OWN)
        /// @param in_inputParamSetIter    Iterator over input parameter sets. (NOT OWN)
        /// @param in_outputParamSetIter   Iterator over output parameter sets. (NOT OWN)
        /// @param in_paramSetStatusSet    Parameter set status set. (NOT OWN)
        ///
        /// @exception BadDefaultParamException if a "default" parameter value for a parameter where 
        /// "default" has no meaning is passed in.
        void Execute(
            Simba::Support::IWarningListener* in_warningListener, 
            Simba::DSI::IParameterSetIter* in_inputParamSetIter,
            Simba::DSI::IParameterSetIter* in_outputParamSetIter,
            Simba::DSI::IParameterSetStatusSet* in_paramSetStatusSet);

        /// @brief Returns the results from the execution.
        ///
        /// This function will be called after query preparation and before execution to retrieve 
        /// metadata about the results that will be returned for this stored procedure. If accurate
        /// metadata cannot be supplied, returning at least a single result of the correct type with 
        /// no metadata will suffice as long as correct metadata is supplied after Execute(). Be 
        /// aware that some applications may have problems if metadata is not available at prepare 
        /// time.
        ///
        /// @return Results from the execution. (NOT OWN)
        virtual Simba::DSI::IResults* GetResults();

        /// @brief Returns the number of parameters in the query.
        ///
        /// Note that the number of parameters should be available even if auto-IPD is disabled.
        ///
        /// @return The number of parameters in the query.
        virtual simba_uint16 GetNumParams();

    // Private =====================================================================================
    private:
        // The connection ILogger. (NOT OWN)
        Simba::Support::ILogger* m_log;

        simba_wstring m_sqlQuery;

        RyftOne_Statement *m_ryft1stmt;

        // The results managed by the executor. (OWN)
        AutoPtr<Simba::DSI::DSIResults> m_results;
    };
}

#endif
