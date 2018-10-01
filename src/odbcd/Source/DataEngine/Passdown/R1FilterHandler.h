// =================================================================================================
///  @file R1FilterHandler.h
///
///  Definition of the Class R1FilterHandler
///
///  Copyright (C) 2008-2013 Simba Technologies Incorporated
// =================================================================================================

#ifndef _RYFTONE_R1FILTERHANDLER_H_
#define _RYFTONE_R1FILTERHANDLER_H_

#include "RyftOne.h"
#include "R1Table.h"
#include "DSIExtSimpleBooleanExprHandler.h"

namespace RyftOne
{
    /// @brief A class that handles the passing down of filters.
    ///
    /// WARNING: This class is intended for demonstration purposes only, not for production use.
    class R1FilterHandler : public Simba::SQLEngine::DSIExtSimpleBooleanExprHandler
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        /// 
        /// @param in_table             The table on which to apply filters. Cannot be NULL.
        /// @param in_codeBaseSettings  The CodeBase settings. (NOT OWN)
        R1FilterHandler(
            Simba::Support::SharedPtr<R1Table> in_table,
            RyftOne_Database *ryft1);

        /// @brief Destructor.
        virtual ~R1FilterHandler();

        /// @brief Ask whether the handler is able to handle more Passdown() calls.
        /// 
        /// After each Passdown() call, this method is called. It allows the handler to stop the
        /// Passdown process if the handler is able to make the decision upon observing certain
        /// Boolean expressions. Essentially, it gives the customer handler the ability to
        /// "short circuit" the pass down process.
        /// 
        /// For example, if a handler is used for passing down a JOIN operation and once the JOIN
        /// is handled, it will not allow any more JOIN condition to be passed down. To avoid
        /// being asked by the SQL engine whether it could handle more clauses, the implementation
        /// for the JOIN handler could simply returns false after a JOIN condition is handled.
        /// 
        /// @return True if the handler is able to handle more conditions, false otherwise.
        virtual bool CanHandleMoreClauses();

        /// @brief Get the result set representing the result of applying the operations 
        /// successfully.
        /// 
        /// NOTE:
        ///
        /// 1) Passdown() must be successfully called one or more times before this method can be 
        ///    invoked, otherwise this method should throw an exception.
        /// 
        /// 2) The result set returned can be the same table used to create this IBooleanExprHandler 
        ///    object (@see DSIExtOperationHandlerFactory::CreateFilterHandler()). If a new result 
        ///    set is returned, it _MUST_ has the same ordered list of columns as the original
        ///    result set. The SQLEngine will update all columns that reference the original result
        ///    set to point to the new result set.
        ///
        /// Note that this function will still be called, even if Passdown() returns false, to allow
        /// for the DSII to partially handle filters and have the SQLEngine handle the rest. If the
        /// filter is entirely unhandled, then TakeResult() should return NULL.
        /// 
        /// @exception SEInvalidOperationException if Passdown() has not been called successfully.
        /// 
        /// @return The result set representing the result of applying the operations on success, 
        /// NULL otherwise.
        virtual Simba::Support::SharedPtr<Simba::SQLEngine::DSIExtResultSet> TakeResult();

    // Protected ===================================================================================
    protected:

        /// @brief Pass down the given Boolean expression representing a filter or join condition.
        /// 
        /// This default implementation always return false. That is, the Boolean expression is 
        /// not supported. A sub-class wishing to support this Boolean expression should override 
        /// this method.
        ///
        /// Note that TakeResult() will still be called, even if this function returns false, to 
        /// allow for the DSII to partially handle filters and have the SQLEngine handle the rest.
        ///
        /// @param in_node          The root of the Boolean expression. Cannot be NULL. (NOT OWN)
        /// 
        /// @return True if the Boolean expression is successfully passed down, false otherwise.
        virtual bool PassdownAnd(Simba::SQLEngine::AEAnd* in_node);

        /// @brief Pass down the given Boolean expression representing a filter or join condition.
        /// 
        /// This default implementation always return false. That is, the Boolean expression is 
        /// not supported. A sub-class wishing to support this Boolean expression should override 
        /// this method.
        ///
        /// @param in_node          The root of the Boolean expression. Cannot be NULL. (NOT OWN)
        /// 
        /// @return True if the Boolean expression is successfully passed down, false otherwise.
        virtual bool PassdownLikePredicate(Simba::SQLEngine::AELikePredicate* in_node);

        /// @brief Pass down the given Boolean expression representing a filter or join condition.
        /// 
        /// This default implementation always return false. That is, the Boolean expression is 
        /// not supported. A sub-class wishing to support this Boolean expression should override 
        /// this method.
        ///
        /// @param in_node          The root of the Boolean expression. Cannot be NULL. (NOT OWN)
        /// 
        /// @return True if the Boolean expression is successfully passed down, false otherwise.
        virtual bool PassdownNot(Simba::SQLEngine::AENot* in_node);

        /// @brief Pass down the given Boolean expression representing a filter or join condition.
        /// 
        /// This default implementation always return false. That is, the Boolean expression is 
        /// not supported. A sub-class wishing to support this Boolean expression should override 
        /// this method.
        ///
        /// Note that TakeResult() will still be called, even if this function returns false, to 
        /// allow for the DSII to partially handle filters and have the SQLEngine handle the rest.
        ///
        /// @param in_node          The root of the Boolean expression. Cannot be NULL. (NOT OWN)
        /// 
        /// @return True if the Boolean expression is successfully passed down, false otherwise.
        virtual bool PassdownOr(Simba::SQLEngine::AEOr* in_node);

        /// @brief Pass down the given filter. The filter is a comparison of the following form,
        ///
        ///     \<in_leftExpr\> \<in_compOp\> \<in_rightExpr\>
        ///
        /// Note that TakeResult() will still be called, even if this function returns false, to 
        /// allow for the DSII to partially handle filters and have the SQLEngine handle the rest.
        ///
        /// @param in_leftExpr      Left side expression. Cannot be NULL. (NOT OWN)
        /// @param in_rightExpr     Right side expression. Cannot be NULL. (NOT OWN)
        /// 
        /// @return true if the filter is successfully passed down, false otherwise.
        virtual bool PassdownSimpleComparison(
            Simba::SQLEngine::DSIExtColumnRef& in_leftExpr,
            Simba::SQLEngine::LiteralValue in_rightExpr,
            Simba::SQLEngine::SEComparisonType in_compOp);

        /// @brief Pass down the given Boolean expression representing a filter or join condition.
        /// 
        /// The Boolean expression is a comparison of the following form,
        ///
        ///     \<in_leftExpr\> \<in_compOp\> \<in_rightExpr\>
        ///
        /// This method is called by PassdownComparison(AEComparison*) when both operands are column
        /// references.
        ///
        /// Note that TakeResult() will still be called, even if this function returns false, to 
        /// allow for the DSII to partially handle filters and have the SQLEngine handle the rest.
        ///
        /// @param in_leftExpr      Left side expression. Cannot be NULL. (NOT OWN)
        /// @param in_rightExpr     Right side expression. Cannot be NULL. (NOT OWN)
        /// @param in_compOp        The comparison operator to apply to the expressions.
        /// 
        /// @return True if the Boolean expression as a filter or join condition is successfully 
        /// passed down, false otherwise.
        virtual bool PassdownSimpleComparison(
            Simba::SQLEngine::DSIExtColumnRef& in_leftExpr,
            Simba::SQLEngine::DSIExtColumnRef& in_rightExpr,
            Simba::SQLEngine::SEComparisonType in_compOp);


        /// @brief Pass down the given Boolean expression representing a filter or join condition.
        /// 
        /// It should only support \<in predicate\> of this form,
        ///
        ///     \<in_column\> IN (in_literals)
        /// 
        /// This method is called by PassdownInPredicate(AEInPredicate*) when the list of 
        /// expressions are all literal values.
        /// 
        /// If a sub-class only wishes to support \<in predicate\> of the form seen here, it should 
        /// only need to provide implementation for this method only. Otherwise, it needs to 
        /// override PassdownInPredicate(AEInPredicate*) and provide a default implementation 
        /// (simply returns false) for this method.
        ///
        /// Note that TakeResult() will still be called, even if this function returns false, to 
        /// allow for the DSII to partially handle filters and have the SQLEngine handle the rest.
        ///
        /// @param in_column        The left operand of the predicate. Cannot be NULL. (NOT OWN)
        /// @param in_literals      A list of literal values. Cannot be NULL. (NOT OWN)
        /// 
        /// @return True if the Boolean expression as a filter or join condition is successfully 
        /// passed down, false otherwise.
        virtual bool PassdownSimpleInPredicate(
            Simba::SQLEngine::DSIExtColumnRef& in_column, 
            Simba::SQLEngine::LiteralVector& in_literals);

        /// @brief Pass down a \<null predicate\>. 
        /// 
        /// This method is called by PassdownNullPredicate(AENullPredicate*) to support the 
        /// \<null predicate\> of this form,
        ///
        ///     \<in_column\> IS [NOT] NULL.
        /// 
        /// This method is called by PassdownNullPredicate(AENullPredicate*) when the expression 
        /// is a column reference.
        ///
        /// Note that TakeResult() will still be called, even if this function returns false, to 
        /// allow for the DSII to partially handle filters and have the SQLEngine handle the rest.
        ///
        /// @param in_column        The column expression. Cannot be NULL. (NOT OWN)
        /// @param in_isNull        True for "IS NULL", false for "IS NOT NULL".
        /// 
        /// @return True if the Boolean expression as a filter or join condition is successfully 
        /// passed down, false otherwise.
        virtual bool PassdownSimpleNullPredicate(
            Simba::SQLEngine::DSIExtColumnRef& in_column, 
            bool in_isNull);

    // Private =====================================================================================
    private:
        /// @brief Construct the filter string. 
        ///
        /// @param in_columnName        Name of the column. Cannot be NULL.
        /// @param in_columnSqlType     SQL Type of the column. Cannot be NULL.
        /// @param in_exprValue         Right side expression value as a simba_wstring.
        /// @param in_compOp            Comparison operation.  Cannot be NULL.
        void ConstructStringComparisonFilter(
            simba_wstring in_columnName,
            simba_int16 in_columnSqlType,
            const simba_wstring& in_exprValue,
            simba_wstring in_RelationalOp);

        /// @brief Construct the date filter string. 
        ///
        /// @param in_columnName        Name of the column. Cannot be NULL.
        /// @param in_dtType            The datetime format type.
        /// @param in_formatCustom      The datetime format specifier string.
        /// @param in_exprValue         Right side expression value as a simba_wstring.
        /// @param in_compOp            Comparison operation.  Cannot be NULL.        void ConstructDateComparisonFilter(
        void ConstructDateComparisonFilter(
            simba_wstring in_columnName,
            unsigned in_dtType,
            string& in_formatCustom,
            const simba_wstring& in_exprValue,
            Simba::SQLEngine::SEComparisonType in_compOp,
            simba_wstring in_RelationalOp);

        /// @brief Construct the time filter string. 
        ///
        /// @param in_columnName        Name of the column. Cannot be NULL.
        /// @param in_dtType            The datetime format type.
        /// @param in_formatCustom      The datetime format specifier string.
        /// @param in_exprValue         Right side expression value as a simba_wstring.
        /// @param in_compOp            Comparison operation.  Cannot be NULL.
        void ConstructTimeComparisonFilter(
            simba_wstring in_columnName,
            unsigned in_dtType,
            string& in_formatCustom,
            const simba_wstring& in_exprValue,
            Simba::SQLEngine::SEComparisonType in_compOp,
            simba_wstring in_RelationalOp);

        /// @brief Construct the number filter string. 
        ///
        /// @param in_columnName        Name of the column. Cannot be NULL.
        /// @param in_dtType            The datetime format type.
        /// @param in_formatCustom      The datetime format specifier string.
        /// @param in_exprValue         Right side expression value as a simba_wstring.
        /// @param in_compOp            Comparison operation.  Cannot be NULL.
        void ConstructNumberComparisonFilter(
            simba_wstring in_columnName,
            string& in_formatCustom,
            const simba_wstring& in_exprValue,
            Simba::SQLEngine::SEComparisonType in_compOp,
            simba_wstring in_RelationalOp);

        /// @brief Construct the currency filter string. 
        ///
        /// @param in_columnName        Name of the column. Cannot be NULL.
        /// @param in_dtType            The datetime format type.
        /// @param in_formatCustom      The datetime format specifier string.
        /// @param in_exprValue         Right side expression value as a simba_wstring.
        /// @param in_compOp            Comparison operation.  Cannot be NULL.
        void ConstructCurrencyComparisonFilter(
            simba_wstring in_columnName,
            string& in_formatCustom,
            const simba_wstring& in_exprValue,
            Simba::SQLEngine::SEComparisonType in_compOp,
            simba_wstring in_RelationalOp);

        /// @brief Construct the PCAP filter string. 
        ///
        /// @param in_columnName        Name of the column. Cannot be NULL.
        /// @param in_exprValue         Right side expression value as a simba_wstring.
        /// @param in_compOp            Comparison operation.  Cannot be NULL.
        void ConstructPCAPComparisonFilter(
            simba_wstring in_columnName,
            const simba_wstring& in_exprValue,
            Simba::SQLEngine::SEComparisonType in_compOp);

        /// @brief Construct the PCAP filter string. 
        ///
        /// @param in_columnName        Name of the column. Cannot be NULL.
        /// @param in_exprValue         Right side expression value as a simba_wstring.
        /// @param in_compOp            Comparison operation.  Cannot be NULL.
        void ConstructPCAPThinner(
            simba_wstring in_columnName,
            const simba_wstring& in_exprValue,
            int in_compOp);


        // The table on which to apply filters. (NOT OWN)
        Simba::Support::SharedPtr<R1Table> m_table;

        int m_limit;
        bool m_negate;
        simba_wstring m_query;
        ColFilters m_colFilters;

        /// Flag for successful pass down.
        bool m_isPassedDown;

        RyftOne_Database *m_ryft1;
    };
}

#endif
