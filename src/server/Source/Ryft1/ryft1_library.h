#pragma once

#include <libryftone.h>

#include <string>
#include <vector>
#include <deque>
using namespace std;

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
    virtual NodeAction          AddRow( ) { return ProcessNode; }
    virtual NodeAction          AddElement(std::string sName, const char **psAttribs, IElement **ppElement) { return ProcessNode; }
    
    // This function is called automatically by the parser
    // for whichever element is on top of the parse stack
    // when enclosed text inside a pair of tags is being processed:
    virtual void                AddText( std::string sText ) {}
    
    // This function is called when an element is about to be popped
    // of the the parse stack
    virtual void                ExitElement() {}

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
  
#define PARSER_WINDOW_SIZE      0x8000

    // The auto-magic parse function.  This processes relationships and then
    // parses the main XML file associated with the object:
    bool Parse(char *pbBuffer, unsigned long dwSize, std::string delim)
    {
        // Don't re-parse:
        if( IsParsed() )
            return true;

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
                bool bLast;
                XML_Parse( parser, "<xml_start>", strlen("<xml_start>"), false);

                for( dwCurr = 0; dwCurr < dwSize; dwCurr += PARSER_WINDOW_SIZE )
                {
                    dwRemain = dwSize - dwCurr;
                    bLast = dwRemain < PARSER_WINDOW_SIZE;
                    if( XML_STATUS_OK != XML_Parse( parser, (const char *)( pbBuffer + dwCurr ),
                        bLast ? dwRemain : PARSER_WINDOW_SIZE, bLast ) )
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
            pThis->m_ParseStack.back()->AddRow( );
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
        if( !strcmp( pElement->ElementName().c_str(), sName ) && pThis->m_iLevel == pElement->ElementLevel() ) 
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

class __meta_config__ {
public:
    __meta_config__(string& in_table);

    class __meta_col__ {
    public:
        string rdf_name;
        string name;
        string type_def;
        string description;
    };

    class __meta_view__ {
    public:
        string id;
        string view_name;
        string restriction;
        string description;
    };

    string table_name;
    string table_remarks;
    string rdf_path;
    vector<__meta_col__> columns;
    vector<__meta_view__> views;
};

class __rdf_config__ {
public:
    __rdf_config__(string& in_path);

    class __rdf_tag__ {
    public:
        string name;
        string start_tag;
        string end_tag;
    };

    string file_glob;
    string record_start;
    string record_end;
    vector<__rdf_tag__> tags;
};

class __catalog_entry__ {
public:
    __catalog_entry__(string in_table);
    __meta_config__ meta_config;
    string path;
};

typedef struct _RyftOne_Table {
    string m_tableName;
    string m_description;
    string m_type;
} RyftOne_Table;
typedef vector<RyftOne_Table> RyftOne_Tables;

typedef struct _RyftOne_Column {
    unsigned m_ordinal;
    string m_tableName;
    string m_colTag;
    string m_colName;
    string m_description;
    unsigned m_dataType;
    string m_typeName;
    unsigned m_bufferLen;
    unsigned m_colSize;
} RyftOne_Column;
typedef vector<RyftOne_Column> RyftOne_Columns;

class RyftOne_Database;
class RyftOne_Result : public IParser {
public:
    RyftOne_Result(RyftOne_Database *in_ryft1);

    bool open(string& in_name, vector<__catalog_entry__>::iterator in_catentry);
    bool appendFilter(string in_filter, int in_hamming);
    
    bool appendRow();
    bool flush();

    bool fetchFirst();
    bool fetchNext();

    int getColumnIndex(int col);
    int getColumnNum(string colTag);

    string& getStringValue(int colIdx);
    bool putStringValue(int colIdx, string colValue);

    // XML Parse
    virtual NodeAction AddRow( );
    virtual NodeAction AddElement( std::string sName, const char **psAttribs, IElement **ppElement );
    virtual void AddText( std::string sText );

private:

    RyftOne_Database *m_ryft1;
    rol_data_set_t m_loaded;

    int m_hamming;
    std::string m_query;
    bool m_queryFinished;

    std::string m_path;
    std::string m_name;
    std::string m_extension;
    std::string m_delim;
    std::vector<string> m_rdfTags;

    struct resultCol {
        int colNum;
        std::string colResult;
    };

    enum {
        typeNative = 0,
        typeInserted
    };

    typedef struct _Row {
        int __type;
        std::vector<resultCol> __row;
    } Row;

    std::vector<Row> m_rows;
    std::vector<Row>::iterator m_rowItr;

    bool __loadTable(vector<__catalog_entry__>::iterator in_itr);
    bool __execute();
    bool __writeRow(FILE *f, vector<Row>::iterator in_itr);
};

class RyftOne_Database {
public:
    RyftOne_Database();

    bool logon(string& in_user, string& in_password);
    void logoff();

    RyftOne_Tables getTables();
    RyftOne_Columns getColumns(string& in_table);

    RyftOne_Result *openTable(string& in_table);
    bool tableExists(string& in_table);
    void createTable(string& in_table, RyftOne_Columns& in_columns);
    void dropTable(string& in_table);

private:
    void __loadCatalog();
    vector<__catalog_entry__>::iterator __findTable(string& in_table);

    vector<__catalog_entry__> __catalog;
};

class RyftOne_Util {
public:
    static void RyftToSqlType(string& in_typeName, unsigned *out_sqlType, unsigned *out_bufferLen);
    static string SqlToRyftType(unsigned in_type, unsigned in_bufLen);
};



