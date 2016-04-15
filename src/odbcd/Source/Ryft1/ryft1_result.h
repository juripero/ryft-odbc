#pragma once

#include <libryftone.h>

#include <string.h>
#include <string>
#include <vector>
#include <deque>
using namespace std;

#include "libmeta.h"
#include "sqlite3.h"

#define XML_STATIC
#include "expat.h"
// The IElement interface exposes useful behavior
// for objects that can be parsed from XML by a parser:
class IElement
{
    friend class IParser;

public:
    inline                      IElement() : m_bDeleteOnPop( false ) {}
    inline                      IElement( const std::string &sElName ) : m_sElName( sElName ), m_bDeleteOnPop( false ) {}

    enum NodeAction
    {
        ProcessNode = 0,
        PushNode,
        SkipNode,
    };

    // This function is called automatically by the parser
    // for whichever element is on top of the parse stack
    // when a new nested start tag is encountered.
    // Return ProcessNode to continue parsing.
    // Return PushNode and set ppElement if
    // parsing should continue with a new IElement object on the
    // top of the parse stack.
    // Return SkipNode to ignore the node and its children.
    virtual NodeAction          StartRow( ) { return ProcessNode; }
    virtual NodeAction          AddElement(std::string sName, const char **psAttribs, IElement **ppElement) { return ProcessNode; }
    
    // This function is called automatically by the parser
    // for whichever element is on top of the parse stack
    // when enclosed text inside a pair of tags is being processed:
    virtual void                AddText( std::string sText ) {}
    
    // This function is called when an element is about to be popped
    // of the the parse stack
    virtual void                ExitElement() {}
    virtual void                ExitRow() {}

    inline const std::string   &ElementName() const { return m_sElName; }
    inline int                  ElementLevel() const { return m_iLevel; }

protected:

    inline                      IElement( const std::string &sElName, int iLevel, bool bDeleteOnPop ) : m_sElName( sElName ), m_iLevel( iLevel ), m_bDeleteOnPop( bDeleteOnPop ) {}

    inline void                 SetLevel( int iLevel ) { m_iLevel = iLevel; }
    inline bool                 DeleteOnPop() const { return m_bDeleteOnPop; }

    std::string                 m_sElName;          // Inherited classes MUST initialize this
                                                    // as it is used to determine when to pop
                                                    // elements from the parser stack!!

    int                         m_iLevel;           // Set by the IParser during XML Parsing

    bool                        m_bDeleteOnPop;     // If true, IParser objects will delete this
                                                    // object upon removing it from the parse stack
                                                    // during XML parsing
};

// Element objects will be stored on a stack by parsers:
typedef std::deque<IElement *> ElementStack;

class IParser : public IElement
{
public:
    inline          IParser() : m_bParsed( false ), m_iLevel( 0 ) {}
    inline          IParser( const std::string &wsElName ) : IElement( wsElName ), m_bParsed( false ), m_iLevel( 0 ), m_inNode(false) {}
  
#define PARSER_WINDOW_SIZE      0x10000

    // The auto-magic parse function.  This processes relationships and then
    // parses the main XML file associated with the object:
    bool Parse(int fd, unsigned long dwSize, std::string delim, bool no_top)
    {
        // Open parse path XML file:
        m_delim = delim;

        XML_Parser parser = XML_ParserCreate( NULL );
        if( parser )
        {
            // We will pop the first element, ourselves:
            m_ParseStack.push_back( this );

            // Set handlers:
            XML_SetElementHandler( parser, IParser::StartHandler, IParser::EndHandler );
            XML_SetCharacterDataHandler( parser, IParser::TextHandler );
            XML_SetUserData( parser, (void *)this );
            
            // Parse:
            try 
            {
                unsigned long dwCurr, dwRemain;
                int readThisLoop;
                bool bLast;
                if(no_top)
                    XML_Parse( parser, "<xml_start>", strlen("<xml_start>"), false);

                char * pbBuffer = (char *)malloc(PARSER_WINDOW_SIZE);

                for( dwCurr = 0; dwCurr < dwSize; dwCurr += PARSER_WINDOW_SIZE )
                {
                    dwRemain = dwSize - dwCurr;
                    bLast = dwRemain < PARSER_WINDOW_SIZE;
                    readThisLoop = read(fd, pbBuffer, bLast ? dwRemain : PARSER_WINDOW_SIZE);
                    if(readThisLoop == -1) {
                        free(pbBuffer);
                        XML_ParserFree( parser );
                        return false;
                    }
                    if( XML_STATUS_OK != XML_Parse( parser, (const char *)( pbBuffer ), readThisLoop, bLast ) )
                        break;
                    else if( bLast )
                        m_bParsed = true;
                }
            }
            catch(...)
            {
                m_bParsed = false;
            }
            XML_ParserFree( parser );
        }

        // NOTE: No need to pop us, as the close tag handler will pop us automatically.
        return m_bParsed;
    }

    inline bool     IsParsed() const { return m_bParsed; }

protected:
    // Internal implementations:
    static void XMLCALL StartHandler( void *pData, const char * sName, const char **pwsAttribs )
    {
        IElement *pPush = NULL;
        IParser *pThis = (IParser *)pData;
        ++pThis->m_iLevel;

        if(pThis->m_delim == sName) {
            pThis->m_ParseStack.back()->StartRow( );
        }
        else  {
            switch( pThis->m_ParseStack.back()->AddElement( sName, pwsAttribs, &pPush ) )
            {
            default:
            case ProcessNode:
                break;
            case PushNode:
                pPush->SetLevel( pThis->m_iLevel );
                pThis->m_ParseStack.push_back( pPush );
                break;
            case SkipNode:
                pThis->m_ParseStack.push_back( new IElement( sName, pThis->m_iLevel, true ) );
                break;
            }
        }
        pThis->m_inNode = true;
    }

    static void XMLCALL TextHandler( void * pData, const char * sText, int len )
    {
        IParser *pThis = (IParser *)pData;
        if(pThis->m_inNode) {
            IElement *pElement = pThis->m_ParseStack.back();
            std::string sString;
            sString.append( sText, len );
            pElement->AddText( sString );
        }
    }

    static void XMLCALL EndHandler( void * pData, const char * sName )
    {
        IParser *pThis = (IParser *)pData;
        IElement *pElement = pThis->m_ParseStack.back();
        if(pThis->m_delim == sName) {
            pThis->m_ParseStack.back()->ExitRow( );
        }
        else if( !strcmp( pElement->ElementName().c_str(), sName ) && pThis->m_iLevel == pElement->ElementLevel() ) 
        {
            pElement->ExitElement();
            pThis->m_ParseStack.pop_back();
            if( pElement->DeleteOnPop() )
                delete pElement;
        }
        --pThis->m_iLevel;
        pThis->m_inNode = false;
    }

    std::string     m_delim;
    bool            m_bParsed;
    bool            m_inNode;
    int             m_iLevel;
    ElementStack    m_ParseStack;
};

__inline void CopyEscapedIdentifier(string& dest, string& src)
{
    string::iterator itr; 
    char pchar = 0;

    try {
        // remove special characters from identifiers
        for(itr = src.begin(); itr != src.end(); ++itr) {
            switch (*itr) {
            case '\'':
            case '\"':
            case '\\':
            case '[':
            case ']':
            case '.':
            case '!':
            case '`':
                if(pchar != '_') 
                    dest += '_';
                pchar = '_';
                break;
            default:
                dest += *itr;
                pchar = *itr;
            }
        }
    }
    catch(exception &e) {
        ;
    }
}

#define SC_NUMERIC  0
#define SC_TEXT     1
#define SC_REAL     2
struct resultCol {
    int colNum;
    int storageClass;
    int storageBytes;
    int charCols;
    union {
        char *text;
        long long integer;
        double real;
    } colResult;

};
typedef vector<resultCol> resultCols;

enum {
    typeNative = 0,
    typeInserted
};

typedef struct _Cursor {
    int __type;
    int __colIdx;
    resultCols __row;
} Cursor;

typedef struct _RyftOne_Column {
    unsigned m_ordinal;
    string m_tableName;
    string m_colTag;
    string m_colName;
    string m_description;
    unsigned m_dataType;
    string m_typeName;
    unsigned m_charCols;
    unsigned m_bufLength;
    unsigned m_dtType;
    string m_formatSpec;
} RyftOne_Column;
typedef vector<RyftOne_Column> RyftOne_Columns;

class RyftOne_Result : public IParser {
public:
    RyftOne_Result( );
   ~RyftOne_Result( );

    void open(string& in_name, vector<__catalog_entry__>::iterator in_catentry);
    void appendFilter(string in_filter, int in_hamming, int in_edit, bool in_caseSensitive);
    
    void prepareAppend();
    void finishUpdate();
    void flush();

    bool fetchFirst();
    bool fetchNext();

    const char * getStringValue(int colIdx);
    int getIntValue(int colIdx);
    long long getInt64Value(int colIdx);
    double getDoubleValue(int colIdx);
    struct tm getDateValue(int colIdx);
    struct tm getTimeValue(int colIdx);
    struct tm getDateTimeValue(int colIdx);

    void putStringValue(int colIdx, string colValue);

protected:
    // XML Parse
    virtual NodeAction StartRow( );
    virtual NodeAction AddElement( std::string sName, const char **psAttribs, IElement **ppElement );
    virtual void AddText( std::string sText );
    virtual void ExitRow();

private:
    RyftOne_Columns __cols;
    vector<string> __glob;

    int __hamming;
    int __edit;
    bool __caseSensitive;

    string __query;
    bool __queryFinished;

    rol_data_set_t __loaded;
    string __path;
    string __name;
    string __extension;
    string __delim;
    vector<string> __rdfTags;

    sqlite3 *__sqlite;
    sqlite3_stmt *__stmt;
    int __transMax;

    Cursor __cursor;

    void __loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr);
    bool __execute();

    int __getColumnNum(string colTag);
    bool __writeRow(FILE *f);

    bool __initTable(string &table);
    bool __storeToSqlite(string& table, const char *file, bool no_top);
    bool __loadFromSqlite(string &table);

    void __date(const char *dateStr, int colIdx, struct tm *date);
    void __time(const char *timeStr, int colIdx, struct tm *time);
    void __datetime(const char *datetimeStr, int colIdx, struct tm*time);
};