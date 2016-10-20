// =================================================================================================
///  @file R1Util.h
///
///  Implements the static utilities
///
///  Copyright (C) 2016 Ryft Systems, Inc.
// =================================================================================================
#ifndef _R1UTIL_H_
#define _R1UTIL_H_

#include <string>
using namespace std;

#define TYPE_NONE                   0

#define DATE_YYYYMMDD               1
#define DATE_YYMMDD                 2
#define DATE_DDMMYYYY               3
#define DATE_DDMMYY                 4
#define DATE_MMDDYYYY               5
#define DATE_MMDDYY                 6

#define TIME_12MMSS                 7
#define TIME_24MMSS                 8

#define DATETIME_YYYYMMDD_12MMSS    9
#define DATETIME_YYYYMMDD_24MMSS    10
#define DATETIME_YYMMDD_12MMSS      11
#define DATETIME_YYMMDD_24MMSS      12
#define DATETIME_DDMMYYYY_12MMSS    13
#define DATETIME_DDMMYYYY_24MMSS    14
#define DATETIME_DDMMYY_12MMSS      15
#define DATETIME_DDMMYY_24MMSS      16
#define DATETIME_MMDDYYYY_12MMSS    17
#define DATETIME_MMDDYYYY_24MMSS    18
#define DATETIME_MMDDYY_12MMSS      19
#define DATETIME_MMDDYY_24MMSS      20

#define TYPE_NUMBER                 21
#define TYPE_CURRENCY               22

using namespace std;

class RyftOne_Util {
public:
    static void RyftToSqlType(string& in_typeName, unsigned *out_sqlType, unsigned *out_charCols, 
        unsigned *out_bufLength, string& out_formatCustom, unsigned *out_typeCustom);
    static string SqlToRyftType(unsigned in_type, unsigned in_bufLen);

    static string UrlEncode(const string& value);
};
#endif