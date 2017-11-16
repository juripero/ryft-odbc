// =================================================================================================
///  @file csv.h
///
///  Handles CSV parsing functions
///
///  Copyright (C) 2017 Ryft Systems, Inc.
// =================================================================================================
#pragma once

#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

struct CSV_ParserStruct;
typedef struct CSV_ParserStruct *CSV_Parser;

enum CSV_Status {
    CSV_STATUS_ERROR = 0,
    CSV_STATUS_OK = 1,
    CSV_STATUS_SUSPENDED = 2
};

enum CSV_State {
    CSV_START_FILE = 0,
    CSV_START_LINE,
    CSV_START_VALUE,
    CSV_READ_VALUE
};

#ifndef CSVCALL
#if defined(_MSC_VER)
#define CSVCALL __cdecl
#elif defined(__GNUC__) && defined(__i386) && !defined(__INTEL_COMPILER)
#define CSVCALL __attribute__((cdecl))
#else
#define CSVCALL
#endif
#endif  /* not defined CSVCALL */

typedef void (CSVCALL *CSV_StartValueHandler) (void *userData, int valueIndex);

typedef void (CSVCALL *CSV_EndValueHandler) (void *userData, int valueIndex, bool endLine);

/* s is not 0 terminated. */
typedef void (CSVCALL *CSV_CharacterDataHandler) (void *userData, const char *s, int len);

struct CSV_ParserStruct {
    char                        __endLine;
    char                        __endValue;
    char                        __escape;
    CSV_StartValueHandler       __startHandler;
    CSV_EndValueHandler         __endHandler;
    CSV_CharacterDataHandler    __textHandler;
    void *                      __userData;
    enum CSV_State              __parserState;
    int                         __valueIndex;
    bool                        __inEscape;
    char                        __lastChar;
};

static CSV_Parser CSV_ParserCreate()
{
    CSV_Parser parser = NULL;
    parser = (CSV_Parser)malloc(sizeof(CSV_ParserStruct));
    memset(parser, 0, sizeof(CSV_ParserStruct));
    parser->__parserState = CSV_START_FILE;
    parser->__endValue = ',';
    parser->__endLine = '\n';
    parser->__escape = '"';
    return parser;
}

static void CSV_ParserFree(CSV_Parser parser)
{
    free(parser);
}

static void CSV_SetValueHandler(CSV_Parser parser, CSV_StartValueHandler startHandler, CSV_EndValueHandler endHandler)
{
    if(parser) {
        parser->__startHandler = startHandler;
        parser->__endHandler = endHandler;
    }
}

static void CSV_SetCharacterDataHandler(CSV_Parser parser, CSV_CharacterDataHandler textHandler)
{
    if(parser) {
        parser->__textHandler = textHandler;
    }
}

static void CSV_SetUserData(CSV_Parser parser, void * userData)
{
    if(parser) {
        parser->__userData = userData;
    }
}

static void CSV_SetDelimiter(CSV_Parser parser, const char endValue, const char endLine)
{
    if(parser) {
        parser->__endValue = endValue;
        parser->__endLine = endLine;
    }
}

static void CSV_SetEscape(CSV_Parser parser, const char escape)
{
    if(parser) {
        parser->__escape = escape;
    }
}
    
static enum CSV_Status CSV_Parse(CSV_Parser parser, const char *s, int len, int isFinal)
{
    char eov = parser->__endValue;
    char eol = parser->__endLine;
    char esc = parser->__escape;
    bool escaped = parser->__inEscape;
    CSV_State state = parser->__parserState;
    char last = parser->__lastChar;
    char c;
    int processed;
    const char *val;
    int vallen;

    for(processed = 0, c = *s; processed < len; processed++, last = c, c = *(s+processed)) {
        switch(state) {
        case CSV_START_FILE:
            state = CSV_START_LINE;
            // fallthrough
        case CSV_START_LINE:
            parser->__valueIndex = 0;
            state = CSV_START_VALUE;
            // fallthrough
        case CSV_START_VALUE:
            val = s+processed;
            vallen = 0;
            parser->__valueIndex++;
            parser->__startHandler(parser->__userData, parser->__valueIndex);
            state = CSV_READ_VALUE;
            // fallthrough
        case CSV_READ_VALUE:
            if(c == esc) {
                if(last == esc) {
                    // output the escape character
                    vallen++;
                    c = '\0';
                }
                escaped = !escaped;
                if(vallen) 
                    parser->__textHandler(parser->__userData, val, vallen);
                val = s+processed+1;
                vallen = 0;
            }
            else if(escaped) {
                // doesn't matter what next character is, we are in an escape
                vallen++;
            }
            else if(c == eov) {
                if(vallen) 
                    parser->__textHandler(parser->__userData, val, vallen);
                parser->__endHandler(parser->__userData, parser->__valueIndex, false);
                state = CSV_START_VALUE;
            }
            else if(c == eol) {
                if(vallen) 
                    parser->__textHandler(parser->__userData, val, vallen);
                parser->__endHandler(parser->__userData, parser->__valueIndex, true);
                state = CSV_START_LINE;
            }
            else
                vallen++;
        }
    }
    if(isFinal) {
        if(state == CSV_READ_VALUE) {
            parser->__endHandler(parser->__userData, parser->__valueIndex, true);
            state == CSV_START_LINE;
        }
    }
    parser->__parserState = state;
    parser->__inEscape = escaped;
    parser->__lastChar = last;
    return CSV_STATUS_OK;
}

#ifdef __cplusplus
}
#endif