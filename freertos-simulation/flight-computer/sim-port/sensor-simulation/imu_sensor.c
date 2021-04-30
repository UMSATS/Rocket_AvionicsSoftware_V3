//---------------------------------------------------------------------------------------------------------------------
// UMSATS 2018-2020
//
// Repository:
//  UMSATS Google Drive: UMSATS/Guides and HowTos.../Command and Data Handling (CDH)/Coding Standards
//
// File Description:
//  Reads sensor data for accelerometer and gyroscope from the BMI088
//  On prototype flight computer:
//            +Z is out of the board (perpendicular to board surface when on a table).
//            +X is towards the recovery circuit (away from where the battery connects).
//            +Y is towards the crystal (away from the programming header).
// History
// 2019-03-29 by Benjamin Zacharias
// - Created.
//---------------------------------------------------------------------------------------------------------------------

#include <board/board.h>
#include "board/components/imu_sensor.h"
#include "protocols/SPI.h"
#include "core/system_configuration.h"
#include "utilities/common.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "datafeeder.h"
#include "math.h"

#define INTERNAL_ERROR -127
#define ACC_TYPE                                        0x800000
#define GYRO_TYPE                                       0x400000

#define ACC_LENGTH                                      6 // Length of a accelerometer measurement in bytes.
#define GYRO_LENGTH                                     6 // Length of a gyroscope measurement in bytes.

#define CONFIG_IMU_SENSOR_DEFAULT_ACC_BANDWIDTH	        UINT8_C(0x02)
#define CONFIG_IMU_SENSOR_DEFAULT_ACC_ODR			    UINT8_C(0x08)
#define CONFIG_IMU_SENSOR_DEFAULT_ACC_RANGE		        UINT8_C(0x02)
#define CONFIG_IMU_SENSOR_DEFAULT_ACC_POWER			    UINT8_C(0x00)

#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_BANDWIDTH	    UINT8_C(0x04)
#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_ODR		        UINT8_C(0x04)
#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_RANGE		    UINT8_C(0x01)
#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_POWER		    UINT8_C(0x00)

static QueueHandle_t s_queue;
static xTaskHandle handle;
static uint8_t dataNeedsToBeConverted = 0;
static uint8_t s_desired_processing_data_rate = 50;
static bool s_is_running = false;

static const IMUSensorConfiguration s_default_configuration = {

        .accel_bandwidth                = CONFIG_IMU_SENSOR_DEFAULT_ACC_BANDWIDTH,
        .accel_output_data_rate         = CONFIG_IMU_SENSOR_DEFAULT_ACC_ODR,
        .accel_range                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_RANGE,
        .accel_power                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_POWER,

        .gyro_bandwidth                 = CONFIG_IMU_SENSOR_DEFAULT_GYRO_BANDWIDTH,
        .gyro_output_data_rate          = CONFIG_IMU_SENSOR_DEFAULT_GYRO_ODR,
        .gyro_range                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_RANGE,
        .gyro_power                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_POWER,
};

static IMUSensorConfiguration s_current_configuration = {

        .accel_bandwidth                = CONFIG_IMU_SENSOR_DEFAULT_ACC_BANDWIDTH,
        .accel_output_data_rate         = CONFIG_IMU_SENSOR_DEFAULT_ACC_ODR,
        .accel_range                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_RANGE,
        .accel_power                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_POWER,

        .gyro_bandwidth                 = CONFIG_IMU_SENSOR_DEFAULT_GYRO_BANDWIDTH,
        .gyro_output_data_rate          = CONFIG_IMU_SENSOR_DEFAULT_GYRO_ODR,
        .gyro_range                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_RANGE,
        .gyro_power                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_POWER,
};




//Wrapper functions for read and write
int8_t user_spi_read( uint8_t dev_addr, uint8_t reg_addr, uint8_t * data, uint16_t len );

int8_t user_spi_write( uint8_t dev_addr, uint8_t reg_addr, uint8_t * data, uint16_t len );

void delay_ms( uint32_t period );

static float imu_sensor_acc2g                ( int16_t acc_value );
static float imu_sensor_g2acc                ( float g );
static float imu_sensor_rot2deg_per_sec      ( int16_t gyro_value );
static float imu_sensor_deg_per_sec2rot      ( float deg_per_sec );


static float imu_sensor_acc2g ( int16_t acc_value )
{
    const int16_t range = pow (2, ( s_current_configuration.accel_range + 1 ) ) * 1.5;
    const float result = (float) acc_value / 32768  * range;
    return result;
}

static float imu_sensor_g2acc ( float g )
{
    const uint16_t  result = (int16_t ) ( g * 32768 / ( pow ( 2, ( s_current_configuration.accel_range + 1 ) ) * 1.5 ) );
    return result;
}

static float imu_sensor_rot2deg_per_sec ( int16_t gyro_value )
{
//    /**
//    * Registers containing the angular velocity sensor output. The sensor output is stored as signed 16-bit
//    * number in 2’s complement format in each 2 registers. From the registers, the gyro values can be
//    * calculated as follows:
//    * Rate_X: RATE_X_MSB * 256 + RATE_X_LSB
//    * Rate_Y: RATE_Y_MSB * 256 +
//    * Rate_Z: RATE_Z_MSB * 256 + RATE_Z_LSB
//    */
//
//    union
//    {
//        int16_t value;
//        uint8_t bytes[sizeof( int16_t )];
//    } container;
//
//    container.value = gyro_value;
//    int16_t actual_value = to_int16_t( container.bytes );

    const int16_t range = pow (2, ( s_current_configuration.gyro_range + 1 ) ) * 1.5;
    const float result = (float) gyro_value / 32768  * range;
    return result;
}

static float imu_sensor_deg_per_sec2rot     ( float deg_per_sec )
{
    return (int16_t ) ( deg_per_sec * 32768 ) / ( pow (2, (s_current_configuration.gyro_range + 1) ) * 1.5 );
}



int imu_sensor_init( FlightSystemConfiguration * parameters )
{

    int status = spi3_init( );
    if ( status != SPI_OK)
    {
        return IMU_ERR;
    }

    s_queue = xQueueCreate( 10, sizeof( IMUSensorData ) );
    if ( s_queue == NULL )
    {
        return IMU_ERR;
    }

    vQueueAddToRegistry( s_queue, "bmi088_queue" );

    return IMU_OK;
}


int imu_sensor_configure (IMUSensorConfiguration * parameters )
{
    if(parameters == NULL)
    {
        s_current_configuration = s_default_configuration;
        return IMU_OK;
    }

    s_current_configuration = *parameters;

    return IMU_OK;
}




void prv_imu_thread_start( void * param )
{
    if(param != NULL)
    {
        FlightSystemConfiguration * systemConfiguration = ( FlightSystemConfiguration * ) param;
        dataNeedsToBeConverted = systemConfiguration->imu_data_needs_to_be_converted;
    }

    //Get the parameters.
    IMUSensorData dataStruct;

    // main loop: continuously read sensor data
    // vTaskDelay(pdMS_TO_TICKS(100)); //Wait so to make sure the other tasks have started.


    bool result_flag;
    xyz_data cxx_data;

    s_is_running = true;
    DEBUG_LINE("IMU sensor task has been successfully started.");

    while ( s_is_running )
    {

        result_flag = datafeeder_get_acc( &cxx_data );
        if ( !result_flag )
        {
            continue;
        }

        dataStruct.acc_x = cxx_data.x;
        dataStruct.acc_y = cxx_data.y;
        dataStruct.acc_z = cxx_data.z;

        result_flag = datafeeder_get_gyro( &cxx_data );
        if ( !result_flag )
        {
            continue;
        }

        dataStruct.gyro_x = cxx_data.x;
        dataStruct.gyro_y = cxx_data.y;
        dataStruct.gyro_z = cxx_data.z;


        dataStruct.timestamp   = xTaskGetTickCount ( );

//        dataStruct.time_ticks   = time_start - xTaskGetTickCount();
//        DISPLAY( "NEW IMU data: %d\n", dataStruct.time_ticks);

        if(dataNeedsToBeConverted)
        {
            dataStruct.acc_x = imu_sensor_acc2g(dataStruct.acc_x);
            dataStruct.acc_y = imu_sensor_acc2g(dataStruct.acc_y);
            dataStruct.acc_z = imu_sensor_acc2g(dataStruct.acc_z);

            dataStruct.gyro_x = imu_sensor_rot2deg_per_sec(dataStruct.gyro_x);
            dataStruct.gyro_y = imu_sensor_rot2deg_per_sec(dataStruct.gyro_y);
            dataStruct.gyro_z = imu_sensor_rot2deg_per_sec(dataStruct.gyro_z);
        }

        imu_add_measurement( &dataStruct );
    }

    DEBUG_LINE("IMU sensor task has successfully exited.");
}



int imu_sensor_start ( void * const param )
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

    if( ! s_is_running ) {
        if (pdFALSE == xTaskCreate(prv_imu_thread_start, "imu-manager", configMINIMAL_STACK_SIZE, param, 5, &handle)) {
            board_error_handler(__FILE__, __LINE__);
        }
    }
    else
    {
        DISPLAY_LINE("IMU Sensor task is already running");
    }

    return IMU_OK;
}

bool imu_sensor_is_running     ()
{
    return s_is_running;
}

void imu_sensor_stop           ()
{
    s_is_running = false;
}


bool imu_read ( IMUSensorData * buffer )
{
    return pdPASS == xQueueReceive( s_queue, buffer, 0 );
}


void delay_ms( uint32_t period )
{
    vTaskDelay ( pdMS_TO_TICKS( period ) ); // wait for the given amount of milliseconds
}



int imu_sensor_test( )
{
    return IMU_ERR;
}



bool imu_add_measurement( IMUSensorData * _data )
{
    return pdTRUE == xQueueSend ( s_queue, _data, 0 );
}


IMUSensorConfiguration
imu_sensor_get_default_configuration()
{
    return s_default_configuration;
}

IMUSensorConfiguration
imu_sensor_get_current_configuration()
{
    return s_current_configuration;
}


void imu_sensor_set_desired_processing_data_rate(uint32_t rate)
{
    s_desired_processing_data_rate = rate;
}
