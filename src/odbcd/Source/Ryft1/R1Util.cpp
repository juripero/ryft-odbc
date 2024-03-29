// =================================================================================================
///  @file R1Util.cpp
///
///  Implements the static utilities
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================
#include <string.h>
#include <iomanip>
#include <sstream>

#include "R1Util.h"
#include "R1Catalog.h"
#include "TypeDefines.h"

void RyftOne_Util::RyftToSqlType(string& in_typeName, unsigned *out_sqlType, unsigned *out_charCols, 
                                 unsigned *out_bufLength, string& out_formatCustom, unsigned *out_typeCustom)
{
    unsigned charCols = 0;
    unsigned bufLength = 0;
    char subitizer;
    char decimal;
    char currency;
    char numType[20];
    int  cnt;

    *out_typeCustom = TYPE_NONE;

    char format[80];
    char formatSpec[80];
    if(!strcasecmp(in_typeName.c_str(), "integer")) {
        *out_sqlType = SQL_INTEGER;
        charCols = 10;
        bufLength = 4;
    }
    else if(!strcasecmp(in_typeName.c_str(), "bigint")) {
        *out_sqlType = SQL_BIGINT;
        charCols = 20;
        bufLength = 20;
    }
    else if(!strcasecmp(in_typeName.c_str(), "double")) {
        *out_sqlType = SQL_DOUBLE;
        charCols = 15;
        bufLength = 8;
    }
    else if(!strncasecmp(in_typeName.c_str(), "datetime", strlen("datetime"))) {
        *out_sqlType = SQL_TYPE_TIMESTAMP;
        const char *paren = strchr(in_typeName.c_str(), '(');
        strcpy(format, "MM-DD-YYYY-24:MM:SS");
        if(paren) {
            strcpy(format, paren+1);
            format[strlen(format)-1] = '\0';
        }
        if(format[0] == 'Y' && format[2] == 'Y') { // YYYY/MM/DD-HH:MM:SS
            if(format[11] == '1') {
                *out_typeCustom = DATETIME_YYYYMMDD_12MMSS;
                sprintf(formatSpec, "%%04d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d %%s", format[4], format[7], format[10], format[13], format[16]);
                charCols = 22;
            }
            else {
                *out_typeCustom = DATETIME_YYYYMMDD_24MMSS;
                sprintf(formatSpec, "%%04d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d", format[4], format[7], format[10], format[13], format[16]);
                charCols = 19;
            }
        }
        else if(format[0] == 'Y') { // YY/MM/DD-HH:MM:SS
            if(format[11] == '1') {
                *out_typeCustom = DATETIME_YYMMDD_12MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d %%s", format[2], format[5], format[8], format[11], format[14]);
                charCols = 22;
            }
            else {
                *out_typeCustom = DATETIME_YYMMDD_24MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d", format[2], format[5], format[8], format[11], format[14]);
                charCols = 19;
            }
        }
        else if (format[0] == 'D' && format[8] == 'Y') { // DD/MM/YYYY-HH:MM:SS
            if(format[11] == '1') {
                *out_typeCustom = DATETIME_DDMMYYYY_12MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%04d%c%%02d%c%%02d%c%%02d %%s", format[2], format[5], format[10], format[13], format[16]);
                charCols = 22;
            }
            else {
                *out_typeCustom = DATETIME_DDMMYYYY_24MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%04d%c%%02d%c%%02d%c%%02d", format[2], format[5], format[10], format[13], format[16]);
                charCols = 19;
            }
        }
        else if (format[0] == 'D') { // DD/MM/YY-HH:MM:SS
            if(format[11] == '1') {
                *out_typeCustom = DATETIME_DDMMYY_12MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d %%s", format[2], format[5], format[8], format[11], format[14]);
                charCols = 22;
            }
            else {
                *out_typeCustom = DATETIME_DDMMYY_24MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d", format[2], format[5], format[8], format[11], format[14]);
                charCols = 19;
            }
        }
        else if (format[0] == 'M' && format[8] == 'Y') { // MM/DD/YYYY-HH:MM:SS
            if(format[11] == '1') {
                *out_typeCustom = DATETIME_MMDDYYYY_12MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%04d%c%%02d%c%%02d%c%%02d %%s", format[2], format[5], format[10], format[13], format[16]);
                charCols = 22;
            }
            else {
                *out_typeCustom = DATETIME_MMDDYYYY_24MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%04d%c%%02d%c%%02d%c%%02d", format[2], format[5], format[10], format[13], format[16]);
                charCols = 19;
            }
        }
        else if (format[0] == 'M') { // MM/DD/YY-HH:MM:SS
            if(format[11] == '1') {
                *out_typeCustom = DATETIME_MMDDYY_12MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d %%s", format[2], format[5], format[8], format[11], format[14]);
                charCols = 22;
            }
            else {
                *out_typeCustom = DATETIME_MMDDYY_24MMSS;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d%c%%02d%c%%02d%c%%02d", format[2], format[5], format[8], format[11], format[14]);
                charCols = 19;
            }
        }
        out_formatCustom = formatSpec;
        bufLength = 16;
    }
    else if(!strncasecmp(in_typeName.c_str(), "date", strlen("date"))) {
        *out_sqlType = SQL_TYPE_DATE;
        const char *paren = strchr(in_typeName.c_str(), '(');
        strcpy(format, "YYYY/MM/DD");
        if(paren) {
            sscanf(paren, "(%s)", format);
            format[strlen(format)-1] = '\0';
        }
        if(strlen(format) == 10) {
            if(format[0] == 'Y') { // YYYY/MM/DD
                *out_typeCustom = DATE_YYYYMMDD;
                sprintf(formatSpec, "%%04d%c%%02d%c%%02d", format[4], format[7]);
            }
            else if (format[0] == 'D') { // DD/MM/YYYY
                *out_typeCustom = DATE_DDMMYYYY;
                sprintf(formatSpec, "%%02d%c%%02d%c%%04d", format[2], format[5]);
            }
            else { // MM/DD/YYYY
                *out_typeCustom = DATE_MMDDYYYY;
                sprintf(formatSpec, "%%02d%c%%02d%c%%04d", format[2], format[5]);
            }
        }
        else if(strlen(format) == 8) {
            if(format[0] == 'Y') { // YY/MM/DD
                *out_typeCustom = DATE_YYMMDD;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d", format[2], format[5]);
            }
            else if(format[0] == 'D') { // DD/MM/YY
                *out_typeCustom = DATE_DDMMYY;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d", format[2], format[5]);
            }
            else { // MM/DD/YY
                *out_typeCustom = DATE_MMDDYY;
                sprintf(formatSpec, "%%02d%c%%02d%c%%02d", format[2], format[5]);
            }
        }
        out_formatCustom = formatSpec;
        charCols = 10;
        bufLength = 6;
    }
    else if(!strncasecmp(in_typeName.c_str(), "time", strlen("time"))) {
        *out_sqlType = SQL_TYPE_TIME;
        const char *paren = strchr(in_typeName.c_str(), '(');
        strcpy(format, "24:MM:SS");
        if(paren) {
            sscanf(paren, "(%s)", format);
            format[strlen(format)-1] = '\0';
        }
        if(!strcmp(format, "24:MM:SS")) {
            *out_typeCustom = TIME_24MMSS;
            strcpy(formatSpec, "%02d:%02d:02d");
            charCols = 8;
        }
        else {
            *out_typeCustom = TIME_12MMSS;
            strcpy(formatSpec, "%02d:%02d:02d %s");
            charCols = 11;
        }
        out_formatCustom = formatSpec;
        bufLength = 6;
    }
    else if(!strncasecmp(in_typeName.c_str(), "varchar", strlen("varchar"))) {
        *out_sqlType = SQL_VARCHAR;
        const char * paren = strchr(in_typeName.c_str(), '(');
        if(paren)
            sscanf(paren, "(%d)", &charCols);
        bufLength = charCols;
    }
    else if(!strncasecmp(in_typeName.c_str(), "list", strlen("list"))) {
        *out_sqlType = SQL_VARCHAR;
        const char * paren = strchr(in_typeName.c_str(), '(');
        if(paren)
            sscanf(paren, "(%d)", &charCols);
        bufLength = charCols;
    }
    else if(!strncasecmp(in_typeName.c_str(), "number", strlen("number"))) {
        *out_sqlType = SQL_VARCHAR;
        *out_typeCustom = TYPE_NUMBER;
        const char * paren = strchr(in_typeName.c_str(), '(');
        if(paren) {
            cnt = sscanf(paren, "(%d,%c,%c,%s)", &charCols, &subitizer, &decimal, &numType[0]);
	        switch (cnt) {
            case 3:
                strcpy(numType,"double");
                // Intentional fall thru
                // Note: 0, 1, 2 were previous errors, so ignored for these changes
	        case 4:
                if ( !strncasecmp(numType, "double", strlen("double")) || 
                     !strncasecmp(numType, "float", strlen("float"))) { 
                    *out_sqlType = SQL_DOUBLE;
                }
                else if (!strncasecmp(numType, "integer", strlen("integer"))) {
                    *out_sqlType = SQL_INTEGER;
                }
                else if (!strncasecmp(numType, "bigint", strlen("bigint"))) {
                    *out_sqlType = SQL_BIGINT;
                }
                break;
            }
        }
        bufLength = charCols;
        sprintf(formatSpec, "%c%c", subitizer, decimal);
        out_formatCustom = formatSpec;
    }
    else if(!strncasecmp(in_typeName.c_str(), "currency", strlen("currency"))) {
        *out_sqlType = SQL_VARCHAR;
        *out_typeCustom = TYPE_CURRENCY;
        const char * paren = strchr(in_typeName.c_str(), '(');
        if(paren) {
            cnt = sscanf(paren, "(%d,%c,%c,%c)", &charCols, &currency, &subitizer, &decimal);
//			// If a numeric type is added, try to convert currency to number on output
//			if (cnt > 4) {
//               if ( !strncasecmp(numType, "double", strlen("double")) || 
//                     !strncasecmp(numType, "float", strlen("float"))) { 
//                    *out_sqlType = SQL_DOUBLE;
//                }
//                else if (!strncasecmp(numType, "integer", strlen("integer"))) {
//                    *out_sqlType = SQL_INTEGER;
//                }
//                else if (!strncasecmp(numType, "bigint", strlen("bigint"))) {
//                    *out_sqlType = SQL_BIGINT;
//                }
//			}	
        }	
        bufLength = charCols;
        sprintf(formatSpec, "%c%c%c", currency, subitizer, decimal);
        out_formatCustom = formatSpec;
    }
    else if(!strncasecmp(in_typeName.c_str(), "metavar", strlen("metavar"))) {
        const char *paren = strchr(in_typeName.c_str(), '(');
        format[0] = '\0';
        if(paren) {
            strcpy(format, paren+1);
            format[strlen(format)-1] = '\0';
        }
        if (!strcasecmp(format, "file")) {
            *out_sqlType = SQL_VARCHAR;
            *out_typeCustom = TYPE_META_FILE;
            charCols = FILENAME_MAX;
            bufLength = FILENAME_MAX;
        }
        else if(!strcasecmp(format, "offset")) {
            *out_sqlType = SQL_BIGINT;
            *out_typeCustom = TYPE_META_OFFSET;
            charCols = 20;
            bufLength = 20;
        }
        else if(!strcasecmp(format, "length")) {
            *out_sqlType = SQL_BIGINT;
            *out_typeCustom = TYPE_META_LENGTH;
            charCols = 20;
            bufLength = 20;
        }
    }
    *out_charCols = charCols;
    *out_bufLength = bufLength;
}

string RyftOne_Util::SqlToRyftType(unsigned in_type, unsigned in_charLen)
{
    switch(in_type) {
    case SQL_INTEGER:
        return "INTEGER";
    case SQL_BIGINT:
        return "BIGINT";
    case SQL_DOUBLE:
        return "DOUBLE";
    case SQL_DATE:
        return "DATE";
    case SQL_TIME:
        return "TIME";
    case SQL_TIMESTAMP:
        return "DATETIME";
    default: {
        char length[16];
        string out_typeName = "VARCHAR";
        snprintf(length, sizeof(length), "(%d)", in_charLen);
        out_typeName += length;
        return out_typeName;
        }
    }
}

string RyftOne_Util::UrlEncode(const string &value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    const char *pstr = value.c_str();
    for ( ; *pstr; ++pstr) {
        string::value_type c = *pstr;

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '(' || c == ')') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << '%' << setw(2) << int((unsigned char) c);
    }
    return escaped.str();
}
