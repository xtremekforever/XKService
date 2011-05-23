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

#ifndef SERVICE_H
#define SERVICE_H

#include <iostream>
#include <fstream>
#include <pqxx/pqxx>

#define SERVICE_CLEANUP_INTERVAL 5

#define SERVICE_NAME        "xtremek-service"

class Database;
class Log;

class Service
{
public:
	Service(bool debug = false);
	~Service();
	
	int execute();
    
    void setDebugMode(bool debug);

protected:
    void clearInvalidRecords();

    // --- Unit Event Functions --- //
	//int checkEventStatus(std::string deviceID, std::string unitID, 
    //                                          std::string unitName);
    int checkEventStatus(std::string deviceID, std::string unitID, 
                         std::string unitName, pqxx::result records);
    //int getLastEventCode(std::string deviceID, std::string * alertTime);
    int getEventMessage(std::string unitID, std::string unitName, 
                        int * type, std::string * message,
                        int code);
    void sendEventEmail(std::string unitID, int alertType, std::string alertMsg, 
                   std::string alertTime);
                        
    // --- Unit Geofence Functions --- //
    int checkGeofenceStatus(std::string deviceID, std::string unitID, 
                       std::string unitName, pqxx::result records);
    bool getGeofenceStatus(std::string geofenceID, std::string unitID);
    bool isInside(std::string lat, std::string lng, std::string centerLat, 
                  std::string centerLng, std::string radius);
    bool isNewGeofenceStatus(bool wasInside, bool isInside);
    void sendGeofenceEmail(std::string unitID, std::string geofenceID, std::string alertMsg, 
                   std::string alertTime);
                       
    // --- Unit Speeding Functions --- //
    void checkSpeedingStatus(std::string deviceID, std::string unitID, 
                       std::string unitName, pqxx::result records);
    std::string getSpeedLimit(std::string unitID);
    bool getSpeedingStatus(std::string unitID);
    //bool isSpeeding(std::string unitID, std::string speed, std::string * speedLimit);
    bool isNewSpeedingStatus(bool wasSpeeding, bool isSpeeding);
    void sendSpeedingEmail(std::string unitID, std::string unitName, std::string alertMsg, 
                           std::string speed, std::string city, std::string county, 
                           std::string state, std::string country, std::string alertTime);
    
    
    // --- Generic Alert Functions --- //
    pqxx::result getLastRecord(std::string deviceID, std::string unitID);
    pqxx::result getLastRecordGroup(std::string deviceID, std::string unitID);
    std::string getAlertStr(int alertType);
    int saveAlert(std::string unitID, std::string alertType,
                  std::string alertMessage, std::string alertTime);
                  
                  
    pqxx::result findNearestAddress(std::string latitude, std::string longitude);

    // --- Mail and SMS Functions --- //
    void sendMail(std::string email, std::string subject, std::string msg);
    void sendSMS(std::string phoneNumber, std::string msg);
	
private:
     static int _startup;

    Database * _db;
    Database * _rgdb;
    Log * log;

    bool debug_mode;

    pqxx::result m_companies;
    
    // This service checks units in groups of companies, so this contains
    // the ID of the current company
    std::string _comID;

    time_t m_time;
};


#endif // SERVICE_H
