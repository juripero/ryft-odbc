// =================================================================================================
///  @file libmeta.h
///
///  Handles meta functions for ODBC
///
///  Copyright (C) 2017 Ryft Systems, Inc.
// =================================================================================================
#pragma once
#include <libconfig.h>
#include <vector>
#include <string>
using namespace std;

extern const char s_R1Root[];
extern const char s_R1Catalog[];
extern const char s_R1Results[];
extern const char s_R1Unload[];
extern const char s_R1Caches[];
extern const char s_TableMeta[];
extern const char s_RyftUser[];

enum DataType {
    dataType_None = 0,
    dataType_XML,
    dataType_JSON,
    dataType_CSV,
    dataType_PCAP
};

class __meta_config__ {
public:
    __meta_config__(string& in_dir);
    __meta_config__();

    class __meta_col__ {
    public:
        string name;
        string type_def;
        string description;
        string meta_name;
        string json_or_xml_tag;
    };

    class __meta_view__ {
    public:
        string id;
        string view_name;
        string restriction;
        string description;
    };

    // common
    int version;
    string table_name;
    string table_remarks;
    string delimiter;
    vector<__meta_col__> columns;
    vector<__meta_view__> views;

    struct stat metafile_stat;
    
    // version 1
    string rdf_path;

    // version 2
    DataType data_type; 
    string file_glob;
    // XML
    string record_tag;
    // JSON
    bool no_top;
    // CSV
    string record_delimiter;
    string field_delimiter;
    // adding PIP capability
    string pip_format;
    // PCAP
    #define FILTER_EQ           0
    #define FILTER_NE           1
    #define FILTER_LIKE         2
    #define FILTER_NOT_LIKE     3
    class __meta_filter__ {
    public:
        string id;
        string filter_name;
        string eq;
        string ne;
    };

    vector<__meta_filter__> filters;

    void write_meta_config(string path);

private:
    void column_meta(config_t in_table_meta, string in_group, string in_name, string in_rdfname);
};

class __rdf_config__ {
public:
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
    int chunk_size;
    // XML
    string record_start;
    string record_end;
    vector<__rdf_tag__> tags;
    // JSON
    bool no_top;
    // CSV
    string record_delimiter;
    string field_delimiter;

    void write_rdf_config(string path);
};

class __catalog_entry__ {
public:
    __catalog_entry__(string in_dir);
    __meta_config__ meta_config;
    
    string __path;

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
#define PARSER_WINDOW_SIZE      0x10000
#define PARSER_SNIFF_SIZE       0x1000

static size_t __stripControlChars(char *pbuf1, char *pbuf2, size_t siz)
{
    size_t out_siz = 0;
    for(size_t i = 0; i < siz; i++) {
        if(pbuf1[i] > 0 && pbuf1[i] <= 0x1F) {
            switch(pbuf1[i]) {
            case 0x09:
            case 0x0A:
            case 0x0D:
                pbuf2[out_siz++] = pbuf1[i];
                break;
            default:
                break;
            }
        }
        else
            pbuf2[out_siz++] = pbuf1[i];
    }
    return out_siz;
}

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
  
    // The auto-magic parse function.  This processes relationships and then
    // parses the main XML file associated with the object:
    bool XMLParse(int fd, size_t stSize, std::string delim, bool no_top)
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
                size_t stCurr, stRemain;
                int readThisLoop;
                bool bLast;
                if(no_top)
                    XML_Parse( parser, "<xml_start>", strlen("<xml_start>"), false);

                char * pbBuffer = (char *)malloc(PARSER_WINDOW_SIZE);
                char * pbBuffer2 = (char *)malloc(PARSER_WINDOW_SIZE);
                if(!pbBuffer || !pbBuffer2) {
                    XML_ParserFree( parser );
                    return false;
                }
                for( stCurr = 0; stCurr < stSize; stCurr += PARSER_WINDOW_SIZE )
                {
                    stRemain = stSize - stCurr;
                    bLast = stRemain < PARSER_WINDOW_SIZE;
                    readThisLoop = read(fd, pbBuffer, bLast ? stRemain : PARSER_WINDOW_SIZE);
                    if(readThisLoop == -1) 
                    {
                        free(pbBuffer);
                        free(pbBuffer2);
                        XML_ParserFree( parser );
                        return false;
                    }
                    readThisLoop = __stripControlChars(pbBuffer, pbBuffer2, readThisLoop);
                    if( XML_STATUS_OK != XML_Parse( parser, (const char *)( pbBuffer2 ), readThisLoop, bLast ) ) 
                    {
                        enum XML_Error code = XML_GetErrorCode(parser);
                        break;
                    }
                    else
                        m_bParsed = true;
                }
                free(pbBuffer);
                free(pbBuffer2);
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

    bool XMLSniff(int fd, size_t stSize, string& record_tag) 
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
                int readThisLoop;
                bool bLast;

                char * pbBuffer = (char *)malloc(PARSER_SNIFF_SIZE);
                if(!pbBuffer) {
                    XML_ParserFree( parser );
                    return false;
                }

                bLast = stSize < PARSER_SNIFF_SIZE;
                readThisLoop = read(fd, pbBuffer, bLast ? stSize : PARSER_SNIFF_SIZE);
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
    inline                      JSONElement() {}
    inline                      JSONElement( const std::string &sElName ) : m_sElName( sElName ) {}

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

protected:

    std::string                 m_sElName;          // Inherited classes MUST initialize this
                                                    // as it is used to determine when to pop
                                                    // elements from the parser stack!!
};

class JSONParser : public JSONElement {
public:

    bool JSONParse(int fd, size_t stSize, bool no_top, string top_object) 
    {
        size_t stRead = 0;
        size_t stCurr = 0;
        size_t stRemain;
        int readThisLoop;
        bool bLast;

        char *buffer = (char *)malloc(PARSER_WINDOW_SIZE);
        if(!buffer)
            return false;

        json_object *jobj;
        json_tokener *jtok = json_tokener_new();
        enum json_tokener_error jerr;

        do {
            stRemain = stSize - stCurr;
            bLast = stRemain < PARSER_WINDOW_SIZE;
            readThisLoop = read(fd, buffer, bLast ? stRemain : PARSER_WINDOW_SIZE);
            if(readThisLoop == -1) {
                free(buffer);
                json_tokener_free(jtok);
                return false;
            }
            stCurr += readThisLoop;

            stRead = 0;
            while((jobj = json_tokener_parse_ex(jtok, buffer + stRead, readThisLoop - stRead)) && no_top) {
                stRead += jtok->char_offset;
                json_parse(jobj, NULL, 1);
                json_object_put(jobj);
            }
        } 
        while(((jerr = json_tokener_get_error(jtok)) == json_tokener_continue) && (stCurr < stSize));

        free(buffer);

        if(jobj && !no_top) {
            json_object *jobj_array = jobj;
            if(!top_object.empty()) {
                json_object_object_get_ex(jobj, top_object.c_str(), &jobj_array);
            }
            json_parse_array(jobj_array, NULL, 0);
            json_object_put(jobj);
        }
        json_tokener_free(jtok);
        return true;
    }

    bool JSONHasTop(int fd, size_t stSize)
    {
        size_t stCurr = 0;
        size_t stRemain;
        int readThisLoop;
        bool bLast;
        char *buffer = (char *)malloc(PARSER_WINDOW_SIZE);
        if(!buffer)
            return false;

        json_object *jobj;
        json_tokener *jtok = json_tokener_new();
        enum json_tokener_error jerr;

        do {
            stRemain = stSize - stCurr;
            bLast = stRemain < PARSER_WINDOW_SIZE;
            readThisLoop = read(fd, buffer, bLast ? stRemain : PARSER_WINDOW_SIZE);
            if(readThisLoop == -1) {
                free(buffer);
                json_tokener_free(jtok);
                return false;
            }
            stCurr += readThisLoop;
            jobj = json_tokener_parse_ex(jtok, buffer, readThisLoop);
        } 
        while(((jerr = json_tokener_get_error(jtok)) == json_tokener_continue) && (stCurr < stSize));

        free(buffer);

        enum json_type type = json_object_get_type(jobj);
        bool hasTop = false;
        if(type == json_type_array) {
            hasTop = true;
        }
        json_tokener_free(jtok);
        return hasTop;
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
                    json_value("", jvalue);
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

#include "csv.h"
class CSVElement
{
    friend class CSVParser;

public:
    inline                      CSVElement() {}
    inline                      CSVElement( const std::string &sElName ) : m_sElName( sElName ) {}

    // This function is called automatically by the parser
    // for whichever element is on top of the parse stack
    // when a new nested start tag is encountered.
    // Return ProcessNode to continue parsing.
    // Return PushNode and set ppElement if
    // parsing should continue with a new XMLElement object on the
    // top of the parse stack.
    // Return SkipNode to ignore the node and its children.
    virtual NodeAction          CSVStartRow( ) { return ProcessNode; }
    virtual NodeAction          CSVAddElement(int valueIndex) { return ProcessNode; }
    
    // This function is called automatically by the parser
    // for whichever element is on top of the parse stack
    // when enclosed text inside a pair of tags is being processed:
    virtual void                CSVAddText( std::string sText ) {}
    
    // This function is called when an element is about to be popped
    // of the the parse stack
    virtual void                CSVExitElement() {}
    virtual void                CSVExitRow() {}

    inline const std::string   &ElementName() const { return m_sElName; }

protected:

    std::string                 m_sElName;          // Inherited classes MUST initialize this
                                                    // as it is used to determine when to pop
                                                    // elements from the parser stack!!
};

class CSVParser : public CSVElement
{
public:
    inline          CSVParser() : m_bParsed( false ) {}
    inline          CSVParser( const std::string &wsElName ) : CSVElement( wsElName ), m_bParsed( false ), m_inNode(false) {}
  
    bool CSVParse(int fd, size_t stSize, std::string value_delim, std::string record_delim)
    {
        // Open parse path XML file:
        m_sniffing = false;
        
        CSV_Parser parser = CSV_ParserCreate( );
        if( parser )
        {
            CSV_SetDelimiter(parser, value_delim[0], record_delim[0]);

            // Set handlers:
            CSV_SetValueHandler( parser, CSVParser::StartHandler, CSVParser::EndHandler );
            CSV_SetCharacterDataHandler( parser, CSVParser::TextHandler );
            CSV_SetUserData( parser, (void *)this );
            
            // Parse:
            try 
            {
                size_t stCurr, stRemain;
                int readThisLoop;
                bool bLast;

                char * pbBuffer = (char *)malloc(PARSER_WINDOW_SIZE);
                if(!pbBuffer) {
                    CSV_ParserFree( parser );
                    return false;
                }
                for( stCurr = 0; stCurr < stSize; stCurr += PARSER_WINDOW_SIZE )
                {
                    stRemain = stSize - stCurr;
                    bLast = stRemain < PARSER_WINDOW_SIZE;
                    readThisLoop = read(fd, pbBuffer, bLast ? stRemain : PARSER_WINDOW_SIZE);
                    if(readThisLoop == -1) 
                    {
                        free(pbBuffer);
                        CSV_ParserFree( parser );
                        return false;
                    }
                    if( CSV_STATUS_OK != CSV_Parse( parser, (const char *)( pbBuffer ), readThisLoop, bLast ) ) 
                    {
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
            CSV_ParserFree( parser );
        }

        // NOTE: No need to pop us, as the close tag handler will pop us automatically.
        return m_bParsed;
    }

    bool CSVSniff(int fd, size_t stSize, string& record_tag) 
    {
        return true;
    }

    inline bool     IsParsed() const { return m_bParsed; }

protected:
    // Internal implementations:
    static void CSVCALL StartHandler( void *pData, int valueIndex )
    {
        CSVParser *pThis = (CSVParser *)pData;
        if(valueIndex == 1) {
            pThis->CSVStartRow();
        }
        pThis->CSVAddElement(valueIndex);
    }

    static void CSVCALL TextHandler( void * pData, const char * sText, int len )
    {
        string text;
        text.append(sText, len);
        CSVParser *pThis = (CSVParser *)pData;
        pThis->CSVAddText(text);
    }

    static void CSVCALL EndHandler( void * pData, int valueIndex, bool endLine )
    {
        CSVParser *pThis = (CSVParser *)pData;
        pThis->CSVExitElement();
        if(endLine)
            pThis->CSVExitRow();
    }

    bool            m_bParsed;
    bool            m_inNode;
    int             m_iLevel;
    bool            m_sniffing;
};

#include <stdio.h>
#include <pwd.h>

class IFile {
    friend class IQueryResult;

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

class XMLFile : public IFile, public XMLParser {
public:
    XMLFile(string output, string delim) : IFile(output), __delim(delim), __in_row(false) { }

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
    bool    __in_row;
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

class CSVFile : public IFile, public CSVParser {
public:
    CSVFile(string output, string field_delimiter, string record_delimiter) : IFile(output), 
        __field_delimiter(field_delimiter), __record_delimiter(record_delimiter)  { }

protected:
    // CSV Parse
    virtual NodeAction CSVStartRow();
    virtual NodeAction CSVAddElement(int valueIndex);
    virtual void CSVAddText( std::string sText );
    virtual void CSVExitElement();
    virtual void CSVExitRow();

    // IFile
    virtual bool prolog( );
    virtual bool epilog( );
    virtual bool startRecord( );
    virtual bool endRecord( );
    virtual bool outputField( string field, string value );
    virtual bool copyFile( char *src_path );

private:
    string  __field_delimiter;
    string  __record_delimiter;
    string  __value;
    bool    __output;
};
