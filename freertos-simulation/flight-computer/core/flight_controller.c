#include "flight_controller.h"
#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include "board/components/recovery.h"

#include "board/board.h"
#include "memory-management/memory_manager.h"
#include "event-detection/event_detector.h"
#include "configurations/UserConfig.h"

#include "board/components/imu_sensor.h"
#include "board/components/pressure_sensor.h"
#include "utilities/common.h"



#if (userconf_FREE_RTOS_SIMULATOR_MODE_ON == 1)
#include "sim-port/sensor-simulation/datafeeder.h"
#endif


static int prvLastRecoverContinuityStatus  [ RecoveryContinuityStatusCount  ] = { RecoveryContinuityStatusOpenCircuit,    RecoveryContinuityStatusOpenCircuit    };
static int prvLastRecoverOverCurrentStatus [ RecoveryOverCurrentStatusCount ] = { RecoveryOverCurrentStatusNoOverCurrent, RecoveryOverCurrentStatusNoOverCurrent };


typedef struct
{
    uint8_t isInitialized       ;
    uint8_t isRunning           ;

    xTaskHandle taskHandle      ;
    void * taskParameters       ;
}FlightControllerState;

static FlightControllerState prvTaskState     = {};


static void prv_flight_controller_task(void * pvParams);
static void get_sensor_data_update(DataContainer * data);
FlightControllerStatus flight_controller_init(void * pvParams)
{
    prvTaskState.taskParameters = pvParams;
    prvTaskState.isInitialized = 1;

    return FLIGHT_CONTROLLER_OK;
}


FlightControllerStatus flight_controller_start()
{
    if ( !prvTaskState.isInitialized )
    {
        return FLIGHT_CONTROLLER_ERR;
    }

//    TaskHandle_t xTask = controller.taskHandle;
//    TaskStatus_t pxTaskStatus;
//    BaseType_t xGetFreeStackSpace = 0;
//    eTaskState eState = eInvalid;
//    vTaskGetInfo(xTask, &pxTaskStatus, xGetFreeStackSpace, eState);


    if ( pdFALSE == xTaskCreate( prv_flight_controller_task, "fl--manager", configMINIMAL_STACK_SIZE, prvTaskState.taskParameters, 5, &prvTaskState.taskHandle ) )
    {
        return FLIGHT_CONTROLLER_ERR;
    }

    return FLIGHT_CONTROLLER_OK;
}

void flight_controller_stop()
{
    prvTaskState.isRunning = 0;
}



static void prv_flight_controller_task(void * pvParams)
{
    (void) pvParams;

    static FlightSystemConfiguration system_configurations   = {0};
    static MemoryManagerConfiguration memoryConfigurations   = {0};

    prvTaskState.isRunning = 1;

    //------------------------------ EDIT SYSTEM CONFIGURATIONS ------------------------------------------------------//
    if ( memory_manager_get_system_configurations ( &system_configurations ) != MEM_OK )
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "System configurations have been extracted.");
    }

    // Now if there is any need we can edit the system configurations to adjust them to our needs
    //
    // Otherwise we leave them unchanged
    //----------------------------------------------------------------------------------------------------------------//



    //------------------------------ EDIT MEMORY CONFIGURATIONS ------------------------------------------------------//
    if ( memory_manager_get_memory_configurations ( &memoryConfigurations ) != MEM_OK)
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "Memory configurations have been extracted.");
    }
    // Now if there is any need we can edit the memory configurations to adjust them to our needs
    //
    // Otherwise we leave them unchanged
    //----------------------------------------------------------------------------------------------------------------//


#if (userconf_FREE_RTOS_SIMULATOR_MODE_ON == 1)
    #if (userconf_USE_COTS_DATA == 0)
        status = memory_manager_get_system_configurations( &system_configurations );
        if ( status != MEM_OK )
        {
            board_error_handler( __FILE__, __LINE__ );
        } else
        {
            DISPLAY_LINE( "System configurations have been extracted");
        }

        systemConfigurationsU.values = system_configurations;
        if(common_is_mem_not_set(systemConfigurationsU.bytes, sizeof(FlightSystemConfiguration)))
        {
            // if memory read returned empty configurations
            system_configurations.imu_sensor_configuration      = imu_sensor_get_default_configuration();
            system_configurations.pressure_sensor_configuration = pressure_sensor_get_default_configuration();
        }

        system_configurations.imu_data_needs_to_be_converted       = 1;
        system_configurations.pressure_data_needs_to_be_converted  = 1;

        status = memory_manager_set_system_configurations( &system_configurations );
        if ( status != MEM_OK )
        {
            board_error_handler( __FILE__, __LINE__ );
        } else
        {
            DISPLAY_LINE( "System configurations have been extracted");
        }

        memoryConfigurationsU.values = memoryConfigurations;
        if(common_is_mem_not_set(systemConfigurationsU.bytes, sizeof(MemoryManagerConfiguration)))
        {
            // if memory read returned empty configurations
            memoryConfigurations = memory_manager_get_default_memory_configurations();
        }

        imu_sensor_start      ( &system_configurations );
        pressure_sensor_start ( &system_configurations );

    #else
        if ( ! imu_sensor_configure ( &system_configurations.imu_sensor_configuration ) )
        {
            board_error_handler( __FILE__, __LINE__ );
        } else
        {
            DEBUG_LINE( "IMU sensor has been configured.");
        }

        if ( ! pressure_sensor_configure ( &system_configurations.pressure_sensor_configuration ) )
        {
            board_error_handler( __FILE__, __LINE__ );
        } else
        {
            DEBUG_LINE( "Pressure sensor has been configured.");
        }

         imu_sensor_start      ( NULL );
         pressure_sensor_start ( NULL );
    #endif
#else
    system_configurations.imu_data_needs_to_be_converted       = 1;
    system_configurations.pressure_data_needs_to_be_converted  = 1;

    if(IMU_OK != imu_sensor_configure(&system_configurations.imu_sensor_configuration))
    {
        board_error_handler( __FILE__, __LINE__ );
    }
    else
    {
        DEBUG_LINE( "IMU sensor has been configured.");
    }

    if(PRESS_SENSOR_OK != pressure_sensor_configure(&system_configurations.pressure_sensor_configuration))
    {
        board_error_handler( __FILE__, __LINE__ );
    }
    else
    {
        DEBUG_LINE( "Pressure sensor has been configured.");
    }

    imu_sensor_start      ( &system_configurations );
    pressure_sensor_start ( &system_configurations );
#endif

    if ( event_detector_init ( &system_configurations ) != EVENT_DETECTOR_OK )
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "Event Detector has been set & configured.");
    }

    // in case of reboot during the flight if memory is not corrupted event_detector_init should change this flag to the
    // appropriate stage that != FLIGHT_STATE_LAUNCHPAD
    if( ! event_detector_is_flight_started () )
    {
        // this will only be triggered in case if we are on the ground before the flight

        // # 1 set the ground pressure and temperature as references for the future calculations
        DEBUG_LINE( "Flight Controller: waiting for the ground pressure & temperature...");
        PressureSensorData initialGroundPressureData;
        while (!pressure_sensor_read(&initialGroundPressureData));
        DEBUG_LINE( "Flight Controller: ground pressure & temperature have been set!");

        system_configurations.ground_pressure    = initialGroundPressureData.pressure;
        system_configurations.ground_temperature = initialGroundPressureData.temperature;

        // # 1 set the flight state to launchpad, grab the current tick count that is ~ 0;
        // set the starting power mode that is supposed to be low power mode
//        system_configurations.last_current_system_time = xTaskGetTickCount(); // TODO: this probably will have to go to a separate misc. data sector
//        system_configurations.last_power_mode = 1; // TODO: so as this

        // updating the algorithm's data
        event_detector_update_configurations(&system_configurations);

        // write these configurations to the memory
        if (memory_manager_set_system_configurations ( &system_configurations ) != MEM_OK)
        {
            board_error_handler(__FILE__, __LINE__);
        }
        else
        {
            DEBUG_LINE("Memory configurations have been updated.");
        }
    }


    if ( FLIGHT_CONTROLLER_OK != flight_state_machine_init( FLIGHT_STATE_LAUNCHPAD ) )
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "Flight State Machine has been set.");
    }

    static DataContainer flightData = {0};
    static FlightState flightState;

    prvTaskState.isRunning    = 1;

    int start_time  = 0;
    int last_time   = 0;
    int seconds     = 0;
    while ( prvTaskState.isRunning )
    {
//        flightData.timestamp = xTaskGetTickCount ( ) - start_time;

        get_sensor_data_update(&flightData);

        event_detector_feed ( &flightData, &flightState );

        flight_state_machine_tick ( flightState, &flightData );

        memory_manager_user_data_update( &flightData );

        memset(&flightData, 0, sizeof(DataContainer));

        if ( ( xTaskGetTickCount() - last_time ) / configTICK_RATE_HZ >= 1 )
        {
            seconds++;
//            DEBUG_LINE( "Flight Time: %d sec", seconds);
            last_time = (xTaskGetTickCount() - start_time);
            DEBUG_LINE ( "CURRENT ALTITUDE : %f", event_detector_current_altitude () );
        }
    }

    prvTaskState.isRunning = false;

}


static void get_sensor_data_update ( DataContainer * data )
{
    IMUSensorData      imu_data;
    PressureSensorData pressure_data;

    if ( imu_read ( &imu_data ) )
    {
        data->acc.data.values.timestamp = imu_data.timestamp;
        memcpy ( &data->acc.data.values.data, &imu_data.acc_x, sizeof ( float ) * 3 );

        data->gyro.data.values.timestamp = imu_data.timestamp;
        memcpy ( &data->gyro.data.values.data, &imu_data.gyro_x, sizeof ( float ) * 3 );
        data->gyro.updated = true;
        data->acc.updated  = true;
    }

    if ( pressure_sensor_read ( &pressure_data ) )
    {
        data->press.data.values.timestamp = pressure_data.timestamp;
        data->press.data.values.data      = pressure_data.pressure;
        data->press.updated               = true;

        data->temp.data.values.timestamp   = pressure_data.timestamp;
        data->temp.data.values.data        = pressure_data.temperature;
        data->temp.updated                 = true;
    }
}

void prvCheckRecoveryStatusAndNotifyIfChanged ( DataContainer * data )
{
    for ( RecoverySelect recovery = RecoverSelectDrogueParachute; recovery < RecoverySelectCount; recovery++ )
    {
        RecoveryContinuityStatus currentContinuityStatus = recoveryCheckContinuity  ( recovery );

        if ( prvLastRecoverContinuityStatus [ recovery ] !=  currentContinuityStatus )
        {
            prvLastRecoverContinuityStatus [ recovery ] = currentContinuityStatus ;

            data->cont.updated = true;
            data->cont.data.values.timestamp = xTaskGetTickCount ( );
            data->cont.data.values.status [ recovery ] = prvLastRecoverContinuityStatus [ recovery ];

        }
    }
}




//....................................................................................................................//
//......................................State Machine Implementation..................................................//
//....................................................................................................................//

typedef struct
{
    FlightState state;
    int (*function)(DataContainer *);
} state_machine_type;

uint8_t isInitialized = 0;

int sm_STATE_LAUNCHPAD   (DataContainer*);
int sm_STATE_PRE_APOGEE  (DataContainer*);
int sm_STATE_APOGEE      (DataContainer*);
int sm_STATE_POST_APOGEE (DataContainer*);
int sm_STATE_MAIN_CHUTE  (DataContainer*);
int sm_STATE_POST_MAIN   (DataContainer*);
int sm_STATE_LANDED      (DataContainer*);
int sm_STATE_EXIT        (DataContainer*);

state_machine_type state_machine[] =
{
        {FLIGHT_STATE_LAUNCHPAD,        sm_STATE_LAUNCHPAD     },
        {FLIGHT_STATE_PRE_APOGEE,       sm_STATE_PRE_APOGEE    },
        {FLIGHT_STATE_APOGEE,           sm_STATE_APOGEE        },
        {FLIGHT_STATE_POST_APOGEE,      sm_STATE_POST_APOGEE   },
        {FLIGHT_STATE_MAIN_CHUTE,       sm_STATE_MAIN_CHUTE    },
        {FLIGHT_STATE_POST_MAIN,        sm_STATE_POST_MAIN     },
        {FLIGHT_STATE_LANDED,           sm_STATE_LANDED        },
        {FLIGHT_STATE_EXIT,             sm_STATE_EXIT          }
};

static FlightState sm_state;

FlightControllerStatus flight_state_machine_init ( FlightState state )
{
    sm_state = state;

    isInitialized = 1;

    return FLIGHT_CONTROLLER_OK;
}



FlightControllerStatus flight_state_machine_tick ( FlightState state, DataContainer * data )
{
    if ( data == NULL )
    {
        return FLIGHT_CONTROLLER_ERR;
    }

    if ( !isInitialized )
    {
        DISPLAY_LINE("Flight Controller has not been initialized.");
        return FLIGHT_CONTROLLER_ERR;
    }

    // Check to make sure that the state is being entered is valid
    if ( sm_state < FLIGHT_STATE_COUNT )
    {
        return state_machine [ state ].function ( data );
    }
    else
    {
        // Throw an exception
        return FLIGHT_CONTROLLER_ERR;
    }
}


int sm_STATE_LAUNCHPAD ( DataContainer * data )
{
    return 0;
}

int sm_STATE_PRE_APOGEE ( DataContainer * data )
{
    prvCheckRecoveryStatusAndNotifyIfChanged ( data );

    return 0;
}

int sm_STATE_APOGEE ( DataContainer * data )
{
    recoveryEnableMOSFET ( RecoverSelectDrogueParachute );
    recoveryActivateMOSFET ( RecoverSelectDrogueParachute );


    prvCheckRecoveryStatusAndNotifyIfChanged ( data );

    return 0;
}

int sm_STATE_POST_APOGEE ( DataContainer * data )
{
    prvCheckRecoveryStatusAndNotifyIfChanged ( data );
    return 0;
}

int sm_STATE_MAIN_CHUTE ( DataContainer * data )
{
    recoveryEnableMOSFET ( RecoverySelectMainParachute );
    recoveryActivateMOSFET ( RecoverySelectMainParachute );

    prvCheckRecoveryStatusAndNotifyIfChanged ( data );

    return 0;
}

int sm_STATE_POST_MAIN ( DataContainer * data )
{
    prvCheckRecoveryStatusAndNotifyIfChanged ( data );

    return 0;
}

int sm_STATE_LANDED ( DataContainer * data )
{
    prvCheckRecoveryStatusAndNotifyIfChanged ( data );

    return 0;
}

int sm_STATE_EXIT ( DataContainer * data )
{

    return 0;
}




















