#pragma once
#include <libconfig.h>
#include <vector>
#include <string>
using namespace std;

extern const char s_R1Catalog[];
extern const char s_R1Results[];
extern const char s_TableMeta[];

class __meta_config__ {
public:
    __meta_config__(string& in_table);
    __meta_config__();

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
    string delimiter;
    vector<__meta_col__> columns;
    vector<__meta_view__> views;

    void write_meta_config(string path);

private:
    void column_meta(config_t in_table_meta, string in_group, string in_name, string in_rdfname);
};

class __rdf_config__ {
public:
    enum DataType {
        dataType_None = 0,
        dataType_XML,
        dataType_JSON
    };

    __rdf_config__(string& in_path);
    __rdf_config__();

    class __rdf_tag__ {
    public:
        string name;
        string start_tag;
        string end_tag;
    };

    DataType data_type; 
    string rdf_name;
    string file_glob;
    string record_start;
    string record_end;
    bool no_top;
    int chunk_size;
    vector<__rdf_tag__> tags;

    void write_rdf_config(string path);
};

class __catalog_entry__ {
public:
    __catalog_entry__(string in_table);
    __meta_config__ meta_config;
    __rdf_config__ rdf_config;
    string path;

    bool _is_valid( );
};

#include <unistd.h>
#include <deque>
using namespace std;

enum NodeAction
{
    ProcessNode = 0,
    PushNode,
    SkipNode,
};

#define XML_STATIC
#include "expat.h"
// The XMLElement interface exposes useful behavior
// for objects that can be parsed from XML by a parser:
class XMLElement
{
    friend class XMLParser;

public:
    inline                      XMLElement() : m_bDeleteOnPop( false ) {}
    inline                      XMLElement( const std::string &sElName ) : m_sElName( sElName ), m_bDeleteOnPop( false ) {}

    // This function is called automatically by the parser
    // for whichever element is on top of the parse stack
    // when a new nested start tag is encountered.
    // Return ProcessNode to continue parsing.
    // Return PushNode and set ppElement if
    // parsing should continue with a new XMLElement object on the
    // top of the parse stack.
    // Return SkipNode to ignore the node and its children.
    virtual NodeAction          StartRow( ) { return ProcessNode; }
    virtual NodeAction          AddElement(std::string sName, const char **psAttribs, XMLElement **ppElement) { return ProcessNode; }
    
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

    inline                      XMLElement( const std::string &sElName, int iLevel, bool bDeleteOnPop ) : m_sElName( sElName ), m_iLevel( iLevel ), m_bDeleteOnPop( bDeleteOnPop ) {}

    inline void                 SetLevel( int iLevel ) { m_iLevel = iLevel; }
    inline bool                 DeleteOnPop() const { return m_bDeleteOnPop; }

    std::string                 m_sElName;          // Inherited classes MUST initialize this
                                                    // as it is used to determine when to pop
                                                    // elements from the parser stack!!

    int                         m_iLevel;           // Set by the XMLParser during XML Parsing

    bool                        m_bDeleteOnPop;     // If true, XMLParser objects will delete this
                                                    // object upon removing it from the parse stack
                                                    // during XML parsing
};

// Element objects will be stored on a stack by parsers:
typedef std::deque<XMLElement *> ElementStack;

class XMLParser : public XMLElement
{
public:
    inline          XMLParser() : m_bParsed( false ), m_iLevel( 0 ) {}
    inline          XMLParser( const std::string &wsElName ) : XMLElement( wsElName ), m_bParsed( false ), m_iLevel( 0 ), m_inNode(false) {}
  
#define PARSER_WINDOW_SIZE      0x10000
#define PARSER_SNIFF_SIZE       0x1000

    // The auto-magic parse function.  This processes relationships and then
    // parses the main XML file associated with the object:
    bool XMLParse(int fd, unsigned long dwSize, std::string delim, bool no_top)
    {
        // Open parse path XML file:
        m_sniffing = false;
        m_delim = delim;

        XML_Parser parser = XML_ParserCreate( NULL );
        if( parser )
        {
            // We will pop the first element, ourselves:
            m_ParseStack.push_back( this );

            // Set handlers:
            XML_SetElementHandler( parser, XMLParser::StartHandler, XMLParser::EndHandler );
            XML_SetCharacterDataHandler( parser, XMLParser::TextHandler );
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
                if(!pbBuffer) {
                    XML_ParserFree( parser );
                    return false;
                }
                for( dwCurr = 0; dwCurr < dwSize; dwCurr += PARSER_WINDOW_SIZE )
                {
                    dwRemain = dwSize - dwCurr;
                    bLast = dwRemain < PARSER_WINDOW_SIZE;
                    readThisLoop = read(fd, pbBuffer, bLast ? dwRemain : PARSER_WINDOW_SIZE);
                    if(readThisLoop == -1) 
                    {
                        free(pbBuffer);
                        XML_ParserFree( parser );
                        return false;
                    }
                    if( XML_STATUS_OK != XML_Parse( parser, (const char *)( pbBuffer ), readThisLoop, bLast ) ) 
                    {
                        enum XML_Error code = XML_GetErrorCode(parser);
                        break;
                    }
                    else
                        m_bParsed = true;
                }
                free(pbBuffer);
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

    bool XMLSniff(int fd, unsigned long dwSize, string& record_tag) 
    {
        m_sniffing = true;
        XML_Parser parser = XML_ParserCreate( NULL );
        if( parser )
        {
            // We will pop the first element, ourselves:
            m_ParseStack.push_back( this );

            // Set handlers:
            XML_SetElementHandler( parser, XMLParser::StartHandler, XMLParser::EndHandler );
            XML_SetCharacterDataHandler( parser, XMLParser::TextHandler );
            XML_SetUserData( parser, (void *)this );
            
            // Parse:
            try 
            {
                unsigned long dwCurr, dwRemain;
                int readThisLoop;
                bool bLast;

                char * pbBuffer = (char *)malloc(PARSER_SNIFF_SIZE);
                if(!pbBuffer) {
                    XML_ParserFree( parser );
                    return false;
                }

                dwRemain = dwSize - dwCurr;
                bLast = dwRemain < PARSER_SNIFF_SIZE;
                readThisLoop = read(fd, pbBuffer, bLast ? dwRemain : PARSER_SNIFF_SIZE);
                if(readThisLoop == -1) 
                {
                    free(pbBuffer);
                    XML_ParserFree( parser );
                    return false;
                }
                if( XML_STATUS_OK != XML_Parse( parser, (const char *)( pbBuffer ), readThisLoop, bLast ) ) 
                {
                    enum XML_Error code = XML_GetErrorCode(parser);
                    free(pbBuffer);
                    XML_ParserFree( parser );
                    return false;
                }
                else
                    m_bParsed = true;
            }
            catch(...)
            {
                m_bParsed = false;
            }
            XML_ParserFree( parser );
        }
        record_tag = m_delim;
        return m_bParsed;
    }

    inline bool     IsParsed() const { return m_bParsed; }

protected:
    // Internal implementations:
    static void XMLCALL StartHandler( void *pData, const char * sName, const char **pwsAttribs )
    {
        XMLElement *pPush = NULL;
        XMLParser *pThis = (XMLParser *)pData;
        ++pThis->m_iLevel;

        if(pThis->m_sniffing && pThis->m_iLevel == 2) {
            pThis->m_delim = sName;
        }
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
                pThis->m_ParseStack.push_back( new XMLElement( sName, pThis->m_iLevel, true ) );
                break;
            }
        }
        pThis->m_inNode = true;
    }

    static void XMLCALL TextHandler( void * pData, const char * sText, int len )
    {
        XMLParser *pThis = (XMLParser *)pData;
        if(pThis->m_inNode) {
            XMLElement *pElement = pThis->m_ParseStack.back();
            std::string sString;
            sString.append( sText, len );
            pElement->AddText( sString );
        }
    }

    static void XMLCALL EndHandler( void * pData, const char * sName )
    {
        XMLParser *pThis = (XMLParser *)pData;
        XMLElement *pElement = pThis->m_ParseStack.back();
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
    bool            m_sniffing;
};

#include <sys/mman.h>
#include <json/json.h>

class JSONElement {

    friend class JSONParser;

public:
    inline                      JSONElement() : m_bDeleteOnPop( false ) {}
    inline                      JSONElement( const std::string &sElName ) : m_sElName( sElName ), m_bDeleteOnPop( false ) {}

    // This function is called automatically by the parser
    // for whichever element is on top of the parse stack
    // when a new nested start tag is encountered.
    // Return ProcessNode to continue parsing.
    // Return PushNode and set ppElement if
    // parsing should continue with a new XMLElement object on the
    // top of the parse stack.
    // Return SkipNode to ignore the node and its children.
    virtual NodeAction          JSONStartRow( ) { return ProcessNode; }
    virtual NodeAction          JSONStartArray(std::string sName) { return ProcessNode; }
    virtual NodeAction          JSONStartGroup(std::string sName) { return ProcessNode; }
    virtual NodeAction          JSONAddElement(std::string sName, const char **psAttribs, JSONElement **ppElement) { return ProcessNode; }
    
    // This function is called automatically by the parser
    // for whichever element is on top of the parse stack
    // when enclosed text inside a pair of tags is being processed:
    virtual void                JSONAddText( std::string sText ) {}
    
    // This function is called when an element is about to be popped
    // of the the parse stack
    virtual void                JSONExitElement() {}
    virtual void                JSONExitGroup() {}
    virtual void                JSONExitArray() {}
    virtual void                JSONExitRow() {}

    inline const std::string   &ElementName() const { return m_sElName; }
    inline int                  ElementLevel() const { return m_iLevel; }

protected:

    inline                      JSONElement( const std::string &sElName, int iLevel, bool bDeleteOnPop ) : m_sElName( sElName ), m_iLevel( iLevel ), m_bDeleteOnPop( bDeleteOnPop ) {}

    inline void                 SetLevel( int iLevel ) { m_iLevel = iLevel; }
    inline bool                 DeleteOnPop() const { return m_bDeleteOnPop; }

    std::string                 m_sElName;          // Inherited classes MUST initialize this
                                                    // as it is used to determine when to pop
                                                    // elements from the parser stack!!

    int                         m_iLevel;           // Set by the XMLParser during XML Parsing

    bool                        m_bDeleteOnPop;     // If true, XMLParser objects will delete this
                                                    // object upon removing it from the parse stack
};

class JSONParser : public JSONElement {
public:

    bool JSONParse(int fd, unsigned long dwSize, bool no_top) 
    {
        unsigned long lRead = 0;
        char *buffer = (char *)mmap(NULL, dwSize, PROT_READ, MAP_PRIVATE, fd, 0);
        if(!buffer) {
            return false;
        }
        json_object *jobj;
        json_tokener *jtok = json_tokener_new();
        while(jobj = json_tokener_parse_ex(jtok, buffer + lRead, dwSize - lRead)) {
            lRead += jtok->char_offset;
            if(no_top) {
                json_parse(jobj, NULL, 1);
            }
            else
                json_parse_array(jobj, NULL, 0);
        }
        json_tokener_free(jtok);
        munmap(buffer, dwSize);
        return true;
    }

    bool JSONHasTop(int fd, unsigned long dwSize)
    {
        unsigned long lRead = 0;
        char *buffer = (char *)mmap(NULL, dwSize, PROT_READ, MAP_PRIVATE, fd, 0);
        if(!buffer) {
            return false;
        }
        json_object *jobj;
        json_tokener *jtok = json_tokener_new();
        jobj = json_tokener_parse_ex(jtok, buffer + lRead, dwSize - lRead);
        enum json_type type = json_object_get_type(jobj);
        bool hasTop = false;
        if(type == json_type_array) {
            hasTop = true;
        }
        json_tokener_free(jtok);
        munmap(buffer, dwSize);
        return hasTop;
    }

    bool JSONParseFromREST(int fd, unsigned long dwSize) 
    {
        unsigned long lRead = 0;
        char *buffer = (char *)mmap(NULL, dwSize, PROT_READ, MAP_PRIVATE, fd, 0);
        if(!buffer) {
            return false;
        }
        json_object *jobj;
        json_tokener *jtok = json_tokener_new();

        jobj = json_tokener_parse_ex(jtok, buffer + lRead, dwSize - lRead);
        if(jobj) {
            lRead += jtok->char_offset;
            jobj = json_object_object_get(jobj, "results");
            json_parse_array(jobj, NULL, 0);
        }
        json_tokener_free(jtok);
        munmap(buffer, dwSize);
        return true;
    }

private:
    void json_value(char *key, json_object *jvalue)
    {
        JSONAddElement(key, NULL, NULL);
        JSONAddText(json_object_get_string(jvalue));
    }

    void json_parse_array(json_object *jobj, char *key, int iLevel) {
        enum json_type type;
        json_object *jarray = jobj;
        if(key) {
            JSONStartArray(key);
            jarray = json_object_object_get(jobj, key);
        }
        if(jarray) {
            int i;
            int arraylen = json_object_array_length(jarray);
            json_object *jvalue;
            for(i = 0; i < arraylen; i++) {
                jvalue = json_object_array_get_idx(jarray, i);
                type = json_object_get_type(jvalue);
                if(type == json_type_array) {
                    json_parse_array(jvalue, NULL, iLevel);
                }
                else if(type == json_type_object) {
                    json_parse(jvalue, NULL, iLevel+1);
                }
                else {
                    json_value(key, jvalue);
                }
            }
        }
        if(key) {
            JSONExitArray();
        }
    }

    void json_parse(json_object * jobj, char *key, int iLevel) 
    {
        if(key) {
            JSONStartGroup(key);
            jobj = json_object_object_get(jobj, key);
        }
        else if(iLevel == 1)
            JSONStartRow( );
        if(jobj) {
            enum json_type type;
            json_object_object_foreach(jobj, key, val) {
                type = json_object_get_type(val);
                switch(type) {
                case json_type_boolean:
                case json_type_double:
                case json_type_int:
                case json_type_string:
                    json_value(key, val);
                    break;
                case json_type_object:
                    json_parse(jobj, key, iLevel+1);
                    break;
                case json_type_array:
                    json_parse_array(jobj, key, iLevel);
                    break;
                }
            }
        }
        if(key) {
            JSONExitGroup();
        }
        else if(iLevel == 1)
            JSONExitRow();
    }
};

#include <stdio.h>

class IFile {
    friend class RyftOne_Result;
public:
    IFile(string output) 
    {
        __ffile = fopen(output.c_str(), "w");
    }

   ~IFile()
    {
        if(__ffile)
            fclose(__ffile);
    }

protected:
    virtual bool prolog( ) { return false; }
    virtual bool epilog( ) { return false; }
    virtual bool startRecord( ) { return false; }
    virtual bool endRecord( ) { return false; }
    virtual bool outputField( string field, string value ) { return false; }
    virtual bool copyFile( char *src_path ) { return false; }

    FILE *  __ffile;
};

class JSONFile : public IFile, public JSONParser {
public:
    JSONFile(string output, bool no_top) : IFile(output), __no_top(no_top), __num_rows(0) { }

protected:
    // JSON Parse
    virtual NodeAction JSONStartRow( );
    virtual NodeAction JSONStartGroup( std::string sName );
    virtual NodeAction JSONStartArray( std::string sName );
    virtual NodeAction JSONAddElement( std::string sName, const char **psAttribs, JSONElement **ppElement );
    virtual void JSONAddText( std::string sText );
    virtual void JSONExitGroup();
    virtual void JSONExitArray();
    virtual void JSONExitRow();

    // IFile
    virtual bool prolog( );
    virtual bool epilog( );
    virtual bool startRecord( );
    virtual bool endRecord( );
    virtual bool outputField( string field, string value );
    virtual bool copyFile( char *src_path );

private:
    bool            __no_top;
    string          __field;
    string          __value;
    string          __predecessor;
    int             __num_rows;
    int             __num_fields;
    deque<string>   __qualifiedCol;

};

class XMLFile : public IFile, public XMLParser {
public:
    XMLFile(string output, string delim) : IFile(output), __delim(delim) { }

protected:
    // XML Parse
    virtual NodeAction StartRow( );
    virtual NodeAction AddElement( std::string sName, const char **psAttribs, XMLElement **ppElement );
    virtual void AddText( std::string sText );
    virtual void ExitRow();

    // IFile
    virtual bool prolog( );
    virtual bool epilog( );
    virtual bool startRecord( );
    virtual bool endRecord( );
    virtual bool outputField( string field, string value );
    virtual bool copyFile( char *src_path );

private:
    string  __delim;
    string  __field;
    string  __value;
};
