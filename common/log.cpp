/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <iostream>
#include <fstream>
#include <syslog.h>

#include "log.h"

using namespace std;

ofstream m_logfile;
    
Log::Log(string logfile, string serviceName, bool debug)
    : m_logfile_str(logfile),
      m_serviceName(serviceName),
      m_debug(debug)
{
    m_isOpen = false;
    
    //gethostname(_hostname, HOSTNAME_BUFSIZE);
   // _pid = getpid();

    //open(logfile);
    openlog (m_serviceName.c_str(), LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);


}

Log::~Log()
{
    close();
}

void Log::setDebugMode(bool debug)
{
    m_debug = debug;
    //open(m_logfile_str);
}

void Log::open(string logfile)
{
    if (!m_isOpen && !m_debug) {
        // Open the file to append to it
        m_logfile.open(logfile.c_str(), ios::app);
        
        m_isOpen = true;
    }
}

void Log::close()
{
    closelog();
    //m_logfile.close();
}

/*string getCurrentTime() 
{
    string timeStr;
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];


    time (&rawtime);  
    timeinfo = localtime ( &rawtime );

    sprintf(buffer, "%d  %d %d:%d:%d", timeinfo->tm_mday, timeinfo->);

    timeStr = buffer;
    return timeStr;
}*/

void Log::write(string msg)
{
    if (m_debug) {        
        cout << msg << endl;
    } else {
        syslog (LOG_INFO, "%s", msg.c_str());

       // m_logfile << getCurrentTime() << " " << _hostname << " " << SERVICE_NAME << "[" << _pid << "]: " << msg << endl;
    }
}
