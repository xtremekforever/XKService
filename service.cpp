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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include <time.h>

#include "service.h"
#include "alert_defs.h"
#include "database.h"
#include "resource.h"
#include "log.h"
#include "utils.h"

using namespace std;
using namespace pqxx;


int mtos(int minutes)
{
    return minutes * 60;
}

int Service::_startup = 0;
Service::Service(bool debug)
    : debug_mode(debug)
{
    log = new Log("/var/log/xtremekservice.log", SERVICE_NAME, debug_mode);

    try {
        _db = new Database(DEFAULT_CONNECTION_STRING);
        _rgdb = new Database(DEFAULT_RG_CONN_STRING);

        m_time = time(NULL) + mtos(SERVICE_CLEANUP_INTERVAL);

        // Get a list of companies
        if (!_db->getResults("select comID from tblCompany", &m_companies)) {
            cerr << "Error reading companies from database!" << endl;
            exit(-1);
        }
    } catch (string ex) {
        log->write(ex);
    }

}

Service::~Service()
{
    delete _db;
    delete _rgdb;
}

void Service::setDebugMode(bool debug)
{
    debug_mode = debug;
    log->setDebugMode(debug);
}

int Service::execute()
{
    try
    {
        // If 5 minutes have elapsed, clear any invalid records.
        if (time(NULL) - m_time >= 0) {
            clearInvalidRecords();
            m_time = time(NULL) + mtos(SERVICE_CLEANUP_INTERVAL);
        }

        // Iterate through all the companies
        for (result::size_type i = 0; i < m_companies.size(); i++) {
            _comID = m_companies[i]["comID"].c_str();
            result rs;
            if (!_db->getResults("select deviceID,unitID,unitName from tblUnits where comID = " + _comID, &rs)) {
                cerr << "Error retrieving device list!" << endl;
                return -1;
            }

            // Iterate through all the units
            for (result::size_type j = 0; j < rs.size(); j++) {
                string deviceID = rs[j]["deviceID"].c_str();
                string unitID = rs[j]["unitID"].c_str();
                string unitName = rs[j]["unitName"].c_str();

               // std::cout << deviceID << "," << unitID << "," << unitName << std::endl;

                result records = getLastRecordGroup(deviceID, unitID);

                // If we've got some records, check events, speeding, and geofences....
                if (records.size() > 0) {
                    checkEventStatus(deviceID, unitID, unitName, records);
                    checkGeofenceStatus(deviceID, unitID, unitName, records);
                    checkSpeedingStatus(deviceID, unitID, unitName, records);
                    // Update the processed flag
                    _db->executeNonQuery("update tblGPRS set processed = true where deviceID = " + deviceID + " and added = '" + records[0]["added"].c_str() + "';");
                }
            }
        }
    }
    catch (string ex)
    {
        log->write("execute(): " + ex);
    }
}

void Service::clearInvalidRecords()
{
    result rs;
    string sql = "from tblGPRS where recTimeRevised > now() + interval '1 day'";
    sql.append(" or lat = 0.0 or long = 0.0 or deviceID = 0");

    _db->getResults("select * " + sql, &rs);

    if (rs.size() > 0) {
        cout << "Found " << rs.size() << " invalid records, deleting...";
        _db->executeNonQuery("delete " + sql);
        cout << "done!" << endl;
    }
}

// Update the status of the unit specified by deviceID and unitID.
// Note: This function does not check if deviceID and unitID match
// in the units table, so make sure they they match before calling
// this function.
int Service::checkEventStatus(string deviceID, string unitID, string unitName, result records)
{
    result rs = records;
    bool alert = false;
    string msgBody, alertTime;
    int code;

    // Iterate through the records to get alerts
    for (result::size_type i = 0; i < rs.size(); i++) {
        // Get message code and alert time
        msgBody = rs[i]["msgBody"].c_str();
        alertTime = rs[i]["recTimeRevised"].c_str();

        // If there is no alert message then return -1 (no message)
        if (msgBody.length() < 1) {
            return -1;
        }

        // Remove the first # character...
        code = atoi(msgBody.erase(0, 1).c_str());
        if (code != -1 && code != -2) {
            alert = true;
        }

        if (alert) {
            int alertType;
            string alertMessage;

            getEventMessage(unitID, unitName, &alertType, &alertMessage, code);

            if (alertType != ALERT_TYPE_NONE) {
                if (saveAlert(unitID, getAlertStr(alertType), alertMessage, alertTime) == 0) {
                    log->write("Got code #" + Utils::itos(code) + (string)", alert time " + alertTime +
                                ", deviceID = " + deviceID);
                    sendEventEmail(unitID, alertType, alertMessage, alertTime);
                }
            }
        }
    }
}

int Service::checkGeofenceStatus(string deviceID, string unitID, string unitName, result records)
{
    result rs = records;
    result geofences;
    string query = "select coalesce(geofenceID, 0) as geofenceID from tblUnitWiseGeofence" +
                   (string)" where unitID = " + unitID + " and isActive='1'";

    // Get list of geofences geofence(s)
    _db->getResults(query, &geofences);
    // No geofences for this unit, don't bother....
    if (geofences.size() < 1) {
        return 0;
    }

    for (result::size_type i = 0; i < geofences.size(); i++) {
        result geofence;
        string geofenceID = geofences[i]["geofenceID"].c_str();
        query = "select centerLat,centerLng,radius,name,email from tblGeofence" + 
                (string)" where comID = " + _comID + " and id = " + geofenceID;

        // Get geofence info (such as area, name, contact, etc)
        _db->getResults(query, &geofence);
        if (geofence.size() < 1) {
            continue;
        }

        // Get the current geofence status to compare with the new status
        bool geofenceStatus = getGeofenceStatus(geofenceID, unitID);

        // Check all the records to see if the unit has left the geofence at all
        for (result::size_type j = 0; j < rs.size(); j++) {
            string alertMsg;
            bool inside;

            inside = isInside(rs[j]["lat"].c_str(), rs[j]["long"].c_str(), 
                              geofence[0]["centerLat"].c_str(), geofence[0]["centerLng"].c_str(),
                              geofence[0]["radius"].c_str());

            // If the status hasn't changed, then ignore this record
            if (!isNewGeofenceStatus(geofenceStatus, inside)) {
                continue;
            }

            geofenceStatus = inside;

            // If there's a new geofence status, create a message here
            if (geofenceStatus) {
                alertMsg = "Unit " + unitName + " is inside of \"" + geofence[0]["name"].c_str() 
                            + "\""; 
            } else {
                alertMsg = "Unit " + unitName + " is outside of \"" + geofence[0]["name"].c_str() 
                            + "\""; 
            }

            log->write(alertMsg);

            // Get the alert time, save it to the alerts table, and send a geofence alert email....
            string alertTime = rs[j]["recTimeRevised"].c_str();
            saveAlert(unitID, getAlertStr(ALERT_TYPE_GEOFENCE), alertMsg, alertTime);
            sendGeofenceEmail(unitID, geofenceID, alertMsg, alertTime);

            // Update the geofence status
            query = "update tblUnitWiseGeofence set isInside = '" + Utils::btos(geofenceStatus) + 
                    (string)"' where unitID = " + unitID + " and geofenceID = " + geofenceID;
            _db->executeNonQuery(query);
        }
    }
}

void Service::checkSpeedingStatus(string deviceID, string unitID, string unitName, result records)
{
    result rs = records;
    string alertTime, alertMessage;
    string speedLimit;
    bool speedingStatus;

    speedLimit = getSpeedLimit(unitID);

    if (atoi(speedLimit.c_str()) <= 0) {
        return;
    }

    // Get the previous speeding status...
    speedingStatus = getSpeedingStatus(unitID);


    for (result::size_type i = 0; i < rs.size(); i++) {
        string speed;
        bool speeding = false;

        speed = rs[i]["velocity"].c_str();

        // Check if the unit is speeding....
        if (atoi(speed.c_str()) > atoi(speedLimit.c_str())) {
            speeding = true;
        }

        // If the unit status hasn't changed (it hasn't stopped speeding or hasn't speeded)
        // then just exit.
        if (!isNewSpeedingStatus(speedingStatus, speeding)) {
            continue;
        }


        speedingStatus = speeding;
        // If there is a speeding status change, create a message to show the user
        if (speedingStatus) {
            alertMessage = "Unit " + unitName + " is over speed limit " +
                                  speedLimit + " mph";
        } else {
            alertMessage = "Unit " + unitName + " is within speed limit " + 
                            speedLimit + " mph";
        }

        log->write(alertMessage);
            
        // Get alert time, save the alert, and email to user
        string alertTime = rs[i]["recTimeRevised"].c_str();
        saveAlert(unitID, ALERT_TYPE_SPEEDING_STR, alertMessage, alertTime);


        // Get reverse geocoding info
        result rg = findNearestAddress(rs[i]["lat"].c_str(), rs[i]["long"].c_str());
        if (rg.size() > 0) {
            // Send email with reverse geocoding info
            sendSpeedingEmail(deviceID, unitName, alertMessage, speed, 
                              rg[0]["placename"].c_str(), rg[0]["adminname2"].c_str(), 
                              rg[0]["adminname1"].c_str(), rg[0]["countryCode"].c_str(),
                                                                        alertTime);
        } else {
            sendSpeedingEmail(deviceID, unitName, alertMessage, speed, "", "", "", "",
                                                                        alertTime);
        }

        // Update the speeding status
        _db->executeNonQuery("update tblUnitWiseRules set isSpeeding = '" + Utils::btos(speedingStatus) + 
                                  (string)"' where unitID = " + unitID);
    }
}

int Service::getEventMessage(string unitID, string unitName, int * type, 
                             string * message, int code)
{
    result rs;

    int alertType;
    string alertMsg;

    // Prefix the message with Unit <unitname>
    alertMsg = "Unit " + unitName;
    
    // Create the alert type and message
    switch (code)
    {
    case GPRS_STD_POSTITION_REPORT:
        return ALERT_TYPE_NONE;
    case GPRS_EMERGENCY_REMOTE_CTL:
        alertType = ALERT_TYPE_RED;
        alertMsg += ": Emergency button pressed";
        break;
    case GPRS_DOOR_OPENED:
        alertType = ALERT_TYPE_RED;
        alertMsg += ": Door opened";
        break;
    case GPRS_UNIT_PARKED:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += " is parked";
        break;
    case GPRS_COMMAND_ACK:
        return ALERT_TYPE_NONE;
    case GPRS_UNIT_STOPPED:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += " just stopped";
        break;
    case GPRS_DOOR_CLOSED:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += ": Door closed";
        break;
    case GPRS_POWER_DISCONNECTED:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += ": Main power disconnected";
        break;
    case GPRS_POWER_CONNECTED:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += ": Main power connected";
        break;
    case GPRS_VEHICLE_BATTERY_DISCHARGED:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += ": Vehicle battery discharged";
        break;
    case GPRS_BACKUP_BATTERY_CHARGED:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += ": Backup battery fully charged";
        break;
    case GPRS_VEHICLE_BATTERY_CHARGED:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += ": Vehicle battery fully charged";
        break;
    case GPRS_BACKUP_BATTERY_DISCHARGED:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += ": Backup battery discharged";
        break;
    case GPRS_SYSTEM_STARTUP:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += ": System startup";
        break;
    case GPRS_ENGINE_ON:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += " turned on engine";
        break;
    case GPRS_ENGINE_OFF:
        alertType = ALERT_TYPE_EVENT;
        alertMsg += " turned off engine";
        break;
    case GPRS_HOOD_TRUNK_OPENED:
        alertType = ALERT_TYPE_RED;
        alertMsg += " hood/trunk opened";
        break;
    case GPRS_SHOCK_SENSOR_WARNING:
        alertType = ALERT_TYPE_RED;
        alertMsg += ": Shock sensor warning (soft impact)";
        break;
    case GPRS_SHOCK_SENSOR_ALARM:
        alertType = ALERT_TYPE_RED;
        alertMsg += ": Shock sensor alert (hard impact)";
        break;
    case GPRS_HIGH_VOLTAGE_CONSUMPTION:
        alertType = ALERT_TYPE_RED;
        alertMsg += ": High voltage consumption detected";
        break;
    case GPRS_TOW_TILT:
        alertType = ALERT_TYPE_RED;
        alertMsg += " was raised/tilted (or towed!)";
        break;
    case GPRS_OUTSIDE_PARKFENCE:
        alertType = ALERT_TYPE_RED;
        alertMsg += " is outside of parkfence";
        break;
    case GPRS_UNAUTHORISED_IGNITION:
        alertType = ALERT_TYPE_RED;
        alertMsg += ": Unauthorized ignition";
        break;
    case GPRS_GPS_TAMPERING:
        alertType = ALERT_TYPE_RED;
        alertMsg += ": GPS Tampering Detected";
        break;
    case GPRS_SOS_ALERT:
        alertType = ALERT_TYPE_RED;
        alertMsg += ": SOS Button Pressed!";
    }
    
    *type = alertType;
    *message = alertMsg;

    return alertType;
}

bool Service::getGeofenceStatus(string geofenceID, string unitID)
{

    result rs;
    string query = "select coalesce(isInside,'0') as isInside from tblUnitWiseGeofence" + 
                   (string)" where unitID = " + unitID + " and geofenceID = " + geofenceID;

    _db->getResults(query, &rs);

    bool inside = false;

    if (rs.size() > 0) {
        rs[0]["isInside"].to(inside);
    }

    return inside;
}

bool Service::isInside(string lat, string lng, string centerLat, string centerLng, string radius)
{
    double distance;

    distance = Utils::calculateDistance(atof(lat.c_str()), atof(lng.c_str()), 
                                 atof(centerLat.c_str()), atof(centerLng.c_str()));
    if (distance < atof(radius.c_str())) {
        return true;
    } else {
        return false;
    }
}

// Checks whether the unit has entered or exited a geofence
bool Service::isNewGeofenceStatus(bool wasInside, bool isInside)
{
    if (wasInside) {                                // If we were inside,
        if (!isInside) {                            // And we're no longer inside,
            return true;                            // The status has changed....
        }
    } else {                                        // If we were NOT inside,
        if (isInside) {                             // But we are now,
            return true;                            // The status has changed...
        }
    }
    return false;                                   // Otherwise, no status change...
}

string Service::getSpeedLimit(string unitID) {
    result rs;
    string query = "select coalesce(rulesID, 0) as rulesID from tblUnitWiseRules where" +
                   (string)" unitID = " + unitID + " and isActive = '1'";

    _db->getResults(query, &rs);

    if (rs.size() > 0) {
        // Find a speeding rule for this unit
        for (int i = 0; i < rs.size(); i++) {
            string rulesID = rs[i]["rulesID"].c_str();
            string rulesQuery = "select f.rulesFor,r.rulesOperator,r.rulesValue from tblRules r inner join" +
                                (string)" tblRulesFor f on f.rulesForID = r.rulesForID where rulesID = " + 
                                rulesID + " and f.rulesFor = 'Speed';";

            result rules;     
            // Get the Speeding rule for the current ruleID
            _db->getResults(rulesQuery, &rules);

            for (int j = 0; j < rules.size(); j++) {
                string rulesOperator = rules[j]["rulesOperator"].c_str();
                // Check if the unit is speeding
                if (rulesOperator == "<") {
                    return rules[j]["rulesValue"].c_str();
                }
            }
        }
    }

    return "0";
}

bool Service::getSpeedingStatus(string unitID)
{
    result rs;
    string query = "select coalesce(isSpeeding,'0') as isSpeeding from tblUnitWiseRules" +
                   (string)" where unitID = " + unitID;

    bool speeding = false;

    _db->getResults(query, &rs);
    if (rs.size() > 0) {
        rs[0]["isSpeeding"].to(speeding);
    }

    return speeding;
}

bool Service::isNewSpeedingStatus(bool wasSpeeding, bool isSpeeding)
{
    if (wasSpeeding) {                              // If the unit was speeding,
        if (!isSpeeding) {                          // And it stopped speeding,
            return true;                            // We've got a new speeding status....
        }
    } else {                                       // If the unit wasn't speeding,
        if (isSpeeding) {                          // But it's speeding now,
            return true;                           // We've got a new speeding status
        }
    }

    return false;
}

result Service::getLastRecord(string deviceID, string unitID) 
{
    string query = "select unitName,lat,long," + 
                   (string)"cast(velocity * 0.621 as int) as velocity,recTimeRevised from" + 
                   (string)" vwUnitRecords where comID = " + _comID + " and unitID = " + unitID + 
                   (string)" and deviceID = " + deviceID + " order by recTime desc limit 1;";
                   
    result rs;
    _db->getResults(query, &rs);
    return rs;
}

result Service::getLastRecordGroup(string deviceID, string unitID)
{
    string query = "select * from fn_get_last_alert(" + deviceID + ");";

    result rs;
    _db->getResults(query, &rs);
    return rs;
}

string Service::getAlertStr(int alertType)
{
    switch (alertType)
    {
    case ALERT_TYPE_NONE:
        return "";
    case ALERT_TYPE_TIME:
        return ALERT_TYPE_TIME_STR;
    case ALERT_TYPE_SPEEDING:
        return ALERT_TYPE_SPEEDING_STR;
    case ALERT_TYPE_GEOFENCE:
        return ALERT_TYPE_GEOFENCE_STR;
    case ALERT_TYPE_EVENT:
        return ALERT_TYPE_EVENT_STR;
    case ALERT_TYPE_RED:
        return ALERT_TYPE_RED_STR;
    default:
        return NULL;
    }
}

// Insert an alert into the alerts table. To prevent duplicate entries, this first checks
// the table to make sure that the alert hasn't been inserted yet. Then it inserts it.
int Service::saveAlert(string unitID, string alertType, string alertMessage, 
                       string alertTime)
{
    result rs;
    string query = "select * from tblAlert where alertType = '" + alertType + "' and alertTime = '" + 
                   alertTime + "' and comID = " + _comID + " and unitID = " + unitID;
                         
    // Check to see if the entry already exists to prevent duplicate entries
    _db->getResults(query, &rs);
    if (rs.size() < 1) {
        // Insert it into the table
        query = "insert into tblAlert(alertType, alertMessage, alertTime, comID, unitID) VALUES('" + 
            alertType + "','" + alertMessage + "','" + alertTime + "'," + _comID + "," + unitID + ")";
        _db->executeNonQuery(query);
    } else {
        return 1;
    }
    
    return 0;
}

void Service::sendEventEmail(string unitID, int alertType, string alertMsg, string alertTime)
{
    result rs;
    string query;
    // Check to see if the user wants to be notified about the alert
    if (alertType == ALERT_TYPE_RED) {
        query = "select email,phoneNumber,coalesce(isSMS,'0') as isSMS" +
                (string)" from tblAlertRules where unitID = " + unitID + " and comID = " + _comID + 
                " and isActive='1'";
        _db->getResults(query, &rs);
        if (rs.size() > 0) {
            vector<string> emails = Utils::split((string)rs[0]["email"].c_str(), ",");

            // Send to all the included addresses
            for (int i = 0; i < emails.size(); i++) {
                if (emails[i].length() > 0) {
                    sendMail(Utils::trim(emails[i]), alertMsg, alertMsg);
                }
            }

            bool isSMS;
            rs[0]["isSMS"].to(isSMS);
            // If SMS is enabled, send an SMS message as well
            if (isSMS ) {
                sendSMS(rs[0]["phoneNumber"].c_str(), alertMsg);
            }
        }
    }
}

void Service::sendGeofenceEmail(string unitID, string geofenceID, string alertMsg, 
                                                                string alertTime)
{
    result rs;
    string query = "select email,phoneNumber,coalesce(isSMS,'0') as isSMS from tblGeofence" + 
                   (string)" where id = " + geofenceID + " and comID = " + _comID +
                   " and isActive = '1'";

    _db->getResults(query, &rs);
    if (rs.size() > 0) {
        vector<string> emails = Utils::split((string)rs[0]["email"].c_str(), ",");

        // Send to all the included addresses
        for (int i = 0; i < emails.size(); i++) {
            if (emails[i].length() > 0) {
                sendMail(emails[i], alertMsg, alertMsg);
            }
        }

        bool isSMS;
        rs[0]["isSMS"].to(isSMS);
        if (isSMS) {
            sendSMS(rs[0]["phoneNumber"].c_str(), alertMsg);
        }
    }
}

void Service::sendSpeedingEmail(string unitID, string unitName, string alertMsg, string speed, string city, 
                                string county, string state, string country, string alertTime)
{
    result rs;
    string alertMessage;
    string message;
    string query = "select email,speedPhoneNum as phoneNumber,description," +
                   (string)"coalesce(isSMS, '0') as isSMS from tblUnitWiseRules where unitID = " + unitID +
                   " and isActive = '1';";
               
    _db->getResults(query, &rs);
    
    if (rs.size() > 0) {
        if (strlen(rs[0]["description"].c_str()) > 0)  {
            message = rs[0]["description"].c_str();
        } else {
            message = alertMsg;
        }

        alertMessage = "Unit ID: " + unitID + "\n";
        alertMessage += "Unit Name: " + unitName + "\n";
        alertMessage += "Message: " + message + "\n";
        alertMessage += "Speed: " + speed + "\n";
        alertMessage += "City: " + city + "\n";
        alertMessage += "County: " + county + "\n";
        alertMessage += "State: " + state + "\n";
        alertMessage += "Country: " + country + "\n";
        alertMessage += "Reported Time: " + alertTime + "\n";
        // TODO: CONVERT TO UTC TIME AND ADD AS "Universal Time: "

        vector<string> emails = Utils::split((string)rs[0]["email"].c_str(), ",");

        // Send to all the included addresses
        for (int i = 0; i < emails.size(); i++) {
            if (emails[i].length() > 0) {
                sendMail(emails[i], alertMsg, alertMessage);
            }
        }
                
        bool isSMS;
 
        rs[0]["isSMS"].to(isSMS);
        if (isSMS) {
            sendSMS(rs[0]["phoneNumber"].c_str(), alertMsg);
        }
    }
}

result Service::findNearestAddress(string latitude, string longitude)
{
    string query = "select * from findNearestAddress(" + latitude + "," + longitude + ");";


    result rs;

    try
    {
        // Find the nearest address
        _rgdb->getResults(query, &rs);

        return rs;
    }
    catch (string ex)
    {
        log->write("Exception in findNearestAddress(): " + ex);
    }

    return rs;
}

// Sends a mail message using the mstmp client. This code is not really portable
// since it relies on the system() command to run the client.
// --- Format ---
// To: <email>
// Subject: <subject>
// <Message>
void Service::sendMail(string email, string subject, string msg)
{
    string msgStr = "";
    string cmdStr;

    msgStr += "To: " + email + "\n";
    msgStr += "Subject: " + subject + "\n";
    msgStr += msg + "\n";
    
    cmdStr = "msmtp -t <<EOT\n" + msgStr + "EOT";

    log->write("Sending email to: " + email);
    //log->write(cmdStr);

    system(cmdStr.c_str());
}

// Sends an SMS
void Service::sendSMS(string phoneNumber, string msg)
{
    Database smsd_db("hostaddr=192.168.12.132 port=5432 user=smsd password=smsd0864 dbname=smsd");

    log->write("Sending SMS message to: " + phoneNumber);

    string str = "select * from sendsms('" + phoneNumber + "', '" + msg + "', '" SERVICE_NAME "');";
    smsd_db.executeNonQuery(str);
}
