#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <libconfig.h>
using namespace std;

const char _exenam[]="ryft1_odbcd";
const char _pidfile[]=".r1odbcd.pid";
const char _R1Catalog[] = "/ryftone/ODBC";
const char _R1Results[] = "/ryftone/ODBC/.results";
const char _TableMeta[] = ".meta.table";

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
                //XML_Parse( parser, "<xml_start>", strlen("<xml_start>"), false);

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

bool readNumber(const char *cstr, double& dbl) {
    dbl = 0;
    char *cnum = new char[strlen(cstr) + 1];
    char *c = cnum;
    *c++ = *cstr++;
    while (*cstr >= '0' && *cstr <= '9')
        *c++ = *cstr++;
    // fractional part
    if (*cstr == '.') {
        *c++ = *cstr++;
        while (*cstr >= '0' && *cstr <= '9')
            *c++ = *cstr++;
    }
    // exponential part
    if (*cstr == 'e' || *cstr == 'E') {
        *c++ = *cstr++;
        if (*cstr == '+' || *cstr == '-')
            *c++ = *cstr++;
        while (*cstr >= '0' && *cstr <= '9')
            *c++ = *cstr++;
    }
    if(*cstr)
        return false;

    dbl = atof(cnum);
    delete cnum;
    return true;
}

#include <math.h>
static bool IsIntegral(double d) {
  double integral_part;
  return modf(d, &integral_part) == 0.0;
}

class __token {
public:
    __token( ) { }

    enum ValueType {
        tokenNull,
        tokenString,
        tokenInteger,
        tokenNumber
    } ;

    ValueType _type;

    void parse(string value) {
        const char *cstr = value.c_str();
        double dbl;
        if(cstr) {
            switch(*cstr) {
            case 0:
                _type = tokenNull;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '-':
                _type = tokenString;
                if(readNumber(cstr, dbl)) {
                    _type = tokenNumber;
                    if(IsIntegral(dbl))
                        _type = tokenInteger;
                }
                break;
            default:
                _type = tokenString;
            }
        }
    }
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

typedef struct __column {
    unsigned    _ordinal;
    string      _colName;
    unsigned    _bufferLen;
    int         _type_num;
    int         _type_text;
    int         _type_int;
} __column;
typedef vector<__column> __columns;

class GuessColumn : public IParser {
public:
    GuessColumn( ) : m_count(0) { }

    // XML Parse
    virtual NodeAction AddRow( );
    virtual NodeAction AddElement( std::string sName, const char **psAttribs, IElement **ppElement );
    virtual void AddText( std::string sText );

    __columns& getCols( ) { return m_cols; }

private:
    int             m_count;
    std::string     m_delim;

    __columns       m_cols;
    int             m_colIdx;

    vector<string>  m_strings;
};

IElement::NodeAction GuessColumn::AddRow()
{
    m_count++;
    m_colIdx = -1;
    __columns::iterator itr;
    vector<string>::iterator strItr;
    for(strItr = m_strings.begin(), itr = m_cols.begin(); itr != m_cols.end(); itr++, strItr++) {
        __token token;
        token.parse(*strItr);
        switch(token._type) {
        case __token::tokenInteger:
            (*itr)._type_int++;
        case __token::tokenNumber:
            (*itr)._type_num++;
            break;
        case __token::tokenString:
            (*itr)._type_text++;
            if((*strItr).length() > (*itr)._bufferLen)
                (*itr)._bufferLen = (*strItr).length();
            break;
        }
        (*strItr).clear();
    }
    return ProcessNode;
}

IElement::NodeAction GuessColumn::AddElement(std::string sName, const char **psAttribs, IElement **ppElement)
{
    if(m_count == 1) {
        __column col = {m_colIdx + 1, sName, 0, 0, 0, 0 }; 
        m_cols.push_back(col);
        m_strings.push_back("");
    }
    m_colIdx++;
    return ProcessNode;
}

void GuessColumn::AddText(std::string sText) 
{
    if(m_strings.size() > m_colIdx)
        m_strings[m_colIdx] += sText;
}

__meta_config__::__meta_config__(string& in_table)
{
    config_t tableMeta;
    config_setting_t *colList;
    config_setting_t *column;
    char path[PATH_MAX];
    __meta_col__ col;
    config_setting_t *view;
    config_setting_t *viewList;
    __meta_view__ v;
    const char *result;
    int idx;

    strcpy(path, _R1Catalog);
    strcat(path, "/");
    strcat(path, in_table.c_str());
    strcat(path, "/");
    strcat(path, _TableMeta);

    config_init(&tableMeta);
    if(CONFIG_TRUE == config_read_file(&tableMeta, path)) {
        if(CONFIG_TRUE == config_lookup_string(&tableMeta, "table_name", &result)) 
            table_name = in_table;
        if(CONFIG_TRUE == config_lookup_string(&tableMeta, "table_remarks", &result))
            table_remarks = result;
        if(CONFIG_TRUE == config_lookup_string(&tableMeta, "rdf", &result))
            rdf_path = result;

        colList = config_lookup(&tableMeta, "columns");
        for(idx=0; colList && (column = config_setting_get_elem(colList, idx)); idx++) {
            col.rdf_name = column->name;
            col.name = config_setting_get_string_elem(column, 0);
            col.type_def = config_setting_get_string_elem(column, 1);
            col.description = config_setting_get_string_elem(column, 2);
            columns.push_back(col);
        }

        viewList = config_lookup(&tableMeta, "views");
        for(idx=0; viewList && (view = config_setting_get_elem(viewList, idx)); idx++) {
            v.id = view->name;
            v.view_name = config_setting_get_string_elem(view, 0);
            v.restriction = config_setting_get_string_elem(view, 1);
            v.description = config_setting_get_string_elem(view, 2);
            views.push_back(v);
        }
    }
    config_destroy(&tableMeta);
}

__rdf_config__::__rdf_config__(string& in_path)
{
    config_t rdfConfig;
    config_setting_t *tagList;
    config_setting_t *tag;
    __rdf_tag__ rdftag;
    const char *result;
    int idx;

    config_init(&rdfConfig);
    if(CONFIG_TRUE == config_read_file(&rdfConfig, in_path.c_str())) {
        if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "file_glob", &result)) 
            file_glob = result;
        if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "record_start", &result))
            record_start = result;
        if(CONFIG_TRUE == config_lookup_string(&rdfConfig, "record_end", &result))
            record_end = result;

        tagList = config_lookup(&rdfConfig, "fields");
        // timwells - leave in for backward compatibility with older RDFs
        if(!tagList)
            tagList = config_lookup(&rdfConfig, "tags");
        for(idx=0; tagList && (tag = config_setting_get_elem(tagList, idx)); idx++) {
            rdftag.name = tag->name;
            rdftag.start_tag = config_setting_get_string_elem(tag, 0);
            rdftag.end_tag = config_setting_get_string_elem(tag, 1);
            tags.push_back(rdftag);
        }
    }
    config_destroy(&rdfConfig);
}

__catalog_entry__::__catalog_entry__(string in_table) : meta_config(in_table) 
{
    path = _R1Catalog;
    path += "/";
    path += in_table;
}

vector<__catalog_entry__> __catalog;

__pid_t get_lock_file(string& exepath)
{
    char pidfile[PATH_MAX];
    string path = ".";
    size_t idx = exepath.rfind("/");
    if(idx != string::npos) 
        path = exepath.substr(0, idx);
    
    sprintf(pidfile, "%s/%s", path.c_str(), _pidfile);
    int lfp = open(pidfile, O_RDONLY);
    if(lfp < 0) 
        return -1;
    char str[10];
    __pid_t pid;
    read(lfp, str, sizeof(str));
    sscanf(str, "%d", &pid);
    close(lfp);
    return pid;
}

void put_lock_file(__pid_t pid, string& exepath)
{
    char pidfile[PATH_MAX];
    string path = ".";
    size_t idx = exepath.rfind("/");
    if(idx != string::npos) 
        path = exepath.substr(0, idx);
    
    sprintf(pidfile, "%s/%s", path.c_str(), _pidfile);
    int lfp = open(pidfile, O_RDWR|O_CREAT, 0640);
    if(lfp < 0) 
        exit(1);
    char str[10];
    sprintf(str, "%d", pid);
    write(lfp, str, strlen(str));
}

bool is_process_running(__pid_t pid)
{
    if(kill(pid, 0) == 0) 
        return true;
    return false;
}

void background()
{
    __pid_t pid = fork();
    if(pid < 0) {
        cerr << "Could not create child process, exiting";
        exit(1);
    }
    if(pid > 0) 
        exit(0);    // parent exit
    setsid();       // obtain new process group
}

int remove_directory(const char *path)
{
   DIR *d = opendir(path);
   size_t path_len = strlen(path);
   int r = -1;

   if (d) {
      struct dirent *p;

      r = 0;
      while (!r && (p=readdir(d)))
      {
          int r2 = -1;
          char *buf;
          size_t len;

          /* Skip the names "." and ".." as we don't want to recurse on them. */
          if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
             continue;

          len = path_len + strlen(p->d_name) + 2; 
          buf = (char *)malloc(len);

          if (buf) {
             struct stat statbuf;

             snprintf(buf, len, "%s/%s", path, p->d_name);
             if (!stat(buf, &statbuf)) {
                if (S_ISDIR(statbuf.st_mode)) {
                   r2 = remove_directory(buf);
                }
                else
                   r2 = unlink(buf);
             }
             free(buf);
          }
          r = r2;
      }
      closedir(d);
   }

   if (!r) {
      r = rmdir(path);
   }

   return r;
}

void stop(string path)
{
    __pid_t pid = get_lock_file(path);
    if((pid != -1) && is_process_running(pid)) {
        cout << "killing process " << pid << "...";
        kill(pid, 15);
        while(is_process_running(pid)) {
            cout << ".";
            sleep(1);
        }
        cout << "done.\n";
    }
    remove_directory(_R1Results);
}

void start(string path)
{
    __pid_t pid = get_lock_file(path);
    if((pid != -1) && is_process_running(pid)) {
        cout << "ryft1_odbcd already running, restart now (yes/no)? ";
        string in;
        getline(std::cin, in);
        if(in != "y" && in != "yes") 
            return;
        stop(path);
    }

    cout << "starting ryft1_odbcd..." + path + "\n";

    background();

    string epath;
    size_t idx = path.rfind("/");
    if(idx != string::npos) {
        epath = path.substr(0, idx);
        epath.append("/");
    }

    put_lock_file(getpid(), path);

    // make the ODBC directory if its not already there
    mkdir(_R1Catalog, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
    mkdir(_R1Results, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 

    // make directory containing bin current directory
    chdir(epath.c_str());
    putenv("LD_LIBRARY_PATH=../lib/x8664");
    if(execl(path.c_str(), _exenam, (char *)NULL)) {
        cerr << "error starting ryft1_odbcd daemon \"" << path << "\" (" << errno << ")\n";
    }
}

void load()
{
    DIR *d;
    struct dirent *dir;
    __catalog.clear();
    if(d = opendir(_R1Catalog)) {
        while((dir = readdir(d)) != NULL) {
            if((dir->d_type == DT_DIR) && (strcmp(dir->d_name, ".") != 0)) {
                __catalog_entry__ cat_ent(dir->d_name);
                if(!cat_ent.meta_config.table_name.empty())
                    __catalog.push_back(cat_ent);
            }
        }
        closedir(d);
    }
}

void list( )
{
    char listitem[260];
    char format[] = "\t%2d\t%s\n";
    load();
    cout << "\tid\ttable_name\n";
    cout << "\t--\t----------\n";
    for(int i = 0; i < __catalog.size(); i++) {
        sprintf(listitem, format, i+1, __catalog[i].meta_config.table_name.c_str()); 
        cout << listitem;
    }
}

string getColumnType(__column& col)
{
    char typeString[80];
    int siz = 1;

    if(col._type_text) {
        while(siz < col._bufferLen)
            siz = siz * 2;
        sprintf(typeString, "VARCHAR(%d)", siz);
    } 
    else if(col._type_int == col._type_num) {
            sprintf(typeString, "INTEGER");
    }
    else
        sprintf(typeString, "DOUBLE");
    return typeString;
}

void add(string name, string filespec, string delim, int chunk)
{
    char path[PATH_MAX];
    char config_path[PATH_MAX];
    char rdf_path[PATH_MAX];
    char rdf_glob[PATH_MAX];

    // make the ODBC directory if its not already there
    mkdir(_R1Catalog, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
    strcpy(path, _R1Catalog);
    strcat(path, "/");
    strcat(path, name.c_str());
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    strcpy(config_path, path);
    strcat(config_path, "/");
    strcat(config_path, _TableMeta);

    sprintf(rdf_path, "%s/%s.rdf", path, name.c_str());
    sprintf(rdf_glob, "*.%s", name.c_str());

    int fd;
    struct stat sb;
    fd = ::open(filespec.c_str(), O_RDONLY);
    if(fd == -1) {
        cerr << "Could not read input file \"" << filespec << "\" (" << errno << ")\n";
        return;
    }

    fstat(fd, &sb);
    char *buffer = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    GuessColumn guess;
    guess.Parse(buffer, sb.st_size, delim);
    munmap(buffer, sb.st_size);
    close(fd);

    __columns cols = guess.getCols();

    config_t tableMeta;
    config_init(&tableMeta);

    config_setting_t *root = config_root_setting(&tableMeta);

    config_setting_t *table_name = config_setting_add(root, "table_name", CONFIG_TYPE_STRING);
    config_setting_set_string(table_name, name.c_str());
    config_setting_t *table_remarks = config_setting_add(root, "table_remarks", CONFIG_TYPE_STRING);
    config_setting_set_string(table_remarks, "");
    config_setting_t *table_rdf = config_setting_add(root, "rdf", CONFIG_TYPE_STRING);
    config_setting_set_string(table_rdf, rdf_path);

    __columns::iterator itr;
    config_setting_t *colListEntry;
    config_setting_t *colList = config_setting_add(root, "columns", CONFIG_TYPE_GROUP);
    for(itr = cols.begin(); itr != cols.end(); itr++) {
        colListEntry = config_setting_add(colList, itr->_colName.c_str(), CONFIG_TYPE_LIST);
        config_setting_t *colElem = config_setting_add(colListEntry, "", CONFIG_TYPE_STRING);
        config_setting_set_string(colElem, itr->_colName.c_str());
        colElem = config_setting_add(colListEntry, "", CONFIG_TYPE_STRING);
        config_setting_set_string(colElem, getColumnType((*itr)).c_str());
        colElem = config_setting_add(colListEntry, "", CONFIG_TYPE_STRING);
        config_setting_set_string(colElem, "");
    }
    config_write_file(&tableMeta, config_path);
    config_destroy(&tableMeta);

    // create an rdf
    config_t tableRdf;
    config_init(&tableRdf);

    root = config_root_setting(&tableRdf);
    char tagEntry[PATH_MAX];
    config_setting_t *rdf_name = config_setting_add(root, "rdf_name", CONFIG_TYPE_STRING);
    config_setting_set_string(rdf_name, name.c_str());
    config_setting_t *chunk_size = config_setting_add(root, "chunk_size_mb", CONFIG_TYPE_INT);
    config_setting_set_int(chunk_size, chunk);
    config_setting_t *file_glob = config_setting_add(root, "file_glob", CONFIG_TYPE_STRING);
    config_setting_set_string(file_glob, rdf_glob);
    config_setting_t *record_start = config_setting_add(root, "record_start", CONFIG_TYPE_STRING);
    sprintf(tagEntry, "<%s>", delim.c_str());
    config_setting_set_string(record_start, tagEntry);
    config_setting_t *record_end = config_setting_add(root, "record_end", CONFIG_TYPE_STRING);
    sprintf(tagEntry, "</%s>", delim.c_str());
    config_setting_set_string(record_end, tagEntry);
    config_setting_t *data_type = config_setting_add(root, "data_type", CONFIG_TYPE_STRING);
    config_setting_set_string(data_type, "XML");

    config_setting_t *tagListEntry;
    config_setting_t *tagList = config_setting_add(root, "fields", CONFIG_TYPE_GROUP);
    for(itr = cols.begin(); itr != cols.end(); itr++) {
        tagListEntry = config_setting_add(tagList, itr->_colName.c_str(), CONFIG_TYPE_LIST);
        config_setting_t *tagElem = config_setting_add(tagListEntry, "", CONFIG_TYPE_STRING);
        sprintf(tagEntry, "<%s>", itr->_colName.c_str());
        config_setting_set_string(tagElem, tagEntry);
        tagElem = config_setting_add(tagListEntry, "", CONFIG_TYPE_STRING);
        sprintf(tagEntry, "</%s>", itr->_colName.c_str());
        config_setting_set_string(tagElem, tagEntry);
    }
    config_write_file(&tableRdf, rdf_path);
    config_destroy(&tableRdf);

    int pid = 0;
    int ret;
    char rhfsctl[PATH_MAX];
    sprintf(rhfsctl, "rhfsctl -a %s", rdf_path);
    ret = system(rhfsctl);
    ret = WEXITSTATUS(ret);

    char cpcmd[PATH_MAX];
    string filename = filespec.substr(0, filespec.find_last_of("."));
    if(filespec.find_last_of("/") != string::npos)
        filename = filespec.substr(filespec.find_last_of("/"), filespec.find_last_of("."));
    sprintf(cpcmd, "cp %s %s/%s.%s", filespec.c_str(), path, filename.c_str(), name.c_str());
    system(cpcmd);
}

void del(string id)
{
    char path[PATH_MAX];
    int idx = atoi(id.c_str()) - 1;
    load();
    cout << "really delete table " + __catalog[idx].meta_config.table_name + " (yes/no)? ";
    string in;
    getline(std::cin, in);
    if(in != "y" && in != "yes") 
        return;

    strcpy(path, _R1Catalog);
    strcat(path, "/");
    strcat(path, __catalog[idx].meta_config.table_name.c_str());
    remove_directory(path);
}

void edit(string id)
{
    char path[PATH_MAX];
    int idx = atoi(id.c_str()) - 1;
    load();
    strcpy(path, _R1Catalog);
    strcat(path, "/");
    strcat(path, __catalog[idx].meta_config.table_name.c_str());
    strcat(path, "/");
    strcat(path, _TableMeta);
    char vi[PATH_MAX];
    sprintf(vi, "vi %s", path);
    system(vi);
}

void version(string exepath)
{
    char cat[PATH_MAX];
    size_t idx1 = exepath.rfind("/");
    string version(exepath.substr(0, idx1));
    version.append("/../../VERSION");
    sprintf(cat, "cat %s", version.c_str());
    system(cat);
}

void usage(string name)
{
    cout << "usage: " << name << " [opts] [args]\n";
    cout << "  opts:\n";
    cout << "  -a, --add\tAdd file as a new database\t(Syntax: -a db_name xml_file record_delim [chunk_size])\n";
    cout << "  -d, --del\tDelete installed database\t(Syntax: -d id)\n";
    cout << "  -e, --edit\tEdit database metadata\t(Syntax: -e id)\n";
    cout << "  -h, --help\tDisplay help\n";
    cout << "  -k, --kill\tKill Ryft ODBC daemon\n";
    cout << "  -l, --list\tList installed databases\n";
    cout << "  -s, --start\tStart Ryft ODBC daemon\n";
    cout << "  -v, --version\tServer version\n";
}

int main(int argc, char *argv[])
{
    char exepath[PATH_MAX];
    char szlink[PATH_MAX];
    sprintf(szlink, "/proc/%d/exe", getpid());
    int bytes = readlink(szlink, exepath, PATH_MAX);
    if(bytes > 0)
        exepath[bytes] = '\0';
    
    if(argc < 2) {
        usage(argv[0]);
        return 1;
    }
    for(int i = 1; i < argc; i++) {
        string arg = argv[i];
        if((arg == "-a") || (arg == "--add")) {
            string name, filespec, delim;
            int chunk = 64;
            if(i + 3 < argc) {
                name = argv[++i];
                filespec = argv[++i];
                delim = argv[++i];
            }
            if(i + 1 < argc) {
                char *chunk_sz = argv[++i];
                chunk = atoi(chunk_sz);
            }
            add(name, filespec, delim, chunk);
        }
        else if((arg == "-d") || (arg == "--del")) {
            string id;
            if(i + 1 < argc) {
                id = argv[++i];
            }
            else usage(argv[0]);
            del(id);
        }
        else if((arg == "-e") || (arg == "--edit")) {
            string id;
            if(i + 1 < argc) {
                id = argv[++i];
            }
            else usage(argv[0]);
            edit(id);
        }
        else if((arg == "-h") || (arg == "--help")) {
            usage(argv[0]);
            return 0;
        }
        else if((arg == "-k") || (arg == "--kill")) {
            stop(exepath);
        }
        else if((arg == "-l") || (arg == "--list")) {
            list();
        }
        else if((arg == "-s") || (arg == "--start")) {
            string path;
            if(i + 1 < argc) {
                path = argv[++i];
            }
            else {
                path = exepath;
                size_t idx = path.rfind("/");
                path = path.substr(0, idx) + "/" + string(_exenam);
            }
            start(path);
        }
        else if((arg == "-v") || (arg == "--version")) {
            version(exepath);
        }
        else {
            cerr << "invalid argument " << argv[i] << "\n\n";
            usage(argv[0]);
            return 1;
        }
    }
 	return 0;
}