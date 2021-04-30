#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// UMSATS 2018-2020
//
// Repository:
//  UMSATS Google Drive: UMSATS/Guides and HowTos.../Command and Data Handling (CDH)/Coding Standards
//
// File Description:
//  Template header file for C / C++ projects. Unused sections can be deleted.
//
// History
// 2019-05-27 by Joseph Howarth
// - Created.
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <inttypes.h>

#include "board/components/imu_sensor.h"
#include "board/components/pressure_sensor.h"


// Defaults for the configuration options.
#define ID                                          0x5A

#define DATA_RATE                                   50
#define INITIAL_WAIT_TIME                           10000       // in milliseconds

#define GND_ALT                                     0
#define GND_PRES                                    101325


typedef struct FlightSystemConfiguration
{
    // 4 bytes
    uint32_t landing_rotation_speed_deg_per_sec;      // -
    uint32_t backup_time_launch_to_apogee_sec;        // -
    uint32_t backup_time_apogee_to_main_sec;          // -
    uint32_t backup_time_main_to_ground_sec;          // -
    uint8_t  e_match_line_keep_active_for;            // -
    uint8_t  launch_acceleration_critical_value_m_s2; // -
    uint16_t altitude_main_recovery_m;                // -
    float    ground_pressure;                         // -
    float    ground_temperature;                      // -
    uint8_t  imu_data_needs_to_be_converted;
    uint8_t  pressure_data_needs_to_be_converted;

    IMUSensorConfiguration      imu_sensor_configuration;
    PressureSensorConfiguration pressure_sensor_configuration;

//    uint32_t last_current_system_time;                // -
//    uint8_t last_flight_state;                        // -
//    uint8_t last_power_mode;                          // -

} FlightSystemConfiguration;

typedef union FlightSystemConfigurationU
{
    struct
    {
        FlightSystemConfiguration values;
    };
    uint8_t bytes[sizeof ( FlightSystemConfiguration )];
} FlightSystemConfigurationU;


// to be removed
typedef struct
{
    uint8_t     id;
    uint32_t    initial_time_to_wait;
    uint8_t     data_rate;
    uint8_t     flags;
    uint32_t    start_data_address;
    uint32_t    end_data_address;




    float       ref_alt;
    float       ref_pres;

    uint8_t     state;
} configuration_data_values;

FlightSystemConfiguration get_default_system_configuration ( ) ;






#endif // CONFIGURATION_H
