#include <iostream>
#include <fstream>
#include <pqxx/pqxx>
#include <signal.h>
#include <boost/timer.hpp>

#include <err.h>

#include "service.h"

#define EXECUTABLE_NAME    "xtremek-service"

using namespace std;

void print_version()
{
    char hostname[256];

    gethostname(hostname, 256);

    cout << "XKService v0.0.1 on " << hostname << endl;
}

int main(int argc, char *argv[])
{
    bool debug_mode = false;

    signal(SIGPIPE, SIG_IGN);

    // Check the command line arguments
    if (argc > 0) {
        for (int i = 0; i < argc; i++) {
            string cmd = argv[i];
            if (cmd == "-h" || cmd == "--help") {
                cout << "Usage: " << APPLICATION_NAME << " [OPTION]" << endl;
                cout << "Command Line Options: " << endl;
                cout << " -h, --help\tshow this help" << endl;
                cout << " -d, --debug\trun in debug mode (don't run as a service)" << endl;
            } else if (cmd == "-d" || cmd == "--debug") {
                debug_mode = true;
            } else if (cmd == "-v" || cmd == "--version") {
                print_version();
            }
        }
    }

    if (!debug_mode) {
        if(daemon(0,0) == -1)
            err(1, NULL);
    }

    print_version();

    Service service(debug_mode);

    boost::timer mytimer;

    while (1) 
    {
        service.execute();
        sleep(1);
    }

    return 0;
}
