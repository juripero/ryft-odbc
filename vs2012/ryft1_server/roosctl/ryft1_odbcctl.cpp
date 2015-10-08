#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

const char _exenam[]="ryft1_odbcd";
const char _pidfile[]=".r1odbcd.pid";

__pid_t get_lock_file( )
{
    int lfp = open(_pidfile, O_RDONLY);
    if(lfp < 0) 
        return -1;
    char str[10];
    __pid_t pid;
    read(lfp, str, sizeof(str));
    sscanf(str, "%d", &pid);
    close(lfp);
    return pid;
}

void put_lock_file(__pid_t pid)
{
    int lfp = open(_pidfile, O_RDWR|O_CREAT, 0640);
    if(lfp < 0) 
        exit(1);
    char str[10];
    sprintf(str, "%d", getpid());
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

void stop()
{
    __pid_t pid = get_lock_file();
    if((pid != -1) && is_process_running(pid)) {
        cout << "killing process " << pid << "...";
        kill(pid, 15);
        while(is_process_running(pid)) {
            cout << ".";
            sleep(1);
        }
        cout << "done.\n";
    }
}

void start(string path)
{
    __pid_t pid = get_lock_file();
    if((pid != -1) && is_process_running(pid)) {
        cout << "ryft1_odbcd already running, restart now (yes/no)? ";
        string in;
        getline(std::cin, in);
        if(in != "y" && in != "yes") 
            return;
        stop();
    }
    if(path.empty()) 
        path = "./" + string(_exenam);

    cout << "starting ryft1_odbcd...\n";
    background();
    putenv("LD_LIBRARY_PATH=../lib");
    put_lock_file(getpid());
    if(execl(path.c_str(), _exenam, (char *)NULL)) {
        cerr << "error starting ryft1_odbcd daemon \"" << path << "\" (" << errno << ")\n";
    }
}

void usage(string name)
{
    cout << "usage: " << name << " [opts] [arg]\n";
    cout << "\topts:\n";
    cout << "\t-h, --help\t\tdisplay help\n";
    cout << "\t-k, --kill\t\tkill Ryft ODBC daemon\n";
    cout << "\t-s, --start\t\tstart Ryft ODBC daemon\n";
}

int main(int argc, char *argv[])
{
    if(argc < 2) {
        usage(argv[0]);
        return 1;
    }
    for(int i = 1; i < argc; i++) {
        string arg = argv[i];
        if((arg == "-h") || (arg == "--help")) {
            usage(argv[0]);
            return 0;
        }
        else if((arg == "-s") || (arg == "--start")) {
            string path;
            if(i + 1 < argc) 
                path = argv[++i];
            start(path);
        }
        else if((arg == "-k") || (arg == "--kill")) {
            stop();
        }
        else {
            cerr << "invalid argument " << argv[i] << "\n\n";
            usage(argv[0]);
            return 1;
        }
    }
 	return 0;
}