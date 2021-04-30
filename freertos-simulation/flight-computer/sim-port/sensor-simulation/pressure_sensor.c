//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// pressure_sensor_bmp3.c
// UMSATS 2018-2020
//
// Repository:
//  UMSATS > Avionics 2019
//
// File Description:
//  Control and usage of BMP3 sensor inside of RTOS task.
//
// History
// 2019-04-06 Eric Kapilik
// - Created.
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// INCLUDES
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

#include "board/components/pressure_sensor.h"
#include <math.h>
#include <stdbool.h>
#include <board/board.h>

#include "protocols/SPI.h"
#include "core/system_configuration.h"
#include "protocols/UART.h"
#include "utilities/common.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "datafeeder.h"

#include "memory-management/memory_manager.h"
#include "event-detection/event_detector.h"


#define PRES_TYPE           0x200000
#define TEMP_TYPE           0x100000

#define GND_ALT             0
#define GND_PRES            101325

#define CONFIG_PRESSURE_SENSOR_DEFAULT_ODR	                     UINT8_C(0x02)
#define CONFIG_PRESSURE_SENSOR_DEFAULT_PRESSURE_OVERSAMPLING     UINT8_C(0x02)
#define CONFIG_PRESSURE_SENSOR_DEFAULT_TEMPERATURE_OVERSAMPLING  UINT8_C(0x02)
#define CONFIG_PRESSURE_SENSOR_DEFAULT_IIR_FILTER_COEFF          UINT8_C(0x04)

static QueueHandle_t s_queue;
static xTaskHandle handle;
static uint8_t dataNeedsToBeConverted = 0;
static uint8_t s_desired_processing_data_rate = 50;
static bool s_is_running = false;
static const struct pressure_sensor_configuration s_default_configuration = {

        .output_data_rate                               = CONFIG_PRESSURE_SENSOR_DEFAULT_ODR,
        .temperature_oversampling                       = CONFIG_PRESSURE_SENSOR_DEFAULT_TEMPERATURE_OVERSAMPLING,
        .pressure_oversampling                          = CONFIG_PRESSURE_SENSOR_DEFAULT_PRESSURE_OVERSAMPLING,
        .infinite_impulse_response_filter_coefficient   = CONFIG_PRESSURE_SENSOR_DEFAULT_IIR_FILTER_COEFF,
};

static struct pressure_sensor_configuration s_current_configuration = {

        .output_data_rate                               = CONFIG_PRESSURE_SENSOR_DEFAULT_ODR,
        .temperature_oversampling                       = CONFIG_PRESSURE_SENSOR_DEFAULT_TEMPERATURE_OVERSAMPLING,
        .pressure_oversampling                          = CONFIG_PRESSURE_SENSOR_DEFAULT_PRESSURE_OVERSAMPLING,
        .infinite_impulse_response_filter_coefficient   = CONFIG_PRESSURE_SENSOR_DEFAULT_IIR_FILTER_COEFF,
};




static void delay_ms( uint32_t period_ms );



int pressure_sensor_init( FlightSystemConfiguration * parameters )
{
    int status = spi2_init( );
    if ( status != SPI_OK )
    {
        return PRESS_SENSOR_ERR;
    }

    s_queue = xQueueCreate( 10, sizeof( PressureSensorData ) );
    if ( s_queue == NULL )
    {
        return PRESS_SENSOR_ERR;
    }

    vQueueAddToRegistry( s_queue, "bmp3_queue" );

    return PRESS_SENSOR_OK;
}



void prv_pressure_sensor_start( void * pvParameters )
{
    if(pvParameters != NULL)
    {
        FlightSystemConfiguration * systemConfiguration = ( FlightSystemConfiguration * ) pvParameters;
        dataNeedsToBeConverted = systemConfiguration->pressure_data_needs_to_be_converted;
    }

    bool result_flag;
    press_data cxx_press_data;
    /* Variable used to store the compensated data */
    PressureSensorData dataStruct;
    memset(&dataStruct, 0, sizeof(PressureSensorData));

    s_is_running = true;
    DEBUG_LINE("Pressure sensor task has been successfully started.");

    while ( s_is_running )
    {
        result_flag = datafeeder_get_press ( &cxx_press_data );
        if ( ! result_flag )
        {
            continue;
        }

        dataStruct.pressure     = cxx_press_data.pressure;
        dataStruct.temperature  = cxx_press_data.temperature;
        dataStruct.timestamp    = cxx_press_data.timestamp;

//        dataStruct.time_ticks   = xTaskGetTickCount() - time_start;

//        DISPLAY( "NEW PRESS data: %d\n", dataStruct.time_ticks);

        if(dataNeedsToBeConverted)
        {
            dataStruct.pressure = (dataStruct.pressure / 100);
        }

        pressure_sensor_add_measurement( &dataStruct );
        memset(&dataStruct, 0, sizeof(PressureSensorData));
    }

    DEBUG_LINE("Pressure sensor task has successfully exited.");
}

bool pressure_sensor_is_running     ()
{
    return s_is_running;
}

void pressure_sensor_stop           ()
{
    s_is_running = false;
}




int pressure_sensor_start( void * pvParameters )
{
    #if (userconf_FREE_RTOS_SIMULATOR_MODE_ON)
    #define MAKE_STR(x) _MAKE_STR(x)
    #define _MAKE_STR(x) #x
    #if (userconf_USE_COTS_DATA == 1)
        const char *CSV_FILE_PATH = MAKE_STR(COTS_CSV_FILE_PATH) ;
        data_feeder_start ( CSV_FILE_PATH );
    #else
        const char *CSV_FILE_PATH = MAKE_STR(SRAD_CSV_FILE_PATH) ;
        data_feeder_start ( CSV_FILE_PATH );
    #endif
    #endif


    if( ! s_is_running )
    {
        if ( pdFALSE == xTaskCreate(prv_pressure_sensor_start, "ps--manager", configMINIMAL_STACK_SIZE, pvParameters, 5, &handle) )
        {
            board_error_handler(__FILE__, __LINE__);
        }
    }
    else
    {
        DISPLAY_LINE("Pressure Sensor task is already running");
    }

    return PRESS_SENSOR_OK;
}



static void delay_ms ( uint32_t period_ms )
{
    vTaskDelay( ( TickType_t ) period_ms );
}



bool pressure_sensor_test ( void )
{
    return true;
}



bool pressure_sensor_read ( PressureSensorData * buffer )
{
    return pdPASS == xQueueReceive ( s_queue, buffer, 0 );
}


bool pressure_sensor_add_measurement ( PressureSensorData * _data )
{
    return pdTRUE == xQueueSend ( s_queue, _data, 0 );
}

int pressure_sensor_configure (PressureSensorConfiguration * parameters )
{
    if(parameters == NULL)
    {
        s_current_configuration = s_default_configuration;
        return PRESS_SENSOR_OK;
    }

    s_current_configuration = *parameters;
    return PRESS_SENSOR_OK;
}


PressureSensorConfiguration pressure_sensor_get_default_configuration()
{
    return s_default_configuration;
}

PressureSensorConfiguration pressure_sensor_get_current_configuration()
{
    return s_current_configuration;
}

void pressure_sensor_set_desired_processing_data_rate(uint32_t rate)
{
    s_desired_processing_data_rate = rate;
}



#ifdef __cplusplus
}
#endif
