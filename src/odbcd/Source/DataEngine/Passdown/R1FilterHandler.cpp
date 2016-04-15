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
    SharedPtr<R1Table> in_table) :
    m_table(in_table),
    m_isPassedDown(false),
    m_negate(false),
    m_hamming(0),
    m_edit(0),
    m_caseSensitive(true)
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
    return SharedPtr<DSIExtResultSet>(new R1FilterResult(m_table, m_filter, m_hamming, m_edit, m_caseSensitive));
}

// Protected =======================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownAnd(AEAnd *in_node)
{
    AEBooleanExpr *lExpr = (AEBooleanExpr *)in_node->GetChild(0);
    AEBooleanExpr *rExpr = (AEBooleanExpr *)in_node->GetChild(1);
    m_filter += "( ";
    if(DSIExtAbstractBooleanExprHandler::Passdown(lExpr)) {
        m_filter += " AND ";
        if(DSIExtAbstractBooleanExprHandler::Passdown(rExpr)) {
            m_filter += " )";
            m_isPassedDown = true;
            return true;
        }
    }
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
    m_filter += "( ";
    if(DSIExtAbstractBooleanExprHandler::Passdown(lExpr)) {
        m_filter += " OR ";
        if(DSIExtAbstractBooleanExprHandler::Passdown(rExpr)) {
            m_filter += " )";
            m_isPassedDown = true;
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownLikePredicate(AELikePredicate* in_node) 
{
    // Try and handle parameters here.
    AEValueExpr* lExpr = in_node->GetLeftOperand();
    AEValueExpr* rExpr = in_node->GetRightOperand();

    //  Only handle two cases: 
    //   - <column_reference> LIKE <literal>
    //   - <literal> <LIKE> <column_reference>
    if ((AE_NT_VX_COLUMN != lExpr->GetNodeType()) || 
        (AE_NT_VX_LITERAL != rExpr->GetNodeType())) {
        if ((AE_NT_VX_COLUMN == rExpr->GetNodeType()) &&
            (AE_NT_VX_LITERAL == lExpr->GetNodeType())) {
            // Swap the pointers, so the expressions are always in the form of
            // <column_reference> <compOp> <parameter>.
            std::swap(lExpr, rExpr);
        }
        else
            return false;
    }

    DSIExtColumnRef colRef;
    if(!GetTableColRef(lExpr, colRef))
        return false;

    // Get information about the column in the table that the filter is applied to.
    IColumns* tableColumns = colRef.m_table->GetSelectColumns();
    assert(tableColumns);
    IColumn *lColumn = tableColumns->GetColumn(colRef.m_colIndex);
    SqlTypeMetadata* columnMetadata = lColumn->GetMetadata();
    simba_int16 columnSqlType = columnMetadata->GetSqlType();

    // Get the literal type of RHS of comparison expression.
    simba_int16 paramSqlType = rExpr->GetMetadata()->GetSqlType();
    SqlDataTypeUtilities* dataTypeUtils = SqlDataTypeUtilitiesSingleton::GetInstance();
    if(dataTypeUtils->IsIntegerType(paramSqlType) || dataTypeUtils->IsExactNumericType(paramSqlType) ||
        dataTypeUtils->IsApproximateNumericType(paramSqlType) || dataTypeUtils->IsAnyCharacterType(paramSqlType)) {

        simba_wstring literalVal = rExpr->GetAsLiteral()->GetLiteralValue();
        if(literalVal.IsNull())
            return false;

        // Get the column name and comparison type.
        simba_wstring columnName;
        lColumn->GetLabel(columnName);

        // Construct the filter string for input to CodeBase.
        ConstructLikeFilter(columnName, columnSqlType, paramSqlType, literalVal);

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

    // Get the literal type of RHS of comparison expression.
    PSLiteralType exprLiteralType = in_rightExpr.first->GetLiteralType();
            
    // Supported literal types: Unsigned Integer, Decimal, Character String, and Date
    if(((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || 
        (exprLiteralType == PS_LITERAL_DECIMAL) || (exprLiteralType == PS_LITERAL_CHARSTR)) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE)))
    {
        // Get the column name.
        simba_wstring columnName;
        lColumn->GetLabel(columnName);

        // Get the literal and prepend it with '-' if it's been negated.
        simba_wstring literalVal;
        if (in_rightExpr.second) {
            literalVal = "-" + in_rightExpr.first->GetLiteralValue();
        }
        else
            literalVal = in_rightExpr.first->GetLiteralValue();

        // Construct the filter string for input to CodeBase.
        ConstructComparisonFilter(columnName, columnSqlType, in_rightExpr.first->GetMetadata()->GetSqlType(), 
            literalVal, in_compOp);

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
    // CodebaseDSII does not support passdown comparison operations where the left and right
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
    // not supported
    UNUSED(in_column);
    UNUSED(in_literals);

    return false;
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

void R1FilterHandler::ConstructComparisonFilter(
    simba_wstring in_columnName,
    simba_int16 in_columnSqlType,
    simba_int16 in_exprSqlType,
    const simba_wstring& in_exprValue,
    SEComparisonType in_compOp)
{
    wstring out_filter = L"\"";
    const wchar_t *pfilter;
    size_t idx1, idx2;
    int hamming = 0;
    int edit = 0;
    bool case_sensitive = true;

    wstring in_filter = in_exprValue.GetAsPlatformWString();
    for(pfilter = in_filter.c_str(); *pfilter; pfilter++) {
        switch(*pfilter) {
        case L'-':
            pfilter++;
            switch(*pfilter) {
            case L'h':
            case L'H': {
                wstring num_hamming;
                for(pfilter++; *pfilter && (*pfilter >= L'0' && *pfilter <= '9'); pfilter++)
                    num_hamming += *pfilter;
                swscanf(num_hamming.c_str(), L"%d", &hamming);

                wstring inside = pfilter;
                idx1 = inside.find_first_of(L"(");
                idx2 = inside.find_last_of(L")");
                if(idx1 != string::npos && idx2 != string::npos) {
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
                }
                break;
                }
            case L'e':
            case L'E': {
                wstring num_edit;
                for(pfilter++; *pfilter && (*pfilter >= L'0' && *pfilter <= '9'); pfilter++)
                    num_edit += *pfilter;
                swscanf(num_edit.c_str(), L"%d", &edit);

                wstring inside = pfilter;
                idx1 = inside.find_first_of(L"(");
                idx2 = inside.find_last_of(L")");
                if(idx1 != string::npos && idx2 != string::npos) {
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
                }
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
                // ignore any other switches
                break;
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
    out_filter += L"\"";

    m_filter += "( RECORD." + in_columnName;

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_filter += " EQUALS ";
        break;
    case SE_COMP_NE:
        m_filter += " NOT_EQUALS ";
        break;
    }

    m_filter += out_filter.c_str();
    m_filter += " )";
    m_hamming = hamming;
    m_edit = edit;
    m_caseSensitive = case_sensitive;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void R1FilterHandler::ConstructLikeFilter(
    simba_wstring in_columnName,
    simba_int16 in_columnSqlType,
    simba_int16 in_exprSqlType,
    const simba_wstring& in_exprValue)
{
    wstring out_filter = L"\"";
    const wchar_t *pfilter;
    size_t idx1, idx2;
    int hamming = 0;
    int edit = 0;
    bool case_sensitive = true;

    wstring in_filter = in_exprValue.GetAsPlatformWString();
    for(pfilter = in_filter.c_str(); *pfilter; pfilter++) {
        switch(*pfilter) {
        case L'-':
            pfilter++;
            switch(*pfilter) {
            case L'h':
            case L'H': {
                wstring num_hamming;
                for(pfilter++; *pfilter && (*pfilter >= L'0' && *pfilter <= '9'); pfilter++)
                    num_hamming += *pfilter;
                swscanf(num_hamming.c_str(), L"%d", &hamming);

                wstring inside = pfilter;
                idx1 = inside.find_first_of(L"(");
                idx2 = inside.find_last_of(L")");
                if(idx1 != string::npos && idx2 != string::npos) {
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
                }
                break;
                }
            case L'e':
            case L'E': {
                wstring num_edit;
                for(pfilter++; *pfilter && (*pfilter >= L'0' && *pfilter <= '9'); pfilter++)
                    num_edit += *pfilter;
                swscanf(num_edit.c_str(), L"%d", &edit);

                wstring inside = pfilter;
                idx1 = inside.find_first_of(L"(");
                idx2 = inside.find_last_of(L")");
                if(idx1 != string::npos && idx2 != string::npos) {
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
                }
                break;
                }
            case L'i':
            case L'I':
                case_sensitive = false;
                break;
            case '-':
                out_filter += L"-";
                break;
            default:
                // ignore any other switches
                break;
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
    out_filter += L"\"";

    m_filter += "( RECORD." + in_columnName;

    if(m_negate) {
        m_filter += " NOT_CONTAINS ";
    }
    else
        m_filter += " CONTAINS ";

    m_filter += out_filter.c_str();
    m_filter += " )";
    m_hamming = hamming;
    m_edit = edit;
    m_caseSensitive = case_sensitive;
}
