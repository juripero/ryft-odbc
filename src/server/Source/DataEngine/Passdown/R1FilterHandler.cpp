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
    m_negate(false)
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
    return SharedPtr<DSIExtResultSet>(new R1FilterResult(m_table, m_filter));
}

// Protected =======================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
bool R1FilterHandler::PassdownAnd(AEAnd *in_node)
{
    AEBooleanExpr *lExpr = (AEBooleanExpr *)in_node->GetChild(0);
    AEBooleanExpr *rExpr = (AEBooleanExpr *)in_node->GetChild(1);
    simba_wstring pushFilter = m_filter;
    m_filter.Clear();
    m_filter += "( ";
    if(DSIExtAbstractBooleanExprHandler::Passdown(lExpr)) {
        m_filter += " AND ";
        if(DSIExtAbstractBooleanExprHandler::Passdown(rExpr)) {
            m_filter += " )";
            m_filter = pushFilter + m_filter;
            m_isPassedDown = true;
            return true;
        }
    }
    m_filter = pushFilter;
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
    simba_wstring pushFilter = m_filter;
    m_filter.Clear();
    m_filter += "( ";
    if(DSIExtAbstractBooleanExprHandler::Passdown(lExpr)) {
        m_filter += " OR ";
        if(DSIExtAbstractBooleanExprHandler::Passdown(rExpr)) {
            m_filter += " )";
            m_filter = pushFilter + m_filter;
            m_isPassedDown = true;
            return true;
        }
    }
    m_filter = pushFilter;
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

    // Get the column name.
    simba_wstring columnName;
    lColumn->GetLabel(columnName);

    simba_wstring literalVal = in_rightExpr.first->GetLiteralValue();

    unsigned typeSpecial = TYPE_NONE;
    string formatSpecial;
    m_table->GetTypeFormatSpecifier(in_leftExpr.m_colIndex, &typeSpecial, formatSpecial);

    // Supported literal types: Unsigned Integer, Decimal, Character String, and Date
    if((exprLiteralType == PS_LITERAL_DATE) && 
        ((columnSqlType == SQL_TYPE_DATE) || (columnSqlType == SQL_DATE) ||
            (columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) || 
         (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
         (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)))
    {
        // Construct the filter string for input to CodeBase.
        ConstructDateComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, in_compOp);

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if((exprLiteralType == PS_LITERAL_TIME) && 
        ((columnSqlType == SQL_TYPE_TIME) || (columnSqlType == SQL_TIME) || 
            (columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) || 
         (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
         (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)))
    {
        ConstructTimeComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, in_compOp);

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if((exprLiteralType == PS_LITERAL_TIMESTAMP) && 
        ((columnSqlType == SQL_TIMESTAMP) || (columnSqlType == SQL_TYPE_TIMESTAMP)) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) || 
         (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
         (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)))
    {
        // Construct the filter string for input to CodeBase.
        m_filter += "( ";
        ConstructDateComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, in_compOp);
        m_filter += " AND ";
        literalVal.Remove(0,11);
        ConstructTimeComparisonFilter(columnName, typeSpecial, formatSpecial, literalVal, in_compOp);
        m_filter += " )";

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if(((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || (exprLiteralType == PS_LITERAL_DECIMAL)) &&
        (typeSpecial == TYPE_NUMBER) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) || 
         (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
         (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)))
    {
        // Construct the filter string for input to CodeBase.
        ConstructNumberComparisonFilter(columnName, formatSpecial, literalVal, in_compOp);

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if(((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || (exprLiteralType == PS_LITERAL_DECIMAL)) &&
        (typeSpecial == TYPE_CURRENCY) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE) || 
         (in_compOp == SE_COMP_GT) || (in_compOp == SE_COMP_GE) ||
         (in_compOp == SE_COMP_LT) || (in_compOp == SE_COMP_LE)))
    {
        // Construct the filter string for input to CodeBase.
        ConstructCurrencyComparisonFilter(columnName, formatSpecial, literalVal, in_compOp);

        // Setting passdown flag so the filter result set is returned.
        m_isPassedDown = true;
        return true;
    }
    else if(((exprLiteralType == PS_LITERAL_APPROXNUM) || (exprLiteralType == PS_LITERAL_USINT) || 
        (exprLiteralType == PS_LITERAL_DECIMAL) || (exprLiteralType == PS_LITERAL_CHARSTR)) &&
        ((in_compOp == SE_COMP_EQ) || (in_compOp == SE_COMP_NE)))
    {
        // Get the literal and prepend it with '-' if it's been negated.
        if (in_rightExpr.second) 
            literalVal = "-" + in_rightExpr.first->GetLiteralValue();

        // Construct the filter string for input to CodeBase.
        ConstructStringComparisonFilter(columnName, columnSqlType, in_rightExpr.first->GetMetadata()->GetSqlType(), 
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

void R1FilterHandler::ConstructStringComparisonFilter(
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
    int width = 0;
    char distance[10];
    char surrounding[10];
    string case_sensitive = "true";

    wstring in_filter = in_exprValue.GetAsPlatformWString();
    for(pfilter = in_filter.c_str(); *pfilter; pfilter++) {
        switch(*pfilter) {
        case L'-':
            pfilter++;
            switch(*pfilter) {
            case L'h':
            case L'H': {
                wstring num_hamming;
                for(pfilter++; *pfilter && (*pfilter >= L'0' && *pfilter <= L'9'); pfilter++)
                    num_hamming += *pfilter;
                swscanf(num_hamming.c_str(), L"%d", &hamming);
                edit = 0;

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
                for(pfilter++; *pfilter && (*pfilter >= L'0' && *pfilter <= L'9'); pfilter++)
                    num_edit += *pfilter;
                swscanf(num_edit.c_str(), L"%d", &edit);
                hamming = 0;

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
            case L'w':
            case L'W': {
                wstring num_width;
                for(idx1 = 1; pfilter[idx1] && (pfilter[idx1] >= L'0' && pfilter[idx1] <= L'9'); idx1++)
                    num_width += pfilter[idx1];
                swscanf(num_width.c_str(), L"%d", &width);
                pfilter += idx1-1;
                break;
                }
            case L'i':
            case L'I':
                case_sensitive = "false";
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

    if(m_table->IsStructuredType()) {
        m_filter += "( RECORD." + in_columnName;
    }
    else 
        m_filter += "( RAW_TEXT";

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_filter += " EQUALS ";
        break;
    case SE_COMP_NE:
        m_filter += " NOT_EQUALS ";
        break;
    }

    if(edit) {
        m_filter += "FEDS(";
        sprintf(distance, "%d", edit);
    }
    else {
        m_filter += "FHS(";
        sprintf(distance, "%d", hamming);
    }
    m_filter += out_filter.c_str();
    m_filter += ",CS=" + case_sensitive;
    m_filter += ",DIST=" + string(distance);
    sprintf(surrounding, "%d", width);
    m_filter += ",WIDTH=" + string(surrounding);
    m_filter += "))";
}

#include <algorithm>
void R1FilterHandler::ConstructDateComparisonFilter(
    simba_wstring in_columnName,
    unsigned in_typeCustom,
    string& in_formatCustom,
    const simba_wstring& in_exprValue,
    SEComparisonType in_compOp)
{
    int year = 1970, mon = 1, day = 1;
    string in_dateLiteral = in_exprValue.GetAsPlatformString();
    if(in_dateLiteral.length())
        sscanf(in_dateLiteral.c_str(), "%04d-%02d-%02d", &year, &mon, &day);

    if(m_table->IsStructuredType()) {
        m_filter += "( RECORD." + in_columnName;
    }
    else 
        m_filter += "( RAW_TEXT";

    m_filter += " CONTAINS DATE(";

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

    m_filter += outfmt.c_str();

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_filter += " = ";
        break;
    case SE_COMP_NE:
        m_filter += " != ";
        break;
    case SE_COMP_GT:
        m_filter += " > ";
        break;
    case SE_COMP_GE:
        m_filter += " >= ";
        break;
    case SE_COMP_LT:
        m_filter += " < ";
        break;
    case SE_COMP_LE:
        m_filter += " <= ";
        break;
    }

    m_filter += outdate;
    m_filter += ") )";
}

void R1FilterHandler::ConstructTimeComparisonFilter(
    simba_wstring in_columnName,
    unsigned in_typeCustom,
    string& in_formatCustom,
    const simba_wstring& in_exprValue,
    SEComparisonType in_compOp)
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
    case TIME_12MMSS:
        sprintf(outtime, formatSpec.c_str(), hour % 12, min, sec);
        m_filter += "( ";
        break;
    }

    if(m_table->IsStructuredType()) {
        m_filter += "( RECORD." + in_columnName;
    }
    else 
        m_filter += "( RAW_TEXT";

    m_filter += " CONTAINS TIME(";
    m_filter += outfmt.c_str();

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_filter += " = ";
        break;
    case SE_COMP_NE:
        m_filter += " != ";
        break;
    case SE_COMP_GT:
        m_filter += " > ";
        break;
    case SE_COMP_GE:
        m_filter += " >= ";
        break;
    case SE_COMP_LT:
        m_filter += " < ";
        break;
    case SE_COMP_LE:
        m_filter += " <= ";
        break;
    }

    m_filter += outtime;
    m_filter += ") )";

    if(in_typeCustom == TIME_12MMSS) {
        m_filter += " AND ( RECORD." + in_columnName + " CONTAINS \"";
        m_filter += (hour / 12) ? "PM" : "AM";
        m_filter += "\" ) )";
    }
}

void R1FilterHandler::ConstructNumberComparisonFilter(
    simba_wstring in_columnName,
    string& in_formatCustom,
    const simba_wstring& in_exprValue,
    SEComparisonType in_compOp)
{
    string numLiteral = in_exprValue.GetAsPlatformString();

    if(m_table->IsStructuredType()) {
        m_filter += "( RECORD." + in_columnName;
    }
    else 
        m_filter += "( RAW_TEXT";

    m_filter += " CONTAINS NUMBER(NUM";

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_filter += " = ";
        break;
    case SE_COMP_NE:
        m_filter += " != ";
        break;
    case SE_COMP_GT:
        m_filter += " > ";
        break;
    case SE_COMP_GE:
        m_filter += " >= ";
        break;
    case SE_COMP_LT:
        m_filter += " < ";
        break;
    case SE_COMP_LE:
        m_filter += " <= ";
        break;
    }

    m_filter += "\"" + numLiteral + "\", ";
    m_filter += "\"" + in_formatCustom.substr(0,1) + "\", ";
    m_filter += "\"" + in_formatCustom.substr(1,1) + "\"";
    m_filter += ") )";
}

void R1FilterHandler::ConstructCurrencyComparisonFilter(
    simba_wstring in_columnName,
    string& in_formatCustom,
    const simba_wstring& in_exprValue,
    SEComparisonType in_compOp)
{
    string numLiteral = in_exprValue.GetAsPlatformString();

    if(m_table->IsStructuredType()) {
        m_filter += "( RECORD." + in_columnName;
    }
    else 
        m_filter += "( RAW_TEXT";

    m_filter += " CONTAINS CURRENCY(CUR";

    // Determine the math symbol for comparison type.
    switch (in_compOp) {
    case SE_COMP_EQ:
        m_filter += " = ";
        break;
    case SE_COMP_NE:
        m_filter += " != ";
        break;
    case SE_COMP_GT:
        m_filter += " > ";
        break;
    case SE_COMP_GE:
        m_filter += " >= ";
        break;
    case SE_COMP_LT:
        m_filter += " < ";
        break;
    case SE_COMP_LE:
        m_filter += " <= ";
        break;
    }

    m_filter += "\"" + in_formatCustom.substr(0,1) + numLiteral + "\", ";
    m_filter += "\"" + in_formatCustom.substr(0,1) + "\", ";
    m_filter += "\"" + in_formatCustom.substr(1,1) + "\", ";
    m_filter += "\"" + in_formatCustom.substr(2,1) + "\"";
    m_filter += ") )";
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
    int width = 0;
    char distance[10];
    char surrounding[10];
    string case_sensitive = "true";

    wstring in_filter = in_exprValue.GetAsPlatformWString();
    for(pfilter = in_filter.c_str(); *pfilter; pfilter++) {
        switch(*pfilter) {
        case L'-':
            pfilter++;
            switch(*pfilter) {
            case L'h':
            case L'H': {
                wstring num_hamming;
                for(pfilter++; *pfilter && (*pfilter >= L'0' && *pfilter <= L'9'); pfilter++)
                    num_hamming += *pfilter;
                swscanf(num_hamming.c_str(), L"%d", &hamming);
                edit = 0;

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
                for(pfilter++; *pfilter && (*pfilter >= L'0' && *pfilter <= L'9'); pfilter++)
                    num_edit += *pfilter;
                swscanf(num_edit.c_str(), L"%d", &edit);
                hamming = 0;

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
            case L'w':
            case L'W': {
                wstring num_width;
                for(idx1 = 1; pfilter[idx1] && (pfilter[idx1] >= L'0' && pfilter[idx1] <= L'9'); idx1++)
                    num_width += pfilter[idx1];
                swscanf(num_width.c_str(), L"%d", &width);
                pfilter += idx1-1;
                break;
                }
            case L'i':
            case L'I':
                case_sensitive = "false";
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

    if(m_table->IsStructuredType()) {
        m_filter += "(RECORD." + in_columnName;
    }
    else
        m_filter += "(RAW_TEXT";

    if(m_negate) {
        m_filter += " NOT_CONTAINS ";
    }
    else
        m_filter += " CONTAINS ";

    if(edit) {
        m_filter += "FEDS(";
        sprintf(distance, "%d", edit);
    }
    else {
        m_filter += "FHS(";
        sprintf(distance, "%d", hamming);
    }
    m_filter += out_filter.c_str();
    m_filter += ",CS=" + case_sensitive;
    m_filter += ",DIST=" + string(distance);
    sprintf(surrounding, "%d", width);
    m_filter += ",WIDTH=" + string(surrounding);
    m_filter += "))";
}
