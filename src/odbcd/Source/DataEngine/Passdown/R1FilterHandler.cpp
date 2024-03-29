// =================================================================================================
///  @file R1FilterHandler.cpp
///
///  Implementation of the Class R1FilterHandler
///
///  Copyright (C) 2008-2012 Simba Technologies Incorporated
// =================================================================================================

#include "R1FilterHandler.h"

#include "AEComparison.h"
#include "AELiteral.h"
#include "AEParameter.h"
#include "AELikePredicate.h"
#include "AENot.h"
#include "AEAnd.h"
#include "AEOr.h"
#include "R1FilterResult.h"
#include "DSIExtColumnRef.h"
#include "DSIExtResultSet.h"
#include "IColumns.h"
#include "NumberConverter.h"
#include "SqlDataTypeUtilities.h"
#include "SqlTypeMetadata.h"

using namespace RyftOne;
using namespace Simba::SQLEngine;
using namespace Simba::DSI;

// Helpers =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
simba_wstring GetParameterValue(AEParameter* in_parameter)
{
    simba_uint32 dataLength = 0;
    const void* data = in_parameter->GetInputData(dataLength);

    if ((0 == dataLength) && (NULL == data))
    {
        // Return a "null" value.
        return simba_wstring();
    }

    // Use the TDW type to avoid ambiguities about sign.
    switch (in_parameter->GetMetadata()->GetTDWType())
    {
        case TDW_SQL_CHAR:
        case TDW_SQL_VARCHAR:
        case TDW_SQL_LONGVARCHAR:
        case TDW_SQL_WCHAR:
        case TDW_SQL_WVARCHAR:
        case TDW_SQL_WLONGVARCHAR:
        {
            return simba_wstring(
                reinterpret_cast<const simba_byte*>(data), 
                dataLength,
                in_parameter->GetMetadata()->GetEncoding());
        }

        case TDW_SQL_NUMERIC:
        case TDW_SQL_DECIMAL:
        {
            return reinterpret_cast<const TDWExactNumericType*>(data)->ToString();
        }

        case TDW_SQL_FLOAT:
        case TDW_SQL_DOUBLE:
        {
            return NumberConverter::ConvertDouble64ToWString(*reinterpret_cast<const simba_double64*>(data));
        }

        case TDW_SQL_REAL:
        {
            return NumberConverter::ConvertDouble32ToWString(*reinterpret_cast<const simba_double32*>(data));
        }

        case TDW_SQL_STINYINT:
        {
            return NumberConverter::ConvertToWString(*reinterpret_cast<const simba_int8*>(data));
        }

        case TDW_SQL_UTINYINT:
        {
            return NumberConverter::ConvertToWString(*reinterpret_cast<const simba_uint8*>(data));
        }

        case TDW_SQL_USMALLINT:
        {
            return NumberConverter::ConvertToWString(*reinterpret_cast<const simba_uint16*>(data));
        }

        case TDW_SQL_SSMALLINT:
        {
            return NumberConverter::ConvertToWString(*reinterpret_cast<const simba_int16*>(data));
        }

        case TDW_SQL_UINTEGER:
        {
            return NumberConverter::ConvertToWString(*reinterpret_cast<const simba_uint32*>(data));
        }

        case TDW_SQL_SINTEGER:
        {
            return NumberConverter::ConvertToWString(*reinterpret_cast<const simba_int32*>(data));
        }

        case TDW_SQL_UBIGINT:
        {
            return NumberConverter::ConvertToWString(*reinterpret_cast<const simba_uint64*>(data));
        }

        case TDW_SQL_SBIGINT:
        {
            return NumberConverter::ConvertToWString(*reinterpret_cast<const simba_int64*>(data));
        }

        default:
        {
            // This shouldn't be hit due to checks before to determine what types of parameters
            // can be passed down.
            SETHROW_INVALID_ARG();
        }
    }
}

// Public ==========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
R1FilterHandler::R1FilterHandler(
    SharedPtr<R1Table> in_table, RyftOne_Database *ryft1) :
    m_ryft1(ryft1),
    m_table(in_table),
    m_isPassedDown(false),
    m_negate(false),
    m_limit(0)
{
    assert(!in_table.IsNull());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
R1FilterHandler::~R1FilterHandler()
{
    ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::CanHandleMoreClauses()
{
    // Always returns true since any incrementally adding more filter clauses is supported.
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
SharedPtr<DSIExtResultSet> R1FilterHandler::TakeResult()
{
    if (!m_isPassedDown)  {
        // Return NULL and let the engine do the filtering.
        return SharedPtr<DSIExtResultSet>();
    }

    // Return filter result.
    return SharedPtr<DSIExtResultSet>(new R1FilterResult(m_table, m_query, m_limit, m_colFilters));
}

// Protected =======================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownAnd(AEAnd *in_node)
{
    AEBooleanExpr *lExpr = (AEBooleanExpr *)in_node->GetChild(0);
    AEBooleanExpr *rExpr = (AEBooleanExpr *)in_node->GetChild(1);
    simba_wstring pushFilter = m_query;
    m_query.Clear();
    m_query += "(";
    if(DSIExtAbstractBooleanExprHandler::Passdown(lExpr)) {
        if(m_table->IsPCAPDatabase()) {
            m_query += " && ";
        }
        else
            m_query += " AND ";
        if(DSIExtAbstractBooleanExprHandler::Passdown(rExpr)) {
            m_query += ")";
            m_query = pushFilter + m_query;
            m_isPassedDown = true;
            return true;
        }
    }
    m_query = pushFilter;
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownNot(AENot* in_node)
{
    AEBooleanExpr *Expr = (AEBooleanExpr *)in_node->GetChild(0);
    m_negate = true;
    if(DSIExtAbstractBooleanExprHandler::Passdown(Expr)) {
        m_negate = false;
        m_isPassedDown = true;
        return true;
    }
    m_negate = false;
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownOr(AEOr* in_node)
{
    AEBooleanExpr *lExpr = (AEBooleanExpr *)in_node->GetChild(0);
    AEBooleanExpr *rExpr = (AEBooleanExpr *)in_node->GetChild(1);
    simba_wstring pushFilter = m_query;
    m_query.Clear();
    m_query += "(";
    if(DSIExtAbstractBooleanExprHandler::Passdown(lExpr)) {
        if(m_table->IsPCAPDatabase()) {
            m_query += " || ";
        }
        else
            m_query += " OR ";
        if(DSIExtAbstractBooleanExprHandler::Passdown(rExpr)) {
            m_query += ")";
            m_query = pushFilter + m_query;
            m_isPassedDown = true;
            return true;
        }
    }
    m_query = pushFilter;
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownLikePredicate(AELikePredicate* in_node) 
{
    bool negate = false;
    int count = in_node->GetChildCount();

    AEValueExpr* lExpr = in_node->GetLeftOperand();
    AEValueExpr* rExpr = in_node->GetRightOperand();

    AENodeType lType = lExpr->GetNodeType();
    AENodeType rType = rExpr->GetNodeType();

    if(lType != AE_NT_VX_COLUMN) {
        std::swap(lExpr, rExpr);
        std::swap(lType, rType);
    }

    if(rType == AE_NT_VX_NEGATE) {
        rExpr = (AEValueExpr*)rExpr->GetChild(0);
        rType = rExpr->GetNodeType();
        negate = true;
    }

    if(lType != AE_NT_VX_COLUMN && rType != AE_NT_VX_LITERAL)
        return false;

    DSIExtColumnRef colRef;
    if(!GetTableColRef(lExpr, colRef))
        return false;

    simba_wstring relationalOp;
    if(m_negate)
        relationalOp = "NOT_";
    relationalOp += "CONTAINS";

    // Get information about the column in the table that the filter is applied to.
    IColumns* tableColumns = colRef.m_table->GetSelectColumns();
    assert(tableColumns);
    IColumn *lColumn = tableColumns->GetColumn(colRef.m_colIndex);
    SqlTypeMetadata* columnMetadata = lColumn->GetMetadata();
    simba_int16 columnSqlType = columnMetadata->GetSqlType();

    simba_wstring columnName;
    lColumn->GetLabel(columnName);

    // ryftprim doesn't like the trailing .[] which can happen in a JSON array that contains unnamed elements
    simba_wstring trailingArray = L".[]";
    simba_int32 loc = columnName.Find(L".[]", columnName.GetLength() - trailingArray.GetLength());
    if(loc != SIMBA_NPOS)
        columnName.Remove(loc, trailingArray.GetLength());

    // Get the literal type of RHS of comparison expression.
    PSLiteralType exprLiteralType = rExpr->GetAsLiteral()->GetLiteralType();
    simba_int16 paramSqlType = rExpr->GetMetadata()->GetSqlType();

    // Get the literal and prepend it with '-' if it's been negated.
    simba_wstring literalVal = rExpr->GetAsLiteral()->GetLiteralValue();

    unsigned typeSpecial = TYPE_NONE;
    string formatSpecial;
    m_table->GetTypeFormatSpecifier(colRef.m_colIndex, &typeSpecial, formatSpecial);

    if (m_table->IsPCAPDatabase() && columnName != L"payload") {

        if (m_table->HasResultThinner(columnName)) {

            // Contruct the filter string based upon built in filters
            if (!m_negate) {
                ConstructPCAPThinner(columnName, literalVal, FILTER_LIKE);
            }
            else
                ConstructPCAPThinner(columnName, literalVal, FILTER_NOT_LIKE);

            m_isPassedDown = true;
            return true;
        }
    }
    else if(((exprLiteralType == PS_LITERAL_DATE) || (exprLiteralType == PS_LITERAL_CHARSTR)) && 
        ((columnSqlType == SQL_TYPE_DATE) || (columnSqlType == SQL_DATE) ||
            (columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP))) 
    {
        if (exprLiteralType == PS_LITERAL_CHARSTR)
            GetOptions(literalVal);

        // Construct the filter string for input to Ryft.
        ConstructDateComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, SE_COMP_EQ, relationalOp);

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if(((exprLiteralType == PS_LITERAL_TIME) || (exprLiteralType == PS_LITERAL_CHARSTR)) && 
        ((columnSqlType == SQL_TYPE_TIME) || (columnSqlType == SQL_TIME) || 
            (columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)))
    {
        if (exprLiteralType == PS_LITERAL_CHARSTR)
            GetOptions(literalVal);

        ConstructTimeComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, SE_COMP_EQ, relationalOp);

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if (((exprLiteralType == PS_LITERAL_TIMESTAMP) || (exprLiteralType == PS_LITERAL_CHARSTR)) &&
        ((columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)))
    {
        if (exprLiteralType == PS_LITERAL_CHARSTR)
            GetOptions(literalVal);

        // Construct the filter string for input to Ryft.
        m_query += "(";
        ConstructDateComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, SE_COMP_EQ, relationalOp);
        m_query += " AND ";
        literalVal.Remove(0, 11);
        ConstructTimeComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, SE_COMP_EQ, relationalOp);
        m_query += ")";

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if (((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || 
                (exprLiteralType == PS_LITERAL_DECIMAL) || (exprLiteralType == PS_LITERAL_CHARSTR)) &&
        (typeSpecial == TYPE_NUMBER))
    {
        if (exprLiteralType == PS_LITERAL_CHARSTR)
            GetOptions(literalVal);

        if (negate)
            literalVal = "-" + rExpr->GetAsLiteral()->GetLiteralValue();

        // Construct the filter string for input to Ryft.
        ConstructNumberComparisonFilter(columnName, formatSpecial, literalVal, SE_COMP_EQ, relationalOp);

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if (((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || 
                (exprLiteralType == PS_LITERAL_DECIMAL) || (exprLiteralType == PS_LITERAL_CHARSTR)) &&
        (typeSpecial == TYPE_CURRENCY))
    {
        if (exprLiteralType == PS_LITERAL_CHARSTR)
            GetOptions(literalVal);
    
        if (negate)
            literalVal = "-" + rExpr->GetAsLiteral()->GetLiteralValue();

        // Construct the filter string for input to Ryft.
        ConstructCurrencyComparisonFilter(columnName, formatSpecial, literalVal, SE_COMP_EQ, relationalOp);

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if (((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) ||
        (exprLiteralType == PS_LITERAL_DECIMAL) || (exprLiteralType == PS_LITERAL_CHARSTR)))
    {
        if (negate)
            literalVal = "--" + rExpr->GetAsLiteral()->GetLiteralValue();

        // Construct the filter string for input to Ryft.
        ConstructStringComparisonFilter(columnName, columnSqlType, literalVal, relationalOp);

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownSimpleComparison(
    DSIExtColumnRef& in_leftExpr,
    LiteralValue in_rightExpr,
    SEComparisonType in_compOp)
{
    assert(in_rightExpr.first);
    assert(!in_leftExpr.m_table.IsNull());

    SE_CHK_INVALID_ARG(in_leftExpr.m_table.Get() != m_table.Get());

    // Get information about the column in the table that the filter is applied to.
    IColumns* tableColumns = in_leftExpr.m_table->GetSelectColumns();
    assert(tableColumns);
    IColumn *lColumn = tableColumns->GetColumn(in_leftExpr.m_colIndex);
    SqlTypeMetadata* columnMetadata = lColumn->GetMetadata();
    simba_int16 columnSqlType = columnMetadata->GetSqlType();

    // Get the column name.
    simba_wstring columnName;
    lColumn->GetLabel(columnName);

    // ryftprim doesn't like the trailing .[] which can happen in a JSON array that contains unnamed elements
    simba_wstring trailingArray = L".[]";
    simba_int32 loc = columnName.Find(L".[]", columnName.GetLength() - trailingArray.GetLength());
    if (loc != SIMBA_NPOS)
        columnName.Remove(loc, trailingArray.GetLength());

    // Get the literal type of RHS of comparison expression.
    PSLiteralType exprLiteralType = in_rightExpr.first->GetLiteralType();

    // Get the literal and prepend it with '-' if it's been negated.
    simba_wstring literalVal = in_rightExpr.first->GetLiteralValue();

    unsigned typeSpecial = TYPE_NONE;
    string formatSpecial;
    m_table->GetTypeFormatSpecifier(in_leftExpr.m_colIndex, &typeSpecial, formatSpecial);

    if (m_table->IsPCAPDatabase() && columnName != L"payload") {

        if (((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE)) && m_table->HasResultThinner(columnName)) {

            // Contruct the filter string based upon built in filters
            if (in_compOp == SE_COMP_EQ) {
                ConstructPCAPThinner(columnName, literalVal, FILTER_EQ);
            }
            else
                ConstructPCAPThinner(columnName, literalVal, FILTER_NE);

            m_isPassedDown = true;
            return true;
        }
        else if ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) ||
                 (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
                 (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)) {

            GetOptions(literalVal);

            // Construct the filter string for input to Ryft.
            ConstructPCAPComparisonFilter(columnName, literalVal, in_compOp);

            // Setting passdown flag so the filter result set is returned.
            m_isPassedDown = true;
            return true;
        }
    }
    else if(((exprLiteralType == PS_LITERAL_DATE) || (exprLiteralType == PS_LITERAL_CHARSTR)) && 
        ((columnSqlType == SQL_TYPE_DATE) || (columnSqlType == SQL_DATE) ||
            (columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) || 
         (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
         (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)))
    {
        if (exprLiteralType == PS_LITERAL_CHARSTR)
            GetOptions(literalVal);

        // Construct the filter string for input to Ryft.
        ConstructDateComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, in_compOp, "CONTAINS");

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if(((exprLiteralType == PS_LITERAL_TIME) || (exprLiteralType == PS_LITERAL_CHARSTR)) && 
        ((columnSqlType == SQL_TYPE_TIME) || (columnSqlType == SQL_TIME) || 
            (columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) || 
         (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
         (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)))
    {
        if (exprLiteralType == PS_LITERAL_CHARSTR)
            GetOptions(literalVal);

        ConstructTimeComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, in_compOp, "CONTAINS");

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if(((exprLiteralType == PS_LITERAL_TIMESTAMP) || (exprLiteralType = PS_LITERAL_CHARSTR)) && 
        ((columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) || 
         (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
         (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)))
    {
        if (exprLiteralType == PS_LITERAL_CHARSTR)
            GetOptions(literalVal);

        // capture the dateLiteral before it is altered.
        simba_wstring dateLiteral = literalVal;

        // Construct the filter string for input to Ryft.
        if((in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
           (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)) {
             m_query += "(";
        }
        m_query += "(";
        ConstructDateComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, in_compOp, "CONTAINS");
        m_query += " AND ";
        literalVal.Remove(0,11);
        ConstructTimeComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, in_compOp, "CONTAINS");
        m_query += " )";
        if((in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
           (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)) {
            m_query += " OR ";
            TDWTimestamp tdwTS(dateLiteral);
            if((in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE)) {
                tdwTS = tdwTS + 1;
            }
            else 
                tdwTS = tdwTS - 1;
            dateLiteral = tdwTS.ToString();
            if((in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE)) {
                ConstructDateComparisonFilter(columnName, typeSpecial, formatSpecial, dateLiteral, SE_COMP_GE, "CONTAINS");
            }
            else
                ConstructDateComparisonFilter(columnName, typeSpecial, formatSpecial, dateLiteral, SE_COMP_LE, "CONTAINS");
            m_query += ")";
        }

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if(((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || 
                (exprLiteralType == PS_LITERAL_DECIMAL) || (exprLiteralType == PS_LITERAL_CHARSTR)) &&
        (typeSpecial == TYPE_NUMBER) && 
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) || 
         (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
         (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)))
    {
        if(exprLiteralType == PS_LITERAL_CHARSTR)
            GetOptions(literalVal);

        if (in_rightExpr.second)
            literalVal = "-" + in_rightExpr.first->GetLiteralValue();

        // Construct the filter string for input to Ryft.
        ConstructNumberComparisonFilter(columnName, formatSpecial, literalVal, in_compOp, "CONTAINS");

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if(((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || 
                (exprLiteralType == PS_LITERAL_DECIMAL) || (exprLiteralType == PS_LITERAL_CHARSTR)) &&
        (typeSpecial == TYPE_CURRENCY) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) || 
         (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
         (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)))
    {
        if (exprLiteralType == PS_LITERAL_CHARSTR)
            GetOptions(literalVal);

        if (in_rightExpr.second) 
            literalVal = "-" + in_rightExpr.first->GetLiteralValue();

        // Construct the filter string for input to Ryft.
        ConstructCurrencyComparisonFilter(columnName, formatSpecial, literalVal, in_compOp, "CONTAINS");

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if(((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || 
                (exprLiteralType == PS_LITERAL_DECIMAL) || (exprLiteralType == PS_LITERAL_CHARSTR)) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE)))
    {
        if (in_rightExpr.second) 
            literalVal = "--" + in_rightExpr.first->GetLiteralValue();

        // Construct the filter string for input to Ryft.
        if(in_compOp == SE_COMP_NE) {
            ConstructStringComparisonFilter(columnName, columnSqlType, literalVal, "NOT_EQUALS");
        }
        else
            ConstructStringComparisonFilter(columnName, columnSqlType, literalVal, "EQUALS");

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownSimpleComparison(
    DSIExtColumnRef& in_leftExpr,
    DSIExtColumnRef& in_rightExpr,
    SEComparisonType in_compOp)
{
    // Do not support passdown comparison operations where the left and right
    // operands are column references.
    UNUSED(in_leftExpr);
    UNUSED(in_rightExpr);
    UNUSED(in_compOp);

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownSimpleInPredicate(
    DSIExtColumnRef& in_column, 
    LiteralVector& in_literals)
{
    assert(!in_column.m_table.IsNull());

    SE_CHK_INVALID_ARG(in_column.m_table.Get() != m_table.Get());

    // Get information about the column in the table that the filter is applied to.
    IColumns* tableColumns = in_column.m_table->GetSelectColumns();
    assert(tableColumns);
    IColumn *lColumn = tableColumns->GetColumn(in_column.m_colIndex);
    SqlTypeMetadata* columnMetadata = lColumn->GetMetadata();
    simba_int16 columnSqlType = columnMetadata->GetSqlType();

    // Get the column name.
    simba_wstring columnName;
    lColumn->GetLabel(columnName);
    
    // ryftprim doesn't like the trailing .[] which can happen in a JSON array that contains unnamed elements
    simba_wstring trailingArray = L".[]";
    simba_int32 loc = columnName.Find(L".[]", columnName.GetLength() - trailingArray.GetLength());
    if(loc != SIMBA_NPOS)
        columnName.Remove(loc, trailingArray.GetLength());

    int literalIdx = 0;
    m_query += "(";

    LiteralVector::iterator literalItr;
    for(literalItr = in_literals.begin(); literalItr != in_literals.end(); literalItr++) {
        char *ptok;
        char *column = NULL;
        char *table = NULL;
        simba_wstring literalVal = literalItr->first->GetLiteralValue();
        string literalAsString = literalVal.GetAsPlatformString();
        char * literal_first = new char[literalAsString.length()+1];
        char * literal_last = NULL;
        strcpy(literal_first, literalAsString.c_str());

        // search literal for wildcard
        if(ptok = strchr(literal_first, '%')) {
            *ptok++ = '\0';
            literal_last = strchr(ptok, '%');
            if(literal_last)
                *literal_last++ = '\0';
            table = ptok;
            if(column = strchr(ptok, '.')) 
                *column++ = '\0';
        }

        string lformatSpecial;
        unsigned ltypeSpecial = TYPE_NONE;
        m_table->GetTypeFormatSpecifier(in_column.m_colIndex, &ltypeSpecial, lformatSpecial);

        simba_wstring relationalOp;
        if (m_negate)
            relationalOp = "NOT_";
        relationalOp += "CONTAINS";

        if(table) {
            string tableAsString = table;
            IQueryResult *result = m_ryft1->OpenTable(tableAsString);
            RyftOne_Columns columns = m_ryft1->GetColumns(tableAsString);
            int colIdx;
            RyftOne_Columns::iterator colItr;
            for(colIdx = 0, colItr = columns.begin(); colItr != columns.end(); colItr++, colIdx++) {
                if(!colItr->m_colName.compare(column))
                    break;
            }
            if(colItr == columns.end())
                continue;

            string rformatSpecial;
            unsigned rtypeSpecial = TYPE_NONE;
            result->GetTypeFormatSpecifier(colIdx, &rtypeSpecial, rformatSpecial);

            string subquery;
            bool bFetch = result->FetchFirst();
            for (; bFetch; bFetch = result->FetchNext()) {
                if (literalIdx++)
                    if (m_negate) {
                        m_query += " AND ";
                    }
                    else
                        m_query += " OR ";

                char dateLiteral[11];
                char timeLiteral[11];
                if(((colItr->m_dataType == SQL_TYPE_DATE) || (colItr->m_dataType == SQL_DATE)) &&
                    ((columnSqlType == SQL_TYPE_DATE) || (columnSqlType == SQL_DATE))) 
                {
                    struct tm tmDate = result->GetDateValue(colIdx);
                    snprintf(dateLiteral, sizeof(dateLiteral), "%04d-%02d-%02d", tmDate.tm_year, tmDate.tm_mon, tmDate.tm_mday);
                    literalVal.SetFromUTF8(dateLiteral);
                    ConstructDateComparisonFilter(columnName, ltypeSpecial, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);
                }
                else if(((colItr->m_dataType == SQL_TYPE_TIME) || (colItr->m_dataType == SQL_TIME)) &&
                    ((columnSqlType == SQL_TYPE_TIME) || (columnSqlType == SQL_TIME))) 
                {
                    struct tm tmTime = result->GetTimeValue(colIdx);
                    snprintf(dateLiteral, sizeof(timeLiteral), "%04d:%02d:%02d", tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec);
                    literalVal.SetFromUTF8(dateLiteral);
                    ConstructTimeComparisonFilter(columnName, ltypeSpecial, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);
                }
                else if(((colItr->m_dataType == SQL_TYPE_TIMESTAMP) || (colItr->m_dataType == SQL_TIMESTAMP)) &&
                    ((columnSqlType == SQL_TYPE_TIMESTAMP) || (columnSqlType == SQL_TIMESTAMP))) 
                {
                    m_query += "(";
                    struct tm tmDatetime = result->GetDateTimeValue(colIdx);

                    snprintf(dateLiteral, sizeof(dateLiteral), "%04d-%02d-%02d", tmDatetime.tm_year, tmDatetime.tm_mon, tmDatetime.tm_mday);
                    literalVal.SetFromUTF8(dateLiteral);
                    ConstructDateComparisonFilter(columnName, ltypeSpecial, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);

                    m_query += " AND ";

                    snprintf(dateLiteral, sizeof(timeLiteral), "%04d:%02d:%02d", tmDatetime.tm_hour, tmDatetime.tm_min, tmDatetime.tm_sec);
                    literalVal.SetFromUTF8(dateLiteral);
                    ConstructTimeComparisonFilter(columnName, ltypeSpecial, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);

                    m_query += ")";
                }
                else if((ltypeSpecial == TYPE_NUMBER) && (rtypeSpecial == TYPE_NUMBER)) 
                {
                    literalVal.SetFromUTF8(result->GetStringValue(colIdx));
                    ConstructNumberComparisonFilter(columnName, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);
                }
                else if((ltypeSpecial == TYPE_CURRENCY) && (rtypeSpecial == TYPE_CURRENCY)) 
                {
                    literalVal.SetFromUTF8(result->GetStringValue(colIdx));
                    ConstructCurrencyComparisonFilter(columnName, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);
                }
                else 
                {
                    subquery = literal_first;
                    subquery += result->GetStringValue(colIdx);
                    subquery += literal_last;
                    literalVal.SetFromUTF8(subquery);

                    // Construct the filter string for input to Ryft.
                    ConstructStringComparisonFilter(columnName, columnSqlType, literalVal, relationalOp);
                }
            }
            delete result;
            delete literal_first;
        }
        else {
            if (literalIdx++)
                if (m_negate) {
                    m_query += " AND ";
                }
                else
                    m_query += " OR ";

            // Get the literal type of RHS of comparison expression.
            PSLiteralType exprLiteralType = literalItr->first->GetLiteralType();

            if((exprLiteralType == PS_LITERAL_DATE) && 
                ((columnSqlType == SQL_TYPE_DATE) || (columnSqlType == SQL_DATE) ||
                    (columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)))
            {
                ConstructDateComparisonFilter(columnName, ltypeSpecial, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);
            }
            else if((exprLiteralType == PS_LITERAL_TIME) && 
                ((columnSqlType == SQL_TYPE_TIME) || (columnSqlType == SQL_TIME) || 
                    (columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)))
            {
                ConstructTimeComparisonFilter(columnName, ltypeSpecial, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);
            }
            else if((exprLiteralType == PS_LITERAL_TIMESTAMP) && 
                ((columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)))
            {
                m_query += "( ";
                ConstructDateComparisonFilter(columnName, ltypeSpecial, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);
                m_query += " AND ";
                literalVal.Remove(0,11);
                ConstructTimeComparisonFilter(columnName, ltypeSpecial, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);
                m_query += " )";
            }
            else if(((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || (exprLiteralType == PS_LITERAL_DECIMAL)) &&
                (ltypeSpecial == TYPE_NUMBER))
            {
                if (literalItr->second) 
                    literalVal = "-" + literalItr->first->GetLiteralValue();

                // Construct the filter string for input to Ryft.
                ConstructNumberComparisonFilter(columnName, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);
            }
            else if(((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || (exprLiteralType == PS_LITERAL_DECIMAL)) &&
                (ltypeSpecial == TYPE_CURRENCY))
            {
                if (literalItr->second) 
                    literalVal = "-" + literalItr->first->GetLiteralValue();

                // Construct the filter string for input to Ryft.
                ConstructCurrencyComparisonFilter(columnName, lformatSpecial, literalVal, SE_COMP_EQ, relationalOp);
            }
            else
                ConstructStringComparisonFilter(columnName, columnSqlType, literalVal, relationalOp);
        }
    }

    m_query += ")";

    // Setting passdown flag so the filter result set is returned.
    m_isPassedDown = true;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownSimpleNullPredicate(
    DSIExtColumnRef& in_column, 
    bool in_isNull)
{
    // not supported
    UNUSED(in_column);
    UNUSED(in_isNull);

    return false;
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
static const wchar_t whitespace_chars[] = L" \f\n\r\t\v";

#define iswhitespace(c) \
    ((c) && wcschr(whitespace_chars, (c)))

void R1FilterHandler::ConstructStringComparisonFilter(
    simba_wstring in_columnName,
    simba_int16 in_columnSqlType,
    const simba_wstring& in_exprValue,
    simba_wstring in_RelationalOp)
{
    wstring out_filter = L"\"";
    const wchar_t *pfilter;
    const wchar_t *pOpt = NULL;
    size_t idx1, idx2;
    bool regex = false;
    bool pipCmd = false;
	bool structuredUsesRaw = false;
    int hamming = 0;
    int edit = 0;
    int width = 0;
    bool line = false;
    int limit = 0;
    char distance[10];
    char surrounding[10];
    bool case_sensitive = 1;

    wstring in_filter = in_exprValue.GetAsPlatformWString();
    for(pfilter = in_filter.c_str(); *pfilter; pfilter++) {
        switch(*pfilter) {
        case L'-':
            pOpt = pfilter;
            pfilter++;
			if (*pfilter == L'a' || *pfilter == L'A')
			{
    			if(m_table->IsStructuredType()) {
				// MNB should check for JSON or CSV
					structuredUsesRaw = true;
				}
				pfilter++;
			}
            switch(*pfilter) {
            case L'r':
            case L'R': {
                wstring inside = pfilter;
                idx1 = inside.find_first_of(L"(");
                idx2 = inside.find_last_of(L")");
                if(idx1 != string::npos && idx2 != string::npos) {
                    wstring search_string = inside.substr(idx1+1, idx2-idx1-1);
                    out_filter += search_string;
                    pfilter += idx2;
                    regex = true;
                }
                break;
                }
			case L'p':
			case L'P': {
			    // PIP command has no expression and list of options
				wstring inside = pfilter;
				idx1 = inside.find_first_of(L"(");
				idx2 = inside.find_last_of(L")");
				if(idx1 != string::npos && idx2 != string::npos) {
					wstring search_string = inside.substr(idx1+1, idx2-idx1-1);
					// ignore leading quote by overwriting out_filter 
					out_filter = search_string;
					pfilter += idx2;
					pipCmd = true;
				}
				break;
				}
            case L'h': // format -hddd(term)
            case L'H': {
                wstring num_hamming;
                for(pfilter++; *pfilter && (*pfilter >= L'0' && *pfilter <= L'9'); pfilter++)
                    num_hamming += *pfilter;
                swscanf(num_hamming.c_str(), L"%d", &hamming);
                edit = 0;

                wstring inside = pfilter;
                idx1 = inside.find_first_of(L"(");
                idx2 = inside.find_last_of(L")");
                if(idx1 == string::npos || idx2 == string::npos) 
                    R1THROWGEN1("InvalidSyntax", pOpt);

                wstring search_string = inside.substr(idx1+1, idx2-idx1-1);
                const wchar_t *psearch = search_string.c_str();
                for( ; *psearch; psearch++) {
                    switch(*psearch) {
                    case L'%':
                    case L'_':
                        out_filter += L"\"?\"";
                        break;
                    default:
                        out_filter += *psearch;
                        break;
                    }
                }
                pfilter += idx2;
                break;
                }
            case L'e':
            case L'E': {
                wstring num_edit;
                for(pfilter++; *pfilter && (*pfilter >= L'0' && *pfilter <= L'9'); pfilter++)
                    num_edit += *pfilter;
                swscanf(num_edit.c_str(), L"%d", &edit);
                hamming = 0;

                wstring inside = pfilter;
                idx1 = inside.find_first_of(L"(");
                idx2 = inside.find_last_of(L")");
                if(idx1 == string::npos || idx2 == string::npos) 
                    R1THROWGEN1("InvalidSyntax", pOpt);
                
                wstring search_string = inside.substr(idx1+1, idx2-idx1-1);
                const wchar_t *psearch = search_string.c_str();
                for( ; *psearch; psearch++) {
                    switch(*psearch) {
                    case L'%':
                    case L'_':
                        out_filter += L"\"?\"";
                        break;
                    default:
                        out_filter += *psearch;
                        break;
                    }
                }
                pfilter += idx2;
                break;
                }
            case L'w':
            case L'W': {
                wstring num_width;
                for(idx1 = 1; pfilter[idx1] && (pfilter[idx1] >= L'0' && pfilter[idx1] <= L'9'); idx1++)
                    num_width += pfilter[idx1];
                swscanf(num_width.c_str(), L"%d", &width);
                pfilter += idx1-1;
                break;
                }
            case L'n':
            case L'N': 
                line = true;
                line = true;
                break;
            case L'l':
            case L'L': {
                wstring num_limit;
                for (idx1 = 1; pfilter[idx1] && (pfilter[idx1] >= L'0' && pfilter[idx1] <= L'9'); idx1++)
                    num_limit += pfilter[idx1];
                swscanf(num_limit.c_str(), L"%d", &limit);
                pfilter += idx1 - 1;
                break;
                }
            case L'i':
            case L'I':
                case_sensitive = false;
                break;
            case L'-':
                out_filter += L"-";
                break;
            default: 
                R1THROWGEN1("InvalidOption", pOpt);
            }
            // skip whitespace after the filter
            for ( ; iswhitespace(*(pfilter+1)); pfilter++) ;
            break;
        case L'%':
        case L'_':
            out_filter += L"\"?\"";
            break;
        default:
            out_filter += *pfilter;
            break;
        }
    }
	if (!pipCmd) {
    	out_filter += L"\"";
	}

    if(m_table->IsStructuredType()) {
	    if(pipCmd) {
			simba_wstring pipFormat;
			m_table->GetPIPFormat(pipFormat);
			if (pipFormat.Find("LAT_COORD", 0) != SIMBA_NPOS) {
        		m_query += "(RECORD";
			} else {
        		m_query += "(RECORD." + in_columnName;
			}
			out_filter = out_filter + L"," + pipFormat.GetAsPlatformWString();
		}
		else if (structuredUsesRaw) {
        	m_query += "(RAW_TEXT";
		}
		else {
        	m_query += "(RECORD." + in_columnName;
		}
    }
    else 
        m_query += "(RAW_TEXT";

    m_query += " ";
    m_query += in_RelationalOp;
    m_query += " ";

    if(regex) {
        m_query += "PCRE2(";
        m_query += out_filter.c_str();
    }
	else if(pipCmd) {
        m_query += "PIP(";
        m_query += out_filter.c_str();
	}
    else {
        if(edit) {
            m_query += "EDIT_DISTANCE(";
            sprintf(distance, "%d", edit);
        }
        else {
            m_query += "HAMMING(";
            sprintf(distance, "%d", hamming);
        }
        m_query += out_filter.c_str();
        m_query += ",DISTANCE=\"" + string(distance) + "\"";
    }
    if(!case_sensitive) 
        m_query += ",CASE=\"FALSE\"";
    if(width) {
        sprintf(surrounding, "%d", width);
        m_query += ",WIDTH=\"" + string(surrounding) + "\"";
    }
    if(line || structuredUsesRaw)
        m_query += ",LINE=\"TRUE\"";

	if (m_table->IsStructuredType()) {
		simba_wstring rawDelimiter;
		m_table->GetFieldDelimiter(rawDelimiter);

		if (!rawDelimiter.IsEmpty())
		{
			simba_wstring cleanedDelim = rawDelimiter.RegexReplace(" ", "", NULL);
			m_query += ",FIELD_DELIMITER=\"" + cleanedDelim + "\"";
		}
	}


    m_query += "))";
    if(limit > m_limit)
        m_limit = limit;
}

void R1FilterHandler::GetOptions(simba_wstring& in_exprValue)
{
    wstring out_filter;
    const wchar_t *pfilter;
    const wchar_t *pOpt = NULL;
    size_t idx1;
    int limit = 0;

    wstring in_filter = in_exprValue.GetAsPlatformWString();
    for (pfilter = in_filter.c_str(); *pfilter; pfilter++) {
        switch (*pfilter) {
        case L'-':
            pOpt = pfilter;
            pfilter++;
            switch (*pfilter) {
            case L'l':
            case L'L': {
                wstring num_limit;
                for (idx1 = 1; pfilter[idx1] && (pfilter[idx1] >= L'0' && pfilter[idx1] <= L'9'); idx1++)
                    num_limit += pfilter[idx1];
                swscanf(num_limit.c_str(), L"%d", &limit);
                pfilter += idx1 - 1;
                break;
            }
            case L'-':
                out_filter += L"-";
                break;
            default:
                R1THROWGEN1("InvalidOption", pOpt);
            }
            // skip whitespace after the filter
            for (; iswhitespace(*(pfilter + 1)); pfilter++);
            break;
        default:
            out_filter += *pfilter;
            break;
        }
    }

    if (limit > m_limit)
        m_limit = limit;

    in_exprValue = out_filter.c_str();
}

#include <algorithm>
void R1FilterHandler::ConstructDateComparisonFilter(
    simba_wstring in_columnName,
    unsigned in_typeCustom,
    string& in_formatCustom,
    const simba_wstring& in_exprValue,
    SEComparisonType in_compOp,
    simba_wstring in_RelationalOp)
{
    int year = 1970, mon = 1, day = 1;
    string in_dateLiteral = in_exprValue.GetAsPlatformString();
    if(in_dateLiteral.length())
        sscanf(in_dateLiteral.c_str(), "%04d-%02d-%02d", &year, &mon, &day);

    if(m_table->IsStructuredType()) {
        m_query += "(RECORD." + in_columnName;
    }
    else 
        m_query += "(RAW_TEXT";

    m_query += " ";
    m_query += in_RelationalOp;
    m_query += " DATE(";

    unsigned typeCustom = in_typeCustom;
    string formatSpec = in_formatCustom.substr(0,14);
    switch(in_typeCustom) {
    case DATETIME_YYYYMMDD_12MMSS:
    case DATETIME_YYYYMMDD_24MMSS:
        typeCustom = DATE_YYYYMMDD;
        break;
    case DATETIME_YYMMDD_12MMSS:
    case DATETIME_YYMMDD_24MMSS:
        typeCustom = DATE_YYMMDD;
        break;
    case DATETIME_DDMMYYYY_12MMSS:
    case DATETIME_DDMMYYYY_24MMSS:
        typeCustom = DATE_DDMMYYYY;
        break;
    case DATETIME_DDMMYY_12MMSS:
    case DATETIME_DDMMYY_24MMSS:
        typeCustom = DATE_DDMMYY;
        break;
    case DATETIME_MMDDYYYY_12MMSS:
    case DATETIME_MMDDYYYY_24MMSS:
        typeCustom = DATE_MMDDYYYY;
        break;
    case DATETIME_MMDDYY_12MMSS:
    case DATETIME_MMDDYY_24MMSS:
        typeCustom = DATE_MMDDYY;
        break;
    }

    char outdate[11];
    string outfmt;
    switch(typeCustom) {
    case DATE_YYMMDD:
    case DATE_YYYYMMDD:
        sprintf(outdate, formatSpec.c_str(), 1111, 22, 33);
        outfmt = outdate;
        std::replace(outfmt.begin(), outfmt.end(), '1', 'Y');
        std::replace(outfmt.begin(), outfmt.end(), '2', 'M');
        std::replace(outfmt.begin(), outfmt.end(), '3', 'D');
        sprintf(outdate, formatSpec.c_str(), year, mon, day);
        break;
    case DATE_DDMMYY:
    case DATE_DDMMYYYY:
        sprintf(outdate, formatSpec.c_str(), 11, 22, 3333);
        outfmt = outdate;
        std::replace(outfmt.begin(), outfmt.end(), '1', 'D');
        std::replace(outfmt.begin(), outfmt.end(), '2', 'M');
        std::replace(outfmt.begin(), outfmt.end(), '3', 'Y');
        sprintf(outdate, formatSpec.c_str(), day, mon, year);
        break;
    case DATE_MMDDYY:
    case DATE_MMDDYYYY:
        sprintf(outdate, formatSpec.c_str(), 11, 22, 3333);
        outfmt = outdate;
        std::replace(outfmt.begin(), outfmt.end(), '1', 'M');
        std::replace(outfmt.begin(), outfmt.end(), '2', 'D');
        std::replace(outfmt.begin(), outfmt.end(), '3', 'Y');
        sprintf(outdate, formatSpec.c_str(), mon, day, year);
        break;
    }

    m_query += outfmt.c_str();

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_query += " = ";
        break;
    case SE_COMP_NE:
        m_query += " != ";
        break;
    case SE_COMP_GT:
        m_query += " > ";
        break;
    case SE_COMP_GE:
        m_query += " >= ";
        break;
    case SE_COMP_LT:
        m_query += " < ";
        break;
    case SE_COMP_LE:
        m_query += " <= ";
        break;
    }

    m_query += outdate;

	if (m_table->IsStructuredType()) {
		simba_wstring rawDelimiter;
		m_table->GetFieldDelimiter(rawDelimiter);
		if (!rawDelimiter.IsEmpty())
		{
			simba_wstring cleanedDelim = rawDelimiter.RegexReplace(" ", "", NULL);
			m_query += ",FIELD_DELIMITER=\"" + cleanedDelim + "\"";
		}
	}

    m_query += "))";
}

void R1FilterHandler::ConstructTimeComparisonFilter(
    simba_wstring in_columnName,
    unsigned in_typeCustom,
    string& in_formatCustom,
    const simba_wstring& in_exprValue,
    SEComparisonType in_compOp,
    simba_wstring in_RelationalOp)
{
    int hour = 0, min = 0, sec = 0;
    string in_dateLiteral = in_exprValue.GetAsPlatformString();
    if(in_dateLiteral.length())
        sscanf(in_dateLiteral.c_str(), "%04d:%02d:%02d", &hour, &min, &sec);

    unsigned typeCustom = in_typeCustom;
    string formatSpec = in_formatCustom.substr(0,14);
    switch(in_typeCustom) {
    case DATETIME_YYYYMMDD_24MMSS:
    case DATETIME_DDMMYYYY_24MMSS:
    case DATETIME_MMDDYYYY_24MMSS:
    case DATETIME_YYMMDD_24MMSS:
    case DATETIME_DDMMYY_24MMSS:
    case DATETIME_MMDDYY_24MMSS:
        in_typeCustom = TIME_24MMSS;
        formatSpec = in_formatCustom.substr(15,14);
        break;
    case DATETIME_YYYYMMDD_12MMSS:
    case DATETIME_YYMMDD_12MMSS:
    case DATETIME_DDMMYYYY_12MMSS:
    case DATETIME_DDMMYY_12MMSS:
    case DATETIME_MMDDYYYY_12MMSS:
    case DATETIME_MMDDYY_12MMSS:
        in_typeCustom = TIME_12MMSS;
        formatSpec = in_formatCustom.substr(15,14);
        break;
    }

    char outtime[9];
    string outfmt;
    sprintf(outtime, formatSpec.c_str(), 11, 22, 33);
    outfmt = outtime;
    std::replace(outfmt.begin(), outfmt.end(), '1', 'H');
    std::replace(outfmt.begin(), outfmt.end(), '2', 'M');
    std::replace(outfmt.begin(), outfmt.end(), '3', 'S');

    switch(in_typeCustom) {
    case TIME_24MMSS:
        sprintf(outtime, formatSpec.c_str(), hour, min, sec);
        break;
    case TIME_12MMSS: {
        int twelve_hour = hour % 12;
        sprintf(outtime, formatSpec.c_str(), twelve_hour ? twelve_hour : 12, min, sec);
        m_query += "(";
        break;
        }
    }

    if(m_table->IsStructuredType()) {
        m_query += "(RECORD." + in_columnName;
    }
    else 
        m_query += "(RAW_TEXT";

    m_query += " ";
    m_query += in_RelationalOp;
    m_query += " TIME(";
    m_query += outfmt.c_str();

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_query += " = ";
        break;
    case SE_COMP_NE:
        m_query += " != ";
        break;
    case SE_COMP_GT:
        m_query += " > ";
        break;
    case SE_COMP_GE:
        m_query += " >= ";
        break;
    case SE_COMP_LT:
        m_query += " < ";
        break;
    case SE_COMP_LE:
        m_query += " <= ";
        break;
    }

    m_query += outtime;
	if (m_table->IsStructuredType()) {
		simba_wstring rawDelimiter;
		m_table->GetFieldDelimiter(rawDelimiter);
		if (!rawDelimiter.IsEmpty())
		{
			simba_wstring cleanedDelim = rawDelimiter.RegexReplace(" ", "", NULL);
			m_query += ",FIELD_DELIMITER=\"" + cleanedDelim + "\"";
		}
	}
    m_query += "))";

    if(in_typeCustom == TIME_12MMSS) {
        m_query += " AND (RECORD." + in_columnName + " CONTAINS \"";
        m_query += (hour < 12) ? "AM" : "PM";
        m_query += "\"";

		if (m_table->IsStructuredType()) {
			simba_wstring rawDelimiter;
			m_table->GetFieldDelimiter(rawDelimiter);
			if (!rawDelimiter.IsEmpty())
			{
				simba_wstring cleanedDelim = rawDelimiter.RegexReplace(" ", "", NULL);
				m_query += ",FIELD_DELIMITER=\"" + cleanedDelim + "\"";
			}
		}

    	m_query += "))";
    }
}

void R1FilterHandler::ConstructNumberComparisonFilter(
    simba_wstring in_columnName,
    string& in_formatCustom,
    const simba_wstring& in_exprValue,
    SEComparisonType in_compOp,
    simba_wstring in_RelationalOp)
{
    string numLiteral = in_exprValue.GetAsPlatformString();

    if(m_table->IsStructuredType()) {
        m_query += "(RECORD." + in_columnName;
    }
    else 
        m_query += "(RAW_TEXT";

    m_query += " ";
    m_query += in_RelationalOp;
    m_query += " NUMBER(NUM";

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_query += " = ";
        break;
    case SE_COMP_NE:
        m_query += " != ";
        break;
    case SE_COMP_GT:
        m_query += " > ";
        break;
    case SE_COMP_GE:
        m_query += " >= ";
        break;
    case SE_COMP_LT:
        m_query += " < ";
        break;
    case SE_COMP_LE:
        m_query += " <= ";
        break;
    }

    m_query += "\"" + numLiteral + "\", ";
    m_query += "\"" + in_formatCustom.substr(0,1) + "\", ";
    m_query += "\"" + in_formatCustom.substr(1,1) + "\"";

	if (m_table->IsStructuredType()) {
		simba_wstring rawDelimiter;
		m_table->GetFieldDelimiter(rawDelimiter);
		if (!rawDelimiter.IsEmpty())
		{
			simba_wstring cleanedDelim = rawDelimiter.RegexReplace(" ", "", NULL);
			m_query += ",FIELD_DELIMITER=\"" + cleanedDelim + "\"";
		}
	}

    m_query += "))";
}

void R1FilterHandler::ConstructCurrencyComparisonFilter(
    simba_wstring in_columnName,
    string& in_formatCustom,
    const simba_wstring& in_exprValue,
    SEComparisonType in_compOp,
    simba_wstring in_RelationalOp)
{
    string numLiteral = in_exprValue.GetAsPlatformString();

    if(m_table->IsStructuredType()) {
        m_query += "(RECORD." + in_columnName;
    }
    else 
        m_query += "(RAW_TEXT";

    m_query += " ";
    m_query += in_RelationalOp;
    m_query += " CURRENCY(CUR";

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_query += " = ";
        break;
    case SE_COMP_NE:
        m_query += " != ";
        break;
    case SE_COMP_GT:
        m_query += " > ";
        break;
    case SE_COMP_GE:
        m_query += " >= ";
        break;
    case SE_COMP_LT:
        m_query += " < ";
        break;
    case SE_COMP_LE:
        m_query += " <= ";
        break;
    }

    m_query += "\"" + in_formatCustom.substr(0,1) + numLiteral + "\", ";
    m_query += "\"" + in_formatCustom.substr(0,1) + "\", ";
    m_query += "\"" + in_formatCustom.substr(1,1) + "\", ";
    m_query += "\"" + in_formatCustom.substr(2,1) + "\"";

	if (m_table->IsStructuredType()) {
		simba_wstring rawDelimiter;
		m_table->GetFieldDelimiter(rawDelimiter);
		if (!rawDelimiter.IsEmpty())
		{
			simba_wstring cleanedDelim = rawDelimiter.RegexReplace(" ", "", NULL);
			m_query += ",FIELD_DELIMITER=\"" + cleanedDelim + "\"";
		}
	}

    m_query += "))";
}

void R1FilterHandler::ConstructPCAPComparisonFilter(
    simba_wstring in_columnName,
    const simba_wstring& in_exprValue,
    Simba::SQLEngine::SEComparisonType in_compOp) 
{
    string searchLiteral = in_exprValue.GetAsPlatformString();

    m_query += "(" + in_columnName;

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_query += " == ";
        break;
    case SE_COMP_NE:
        m_query += " != ";
        break;
    case SE_COMP_GT:
        m_query += " > ";
        break;
    case SE_COMP_GE:
        m_query += " >= ";
        break;
    case SE_COMP_LT:
        m_query += " < ";
        break;
    case SE_COMP_LE:
        m_query += " <= ";
        break;
    }

    m_query += searchLiteral;
    m_query += ")";
}

void R1FilterHandler::ConstructPCAPThinner(
    simba_wstring in_columnName,
    const simba_wstring& in_exprValue,
    int in_compOp)
{
    string searchLiteral = in_exprValue.GetAsPlatformString();
    string constraint;
    char query[1024];
    constraint = m_table->GetResultThinnerQuery(in_columnName, in_compOp);

    ColFilter colFilter;
    colFilter.compOp = in_compOp;
    colFilter.searchLiteral = searchLiteral;
    colFilter.colName = in_columnName.GetAsPlatformString();
    m_colFilters.push_back(colFilter);

    snprintf(query, sizeof(query), constraint.c_str(), searchLiteral.c_str());
    m_query += query;
}
