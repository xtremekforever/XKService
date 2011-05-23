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
#include <string.h>
#include <iostream>
#include <pqxx/pqxx>

#include "database.h"

using namespace pqxx;

// Default constructor for Database class. Creates a new connection using the default
// connection string. TODO: Make the connection string configurable from a .cf file
Database::Database(std::string connection_str)
{
    m_connectionStr = connection_str;
    m_conn = new connection(m_connectionStr);
}

Database::~Database()
{
    m_conn->disconnect();
    delete m_conn;
}

bool Database::getResults(std::string query, result * rs)
{
    try {
        work transaction(*m_conn);

        if (rs) {
            *rs = transaction.exec(query);
        } else {
            transaction.exec(query);
        }
        transaction.commit();
    }  catch (const pqxx::sql_error &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << e.query() << std::endl;
        return false;
    } catch (const pqxx::broken_connection &e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
	
    return true;
}

int Database::executeNonQuery(std::string query)
{
    int rc = 0;
    result rs;

    if (!getResults(query, &rs)) {
        return -1;
    } else if (rs.size() > 0 && strlen(rs[0][0].c_str()) > 0) {
        rs[0][0].to(rc);
    }
	
    return rc;
}
