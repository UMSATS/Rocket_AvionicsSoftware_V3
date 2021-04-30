//
// Created by vasil on 07/02/2021.
//

#include "system_configuration.h"

static bool prvIsInitialized = false;

FlightSystemConfiguration DEFAULT_FLIGHT_SYSTEM_CONFIGURATION = {0};


FlightSystemConfiguration get_default_system_configuration ( )
{
    if ( prvIsInitialized == false )
    {
        // TODO: to be edited from GUI
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.landing_rotation_speed_deg_per_sec      = 0;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.backup_time_launch_to_apogee_sec        = 27;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.backup_time_apogee_to_main_sec          = 116;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.backup_time_main_to_ground_sec          = 191;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.e_match_line_keep_active_for            = 5;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.launch_acceleration_critical_value_m_s2 = 7 /*6.9*/    ;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.altitude_main_recovery_m                = 381;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.ground_pressure                         = 0;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.ground_temperature                      = 0;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.imu_data_needs_to_be_converted          = 0;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.pressure_data_needs_to_be_converted     = 0;
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.imu_sensor_configuration                = imu_sensor_get_default_configuration ( );
        DEFAULT_FLIGHT_SYSTEM_CONFIGURATION.pressure_sensor_configuration           = pressure_sensor_get_default_configuration ( );
        prvIsInitialized = true;

    }

    return DEFAULT_FLIGHT_SYSTEM_CONFIGURATION;
}