#ifndef __EVENT_DETECTOR_H
#define __EVENT_DETECTOR_H

#include <stdbool.h>
#include <stdint-gcc.h>


// Detection Parameters
struct DataContainer;
typedef struct FlightSystemConfiguration FlightSystemConfiguration;

typedef enum FlightState
{
    FLIGHT_STATE_LAUNCHPAD   = 0,
    FLIGHT_STATE_PRE_APOGEE  = 1,
    FLIGHT_STATE_APOGEE      = 2,
    FLIGHT_STATE_POST_APOGEE = 3,
    FLIGHT_STATE_MAIN_CHUTE  = 4,
    FLIGHT_STATE_POST_MAIN   = 5,
    FLIGHT_STATE_LANDED      = 6,
    FLIGHT_STATE_EXIT        = 7,
    FLIGHT_STATE_COUNT
} FlightState;

typedef enum EventDetectorStatus
{
    EVENT_DETECTOR_ERR = 0,
    EVENT_DETECTOR_OK = 1
} EventDetectorStatus;


EventDetectorStatus event_detector_init ( FlightSystemConfiguration * configurations );

EventDetectorStatus event_detector_update_configurations ( FlightSystemConfiguration * configurations );

EventDetectorStatus event_detector_feed ( struct DataContainer * data, FlightState * state );

float event_detector_current_altitude ( );

bool event_detector_is_flight_started ( );

#endif