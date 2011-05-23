 
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

#ifndef LOG_H
#define LOG_H

#include <iostream>

class Log
{
public:
    /** Log class constructor. The logfile parameter specifies the file to log to. **/
    Log(std::string logfile, std::string serviceName, bool debug = false);
    /** Default log class destructor **/
    ~Log();
   
    void setDebugMode(bool debug);
    
    void write(std::string msg);
protected:
    void open(std::string logfile);
    void close();
    
private:    
    // If this is set to true, then output is sent to the stdout (or cout)
    bool m_debug;
    bool m_isOpen;
    
    std::string m_logfile_str;
    std::string m_serviceName;
    //char _hostname[HOSTNAME_BUFSIZE];
    //pid_t _pid;
};

#endif // LOG_H
