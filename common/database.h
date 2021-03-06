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

#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <pqxx/pqxx>

class Database
{
public:
	Database(std::string connection_str);
	~Database();

        bool getResults(std::string query, pqxx::result * rs);
	
        int executeNonQuery(std::string query);
private:	
        pqxx::connection * m_conn;
        std::string m_connectionStr;
};

#endif // DATABASE_H
