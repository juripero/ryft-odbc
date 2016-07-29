#pragma once

#include "RyftOne.h"
using namespace RyftOne;

namespace Simba
{
namespace Support
{
    class ILogger;
}
}

#include <libryftone.h>

#include <string.h>
#include <string>
#include <vector>
#include <deque>
using namespace std;

#include "libmeta.h"
#include "sqlite3.h"

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

__inline void CopyEscapedLiteral(string& dest, string& src)
{
    string::iterator itr; 
    try {
        for(itr = src.begin(); itr != src.end(); ++itr) {
            if(*itr == '\'') {
                dest += "''";
            }
            else
                dest += *itr;
        }
    }
    catch(exception& e) {
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

class RyftOne_Result : public XMLParser, public JSONParser {
public:
    RyftOne_Result(ILogger *);
   ~RyftOne_Result( );

    void open(string& in_name, vector<__catalog_entry__>::iterator in_catentry, string in_server, string in_token);
    void appendFilter(string in_filter, int in_hamming, int in_edit, bool in_caseSensitive);
    
    void prepareAppend();
    void finishUpdate();
    void flush();

    bool fetchFirst();
    bool fetchNext();
    void closeCursor();

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
    virtual NodeAction AddElement( std::string sName, const char **psAttribs, XMLElement **ppElement );
    virtual void AddText( std::string sText );
    virtual void ExitRow();

    // JSON Parse
    virtual NodeAction JSONStartRow( );
    virtual NodeAction JSONStartArray( std::string sName );
    virtual NodeAction JSONStartGroup( std::string sName );
    virtual NodeAction JSONAddElement( std::string sName, const char **psAttribs, JSONElement **ppElement );
    virtual void JSONAddText( std::string sText );
    virtual void JSONExitArray();
    virtual void JSONExitGroup();
    virtual void JSONExitRow();

private:
    __rdf_config__::DataType __dataType;

    RyftOne_Columns __cols;
    vector<string> __glob;

    int __hamming;
    int __edit;
    bool __caseSensitive;

    string __query;
    bool __queryFinished;

    string __path;
    string __name;
    string __extension;
    string __delimiter;
    vector<string> __metaTags;
    bool __no_top;

    deque<string> __qualifiedCol;

    sqlite3 *__sqlite;
    sqlite3_stmt *__stmt;
    int __transMax;

    Cursor __cursor;

    // Reference to the ILogger. (NOT OWN)
    ILogger* __log;

    string __restServer;
    string __restToken;

    void __loadTable(string& in_name, vector<__catalog_entry__>::iterator in_itr);
    bool __execute();

    int __getColumnNum(string colTag);

    bool __initTable(string &query, string& table);
    void __dropTable(string &query);

    bool __storeToSqlite(string& table, const char *file, bool no_top, bool is_query );
    bool __loadFromSqlite(string &query);

    void __date(const char *dateStr, int colIdx, struct tm *date);
    void __time(const char *timeStr, int colIdx, struct tm *time);
    void __datetime(const char *datetimeStr, int colIdx, struct tm*time);
};

