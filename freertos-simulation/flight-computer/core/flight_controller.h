#ifndef DATA_LOGGING_H
#define DATA_LOGGING_H

typedef enum FlightState FlightState;
typedef struct DataContainer DataContainer;

typedef enum { FLIGHT_CONTROLLER_ERR   = 0, FLIGHT_CONTROLLER_OK    = 1 } FlightControllerStatus;

FlightControllerStatus flight_controller_init  ( void * pvParams );
FlightControllerStatus flight_controller_start ( );
void flight_controller_stop();


FlightControllerStatus flight_state_machine_init ( FlightState state );
FlightControllerStatus flight_state_machine_tick ( FlightState state, DataContainer * data );

#endif // DATA_LOGGING_H
