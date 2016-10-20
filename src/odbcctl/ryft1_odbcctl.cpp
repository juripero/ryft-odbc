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
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <libconfig.h>
#include <math.h>
#include <string>
#include <vector>
using namespace std;

#include "libmeta.h"

const char _exenam[]="ryft1_odbcd";
const char _pidfile[]=".r1odbcd.pid";

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

vector<__catalog_entry__> __catalog;

typedef struct __column {
    unsigned    _ordinal;
    string      _colName;
    unsigned    _bufferLen;
    int         _type_num;
    int         _type_text;
    int         _type_int;
} __column;
typedef vector<__column> __columns;

class GuessColumn : public XMLParser, public JSONParser {
public:
    GuessColumn( ) : m_count(0) { }

    // XML Parse
    virtual NodeAction StartRow( );
    virtual NodeAction AddElement( std::string sName, const char **psAttribs, XMLElement **ppElement );
    virtual void AddText( std::string sText );

    // JSON Parse 
    virtual NodeAction JSONStartRow( );
    virtual NodeAction JSONAddElement( std::string sName, const char **psAttribs, JSONElement **ppElement );
    virtual void JSONAddText( std::string sText );

    __columns& getCols( ) { return m_cols; }

private:
    int             m_count;
    std::string     m_delim;

    __columns       m_cols;
    int             m_colIdx;

    vector<string>  m_strings;
};

NodeAction GuessColumn::JSONStartRow()
{
    return StartRow();
}
NodeAction GuessColumn::StartRow()
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

NodeAction GuessColumn::JSONAddElement(std::string sName, const char **psAttribs, JSONElement **ppElement)
{
    return AddElement(sName, NULL, NULL);
}
NodeAction GuessColumn::AddElement(std::string sName, const char **psAttribs, XMLElement **ppElement)
{
    if(m_count == 1) {
        __column col = {m_colIdx + 1, sName, 0, 0, 0, 0 }; 
        m_cols.push_back(col);
        m_strings.push_back("");
    }
    m_colIdx++;
    return ProcessNode;
}

void GuessColumn::JSONAddText(std::string sText)
{
    AddText(sText);
}
void GuessColumn::AddText(std::string sText) 
{
    if(m_strings.size() > m_colIdx)
        m_strings[m_colIdx] += sText;
}

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
    __pid_t pid = -1;
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
    int lfp = open(pidfile, O_RDWR|O_CREAT|O_TRUNC, 0640);
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
    remove_directory(s_R1Results);
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
    mkdir(s_R1Catalog, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
    mkdir(s_R1Results, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 

    // make directory containing bin current directory
    chdir(epath.c_str());
    putenv("LD_LIBRARY_PATH=../../lib/x8664");
    if(execl(path.c_str(), _exenam, (char *)NULL)) {
        cerr << "error starting ryft1_odbcd daemon \"" << path << "\" (" << errno << ")\n";
    }
}

void load()
{
    DIR *d;
    struct dirent *dir;
    __catalog.clear();
    if(d = opendir(s_R1Catalog)) {
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

void add(string filespec)
{
    string in;
    string delim;
    string name;
    char path[PATH_MAX];
    char rdf_path[PATH_MAX];
    char rdf_glob[PATH_MAX];
    int chunk = 64;
    __rdf_config__::DataType data_type = __rdf_config__::dataType_None;

    int fd;
    struct stat sb;
    fd = ::open(filespec.c_str(), O_RDONLY);
    if(fd == -1) {
        cerr << "Could not read input file \"" << filespec << "\" (" << errno << ")\n";
        return;
    }
    fstat(fd, &sb);
    bool hasTop = false;
    GuessColumn guess;
    if(guess.XMLSniff(fd, sb.st_size, delim)) {
        cout << "XML record delimiter [<" + delim + ">]: ";
        getline(std::cin, in);
        if(!in.empty())
            delim = in;

        data_type = __rdf_config__::dataType_XML;
        lseek(fd, 0, SEEK_SET);
        guess.XMLParse(fd, sb.st_size, delim, false);
    }
    else {
        data_type = __rdf_config__::dataType_JSON;
        bool hasTop = guess.JSONHasTop(fd, sb.st_size);
        if(!guess.JSONParse(fd, sb.st_size, !hasTop, "")) {
            cerr << "Could not read input file \"" << filespec << "\" (" << errno << ")\n";
            return;
        }
    }
    close(fd);

    strcpy(path, filespec.c_str());
    char *pstr = strrchr(path, '.');
    if(pstr)
        *pstr = '\0';
    name = path;
    cout << "Table name [" + name + "]: ";
    getline(std::cin, in);
    if(!in.empty())
        name = in;

    char str[16];
    sprintf(str, "%d", chunk);
    cout << "File chunk size [" + string(str) + "]: ";
    getline(std::cin, in);
    if(!in.empty())
        chunk = atoi(in.c_str());

    // make the ODBC directory if its not already there
    mkdir(s_R1Catalog, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
    strcpy(path, s_R1Catalog);
    strcat(path, "/");
    strcat(path, name.c_str());
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    __meta_config__ metaconfig;
    metaconfig.table_name = name;

    sprintf(rdf_path, "%s/%s.rdf", path, name.c_str());
    metaconfig.rdf_path = rdf_path;

    __columns::iterator itr;
    __meta_config__::__meta_col__ metacol;
    __columns cols = guess.getCols();
    for(itr = cols.begin(); itr != cols.end(); itr++) {
        metacol.xml_tag = itr->_colName;
        metacol.name = itr->_colName;
        metacol.type_def = getColumnType(*itr);
        metaconfig.columns.push_back(metacol);
    }
    metaconfig.write_meta_config(path);

    // create an rdf
    __rdf_config__ rdfconfig;

    sprintf(rdf_path, "%s/%s.rdf", path, name.c_str());
    sprintf(rdf_glob, "*.%s", name.c_str());
    rdfconfig.rdf_name = name;
    rdfconfig.data_type = data_type;
    rdfconfig.file_glob = rdf_glob;
    rdfconfig.chunk_size = chunk;
    rdfconfig.no_top = !hasTop;

    char tagEntry[PATH_MAX];
    sprintf(tagEntry, "<%s>", delim.c_str());
    rdfconfig.record_start = tagEntry;
    sprintf(tagEntry, "</%s>", delim.c_str());
    rdfconfig.record_end = tagEntry;

    __rdf_config__::__rdf_tag__ rdftag;
    for(itr = cols.begin(); itr != cols.end(); itr++) {
        rdftag.name = itr->_colName;
        sprintf(tagEntry, "<%s>", itr->_colName.c_str());
        rdftag.start_tag = tagEntry;
        sprintf(tagEntry, "</%s>", itr->_colName.c_str());
        rdftag.end_tag = tagEntry;
        rdfconfig.tags.push_back(rdftag);
    }
    rdfconfig.write_rdf_config(rdf_path);

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

    strcpy(path, s_R1Catalog);
    strcat(path, "/");
    strcat(path, __catalog[idx].meta_config.table_name.c_str());
    remove_directory(path);
}

void edit(string id)
{
    char path[PATH_MAX];
    int idx = atoi(id.c_str()) - 1;
    load();
    strcpy(path, s_R1Catalog);
    strcat(path, "/");
    strcat(path, __catalog[idx].meta_config.table_name.c_str());
    strcat(path, "/");
    strcat(path, s_TableMeta);
    char vi[PATH_MAX];
    sprintf(vi, "vi %s", path);
    system(vi);
}

void version(string exepath)
{
    char cat[PATH_MAX];
    size_t idx1 = exepath.rfind("/");
    string version(exepath.substr(0, idx1));
    version.append("/.version");
    sprintf(cat, "cat %s", version.c_str());
    system(cat);
}

void usage(string name)
{
    cout << "usage: " << name << " [opts] [args]\n";
    cout << "  opts:\n";
    cout << "  -a, --add\tAdd file as a new database (Syntax: -a source_file)\n";
    cout << "  -d, --del\tDelete installed database (Syntax: -d id)\n";
    cout << "  -e, --edit\tEdit database metadata (Syntax: -e id)\n";
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
            string filespec;
            if(i + 1 < argc) {
                filespec = argv[++i];
            }
            else usage(argv[0]);
            add(filespec);
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