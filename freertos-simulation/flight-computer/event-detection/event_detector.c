#include "event_detector.h"

#include <FreeRTOS.h>
#include <task.h>

#include <math.h>
#include <inttypes.h>
#include <stdbool.h>
#include "utilities/common.h"
#include "configurations/UserConfig.h"
#include "memory-management/memory_manager.h"

#include "protocols/UART.h"
#include "data_window.h"

#define CRITICAL_VERTICAL_ACCELERATION  6.9 // [g]
#define APOGEE_ACCELERATION             0.1 // [g]
#define MAIN_CHUTE_ALTITUDE             381 // [m] (converted from 1,250ft)
#define LANDING_ROTATION_SPEED          5   // [deg / s]

#define ALTITUDE_SENSITIVITY_THRESHOLD  25

static FlightState prvFlightState;

static float CURRENT_ALTITUDE          = 0;
static uint32_t GROUND_PRESSURE        = 0;
static float GROUND_ALTITUDE           = 0;
static int   INITIALIZED               = 0;

static int   AVERAGE_PRESSURE_SAMPLING_RATE = 0;
static int   AVERAGE_IMU_SAMPLING_RATE      = 0;

static int prvEventDelayCounter             = 0;
static int prvDELAY_MS                      = 1000;

typedef struct
{
    IMUDataU      inertial;
    PressureDataU pressure;
} FlightData;


#if ( userconf_EVENT_DETECTION_AVERAGING_SUPPORT_ON == 1 )
static moving_data_buffer altitude_data_window, vertical_acc_data_window;
static float mean ( float * array, size_t length, size_t start, size_t end );
#endif


static float prvCalculateAltitude ( float pressure );
static bool prvDetectLaunch    ( float vertical_acceleration_in_g );
static bool prvDetectApogee    ( float acceleration_x_in_g, float acceleration_y_in_g, float acceleration_z_in_g );
static bool prvDetectAltitude  ( float target_altitude, uint32_t ground_pressure, uint32_t current_pressure );
static bool prvDetectLanding   ( float gyro_x_in_deg_per_sec, float gyro_y_in_deg_per_sec, float gyro_z_in_deg_per_sec );

void prvMarkNewEvent ( DataContainer * data )
{
    data->event.updated               = true;
    data->event.data.values.status    = prvFlightState;
    data->event.data.values.timestamp = xTaskGetTickCount ( );
}


float event_detector_current_altitude ( )
{
    return CURRENT_ALTITUDE;
}

bool event_detector_is_flight_started ( )
{
    return prvFlightState != FLIGHT_STATE_LAUNCHPAD;
}


EventDetectorStatus event_detector_init ( FlightSystemConfiguration * configurations )
{
    if ( INITIALIZED == 1 )
    {
        DISPLAY_LINE( "Event Detector has been already initialized." );
        return EVENT_DETECTOR_OK;
    }

    if ( configurations == NULL )
    {
        return EVENT_DETECTOR_ERR;
    }

    GROUND_PRESSURE = configurations->ground_pressure;
    GROUND_ALTITUDE = prvCalculateAltitude ( GROUND_PRESSURE );

    // in case of reboot during the flight if memory is not corrupted this flag should change to the
    // appropriate current flight stage that != FLIGHT_STATE_LAUNCHPAD
    FlightEventU lastFlightEventEntry = { 0 };
    if ( MEM_OK != memory_manager_get_last_data_entry ( MemoryUserDataSectorFlightEvent, lastFlightEventEntry.bytes ) )
    {
        return EVENT_DETECTOR_ERR;
    }

    if ( common_is_mem_empty ( lastFlightEventEntry.bytes, sizeof ( FlightEventU ) ) )
    {
        prvFlightState = FLIGHT_STATE_LAUNCHPAD;
    }
    else
    {
        prvFlightState = lastFlightEventEntry.values.status; // TODO: make sure that this is fixed
    }

#if ( userconf_EVENT_DETECTION_AVERAGING_SUPPORT_ON == 1 )
    data_window_init ( &altitude_data_window );
    data_window_init ( &vertical_acc_data_window );
#endif


    INITIALIZED = 1;
    return EVENT_DETECTOR_OK;
}


EventDetectorStatus event_detector_update_configurations ( FlightSystemConfiguration * configurations )
{
    if ( INITIALIZED == 0 )
    {
        DISPLAY_LINE( "CANNOT update configurations. Event Detector has not been initialized." );
        return EVENT_DETECTOR_ERR;
    }

    if ( configurations == NULL )
    {
        DISPLAY_LINE( "CANNOT update configurations. NullPointer." );
        return EVENT_DETECTOR_ERR;
    }

    GROUND_PRESSURE = configurations->ground_pressure;
    GROUND_ALTITUDE = prvCalculateAltitude ( GROUND_PRESSURE );

    return EVENT_DETECTOR_OK;
}


EventDetectorStatus event_detector_feed ( struct DataContainer * data, FlightState * flightState )
{
    if ( INITIALIZED == 0 )
    {
        DISPLAY_LINE( "CANNOT feed. Event Detector has not been initialized." );
        return EVENT_DETECTOR_ERR;
    }

    if ( data->press.updated )
    {
        CURRENT_ALTITUDE = prvCalculateAltitude ( data->press.data.values.data ) - GROUND_ALTITUDE;
#if ( userconf_EVENT_DETECTION_AVERAGING_SUPPORT_ON == 1 )
        data_window_insert ( &altitude_data_window, &CURRENT_ALTITUDE );
#endif
    }

    switch ( prvFlightState )
    {
        case FLIGHT_STATE_LAUNCHPAD:
        {
            if ( data->acc.updated )
            {
                if ( prvDetectLaunch ( data->acc.data.values.data[ 0 ] ) )
                {
                    DEBUG_LINE( "FLIGHT_STATE_LAUNCHPAD: Detected Launch!");
                    prvFlightState = FLIGHT_STATE_PRE_APOGEE;
                    *flightState = prvFlightState;
                    prvMarkNewEvent ( data );
                }
            }


            return EVENT_DETECTOR_OK;
        }

        case FLIGHT_STATE_PRE_APOGEE:
        {
            if ( data->acc.updated )
            {
#if ( userconf_EVENT_DETECTION_AVERAGING_SUPPORT_ON == 1 )
                // Here we need to start looking at the average altitude and see the differences in the gradient sign
                // the idea is that if the gradient changes the sign then we reached the apogee since the altitude
                // is now decreasing instead of increasing. Also to avoid mechanical errors if the sign changes
                // dramatically then it cannot represent the real world scenario, as in the real world the inertia
                // makes it stop really slowly as well as falling down, therefore the difference needs to be small.

                float previous_average_altitude = mean ( altitude_data_window.linear_repr, sizeof ( altitude_data_window.linear_repr ), 0, MOVING_BUFFER_RANGE - 1 );
                float last_average_altitude     = mean ( altitude_data_window.linear_repr, sizeof ( altitude_data_window.linear_repr ), 1, MOVING_BUFFER_RANGE );

                float difference          = last_average_altitude - previous_average_altitude;
                float absolute_difference = fabs ( fabs ( last_average_altitude ) - fabs ( previous_average_altitude ) );

                if ( ( difference < 0 ) && absolute_difference < 5 && absolute_difference > 0.2 )
                {
                    DEBUG_LINE( "FLIGHT_STATE_PRE_APOGEE: Detected APOGEE!" );
                    prvFlightState = FLIGHT_STATE_APOGEE;
                    *flightState = prvFlightState;
                    prvMarkNewEvent ( data );

                    prvEventDelayCounter = xTaskGetTickCount ();
                }
#else
                if ( prvDetectApogee( data->acc.data.values.data[ 0 ], data->acc.data.values.data[ 1 ],
                                      data->acc.data.values.data[ 2 ] ) )
                {
                    DISPLAY_LINE( "Detected APOGEE at %fm", CURRENT_ALTITUDE);
                    prvFlightState = FLIGHT_STATE_APOGEE;
                    *flightState = prvFlightState;
                    prvMarkNewEvent ( data );
                }
#endif

            }


            return EVENT_DETECTOR_OK;
        }
        case FLIGHT_STATE_APOGEE:
        {
            if ( xTaskGetTickCount () - prvEventDelayCounter >= pdMS_TO_TICKS ( prvDELAY_MS ))
            {
                DEBUG_LINE( "FLIGHT_STATE_APOGEE: Igniting recovery circuit, OPENING DROGUE PARACHUTE!" );

                prvFlightState = FLIGHT_STATE_POST_APOGEE;
                *flightState = prvFlightState;
                prvMarkNewEvent ( data );
            }


            return EVENT_DETECTOR_OK;
        }

        case FLIGHT_STATE_POST_APOGEE:
        {
            if ( data->press.updated )
            {
                if ( prvDetectAltitude ( MAIN_CHUTE_ALTITUDE, GROUND_ALTITUDE, data->press.data.values.data ) )
                {
//                    DEBUG_LINE( "FLIGHT_STATE_POST_APOGEE: Detected Main Chute!");

                    prvFlightState = FLIGHT_STATE_MAIN_CHUTE;
                    *flightState = prvFlightState;
                    prvMarkNewEvent ( data );

                    prvEventDelayCounter = xTaskGetTickCount ();
                }
            }

            return EVENT_DETECTOR_OK;
        }

        case FLIGHT_STATE_MAIN_CHUTE:
        {
            if ( xTaskGetTickCount () - prvEventDelayCounter >= pdMS_TO_TICKS ( prvDELAY_MS ))
            {
                DEBUG_LINE( "FLIGHT_STATE_MAIN_CHUTE: Igniting recovery circuit, OPENING MAIN PARACHUTE!" );

                prvFlightState = FLIGHT_STATE_POST_MAIN;
                *flightState = prvFlightState;
                prvMarkNewEvent ( data );
            }

            return EVENT_DETECTOR_OK;
        }

        case FLIGHT_STATE_POST_MAIN:
        {
            if ( data->gyro.updated )
            {
                if ( prvDetectLanding ( data->gyro.data.values.data[ 0 ], data->gyro.data.values.data[ 1 ], data->gyro.data.values.data[ 2 ] ) )
                {
                    DEBUG_LINE( "FLIGHT_STATE_POST_MAIN: Detected landing!" );

                    prvFlightState = FLIGHT_STATE_LANDED;
                    *flightState = prvFlightState;
                    prvMarkNewEvent ( data );

                    prvEventDelayCounter = xTaskGetTickCount ();
                    return EVENT_DETECTOR_OK;
                }
            }

            // OR

            if ( data->press.updated )
            {
                if ( prvDetectAltitude ( 0, GROUND_ALTITUDE, data->press.data.values.data ) )
                {
                    DEBUG_LINE( "FLIGHT_STATE_POST_MAIN: Detected landing!" );

                    prvFlightState = FLIGHT_STATE_LANDED;
                    *flightState = prvFlightState;
                    prvMarkNewEvent ( data );

                    prvEventDelayCounter = xTaskGetTickCount ();
                    return EVENT_DETECTOR_OK;
                }
            }

            return EVENT_DETECTOR_OK;
        }
        case FLIGHT_STATE_LANDED:
        {
            if ( xTaskGetTickCount () - prvEventDelayCounter >= pdMS_TO_TICKS ( prvDELAY_MS ))
            {
                DEBUG_LINE( "FLIGHT_STATE_LANDED: Rocket landed! Exiting the mission..." );

                prvFlightState = FLIGHT_STATE_EXIT;
                *flightState = prvFlightState;
                prvMarkNewEvent ( data );

                prvEventDelayCounter = xTaskGetTickCount ();
            }

            return EVENT_DETECTOR_OK;
        }

        case FLIGHT_STATE_EXIT:
        {

            if ( xTaskGetTickCount () - prvEventDelayCounter >= pdMS_TO_TICKS ( prvDELAY_MS ))
            {
                DEBUG_LINE( "FLIGHT_STATE_EXIT: Exit!" );

                prvFlightState = FLIGHT_STATE_COUNT;
                *flightState = prvFlightState;
                prvMarkNewEvent ( data );
            }

            return EVENT_DETECTOR_OK;
        }
        case FLIGHT_STATE_COUNT:
        default:
            return EVENT_DETECTOR_ERR;
    }
}


static bool prvDetectLaunch ( float vertical_acceleration_in_g )
{
    return vertical_acceleration_in_g > CRITICAL_VERTICAL_ACCELERATION;
}


static bool prvDetectApogee ( float acceleration_x_in_g, float acceleration_y_in_g, float acceleration_z_in_g )
{
    const float ACCELERATION_VECTOR = sqrt ( acceleration_x_in_g * acceleration_x_in_g + acceleration_y_in_g * acceleration_y_in_g + acceleration_z_in_g * acceleration_z_in_g );
    return ACCELERATION_VECTOR < APOGEE_ACCELERATION;
}


static bool prvDetectAltitude ( float target_altitude, uint32_t ground_pressure, uint32_t current_pressure )
{
    float current_altitude = prvCalculateAltitude ( current_pressure ) - ground_pressure;
    return fabsf ( ( current_altitude ) - ( target_altitude ) ) < ALTITUDE_SENSITIVITY_THRESHOLD;
}

static bool prvDetectLanding ( float gyro_x_in_deg_per_sec, float gyro_y_in_deg_per_sec, float gyro_z_in_deg_per_sec )
{
    float gyroscope_orientation_vector = sqrt ( gyro_x_in_deg_per_sec * gyro_x_in_deg_per_sec + gyro_y_in_deg_per_sec * gyro_y_in_deg_per_sec + gyro_z_in_deg_per_sec * gyro_z_in_deg_per_sec );
    return gyroscope_orientation_vector < LANDING_ROTATION_SPEED;
}


static float prvCalculateAltitude ( float pressure )
{
    /*
     Pb = static pressure (pressure at sea level) [Pa]
     Tb = standard temperature (temperature at sea level) [K]
     Lb = standard temperature lapse rate [K/m] = -0.0065 [K/m]
     h  = height about sea level [m]
     hb = height at the bottom of atmospheric layer [m]
     R  = universal gas constant = 8.31432
     q0 = gravitational acceleration constant = 9.80665
     M  = molar mass of Earth’s air = 0.0289644 [kg/mol]
    */

    static const float Pb = 101325.00f;
    static const float Tb = 15.00 + 273.15;
    static const float Lb = -0.0065;
    static const int   hb = 0;
    static const float R  = 8.31432;
    static const float g0 = 9.80665;
    static const float M  = 0.0289644;

    return hb + ( Tb / Lb ) * ( pow ( ( pressure / Pb ), ( -R * Lb ) / ( g0 * M ) ) - 1 );
}


#if ( userconf_EVENT_DETECTION_AVERAGING_SUPPORT_ON == 1 )

static float mean ( float * array, size_t length, size_t start, size_t end )
{
    assert( end < length );
    assert( start < end );

    float sum = 0;
    for ( size_t i   = start; i <= end; i++ )
    {
        sum += array[ i ];
    }

    return sum / ( end - start );
}

#endif





