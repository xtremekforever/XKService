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

#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <sstream>
#include <algorithm>
#include <math.h>
#include <string>
#include <vector>

class Utils
{
public:
    Utils() {};
    
    // Convert a integer to a string
    static std::string itos(int i)
    {
        std::string s;
        std::stringstream out;
        out << i;
        s = out.str();

        return s;
    }

    // Convert a boolean to a string
    static std::string btos(bool b)
    {
        std::string s;
        std::stringstream out;
        out << b;
        s = out.str();

        return s;
    }

    static std::vector<std::string> split(const std::string& str, 
                                            const std::string& delimiters)
    {
        std::vector<std::string> tokens;
        std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
        std::string::size_type pos = str.find_first_of(delimiters, lastPos);

        while (std::string::npos != pos || std::string::npos != lastPos)
        {
            // Found a token, add it to the vector.
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            // Skip delimiters.  Note the "not_of"
            lastPos = str.find_first_not_of(delimiters, pos);
            // Find next "non-delimiter"
            pos = str.find_first_of(delimiters, lastPos);
        }

        return tokens;
    }

    static std::string trim(const std::string & source, const std::string & t = " ")
    {
        std::string str = source;

        // Trim on left side
        str = str.erase(0, source.find_first_not_of(t));
        // Trim on right side
        str = str.erase(str.find_last_not_of(t) + 1);

        return str;
    }
    
    static const double PI = 3.14159265358979323846;

    static double toRadian(double val) 
    { 
        //return val * (3.1415926535897932384626433832795 / 180); 
        return val * (PI / 180); 
    }

    static double diffRadian(double val1, double val2) 
    { 
        return toRadian(val2) - toRadian(val1); 
    }

    static const double EarthRadiusInMiles = 3956.0;
    static const double EarthRadiusInKilometers = 6367.0;

    static double calculateDistance(double lat1, double lng1, double lat2, double lng2)
    {
        double earthRadius = EarthRadiusInKilometers;

        return earthRadius * 2 * asin(std::min(1.0, sqrt((pow(sin((diffRadian(lat1, lat2)) / 2.0), 2.0) + 
                            cos(toRadian(lat2)) * pow(sin((diffRadian(lng1, lng2)) / 2.0), 2.0)))));
    }
};

#endif // DATABASE_H
