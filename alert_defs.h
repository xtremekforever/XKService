
#ifndef ALERT_DEFS
#define ALERT_DEFS

/* All the alert codes for the GPRS */

// --- Non critical codes --- //
#define GPRS_STD_POSTITION_REPORT   0

// --- Yellow alert codes (medium severity) --- //
#define GPRS_EMERGENCY_REMOTE_CTL       1
#define GPRS_DOOR_OPENED                2
#define GPRS_UNIT_PARKED                3
#define GPRS_COMMAND_ACK                4
#define GPRS_UNIT_STOPPED               5
#define GPRS_DOOR_CLOSED                6
#define GPRS_POWER_DISCONNECTED         7
#define GPRS_POWER_CONNECTED            8
#define GPRS_VEHICLE_BATTERY_DISCHARGED 9
#define GPRS_BACKUP_BATTERY_CHARGED     10
#define GPRS_VEHICLE_BATTERY_CHARGED    11
#define GPRS_BACKUP_BATTERY_DISCHARGED  12
#define GPRS_SYSTEM_STARTUP             13
#define GPRS_ENGINE_ON                  14
#define GPRS_ENGINE_OFF                 15

// --- Red alert codes --- //
#define GPRS_HOOD_TRUNK_OPENED          16
#define GPRS_SHOCK_SENSOR_WARNING       17
#define GPRS_SHOCK_SENSOR_ALARM         18
#define GPRS_HIGH_VOLTAGE_CONSUMPTION   19
#define GPRS_TOW_TILT                   20
#define GPRS_OUTSIDE_PARKFENCE          21
#define GPRS_UNAUTHORISED_IGNITION      22
#define GPRS_GPS_TAMPERING              23
#define GPRS_SOS_ALERT                  69

// --- Alert Type Strings --- //
#define ALERT_TYPE_TIME_STR           "Time"
#define ALERT_TYPE_SPEEDING_STR       "Speeding"
#define ALERT_TYPE_GEOFENCE_STR       "Geofence"
#define ALERT_TYPE_EVENT_STR          "Event"
#define ALERT_TYPE_RED_STR            "Red Alert"


// --- ALert Type codes ---- //
#define ALERT_TYPE_NONE               0
#define ALERT_TYPE_TIME               1
#define ALERT_TYPE_SPEEDING           2
#define ALERT_TYPE_GEOFENCE           3
#define ALERT_TYPE_EVENT              4
#define ALERT_TYPE_RED                5

#endif
