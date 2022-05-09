//-------------------------------------------------------------------------------------------------------------------------------------------------------------
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
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// INCLUDES
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "board/components/icm20948_imu_sensor.h"
#include "protocols/SPI.h"
#include "cmsis_os.h"
#include "utilities/common.h"
#include "math.h"
#include "board/hardware_definitions.h"
#include <stdio.h>

#define INTERNAL_ERROR -127
#define ACC_TYPE                                        (uint8_t) 0x140000
#define GYRO_TYPE                                       (uint8_t) 0x100000

#define ACC_LENGTH                                      6 // Length of a accelerometer measurement in bytes.
#define GYRO_LENGTH                                     6 // Length of a gyroscope measurement in bytes.

//#define CONFIG_IMU_SENSOR_DEFAULT_ACC_BANDWIDTH	        BMI08X_ACCEL_BW_NORMAL
//#define CONFIG_IMU_SENSOR_DEFAULT_ACC_ODR			    BMI08X_ACCEL_ODR_100_HZ
//#define CONFIG_IMU_SENSOR_DEFAULT_ACC_RANGE		        BMI088_ACCEL_RANGE_12G
//#define CONFIG_IMU_SENSOR_DEFAULT_ACC_POWER			    BMI08X_ACCEL_PM_ACTIVE
//
//#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_BANDWIDTH	    BMI08X_GYRO_BW_23_ODR_200_HZ
//#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_ODR		        BMI08X_GYRO_BW_23_ODR_200_HZ
//#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_RANGE		    BMI08X_GYRO_RANGE_1000_DPS
//#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_POWER		    BMI08X_GYRO_PM_NORMAL

#define SWO_Pin GPIO_PIN_4      // this is for the chip select pin TODO: change
#define SWO_GPIO_Port GPIOB     // if this value is changed make sure it is changed in the IMU.c file (readIMU and writeIMU) TODO: change
                                // if theres a way of automating that change I didnt know it

double SENSSCALEGYRO = 16.4;    // the IMU is programmed to have +/- 2000dps range so this is the sensitivity scale at that range
double SENSSCALEACCEL = 2048.0; // the IMU is programmed to have +/- 16g range so this is the sensitivity scale at that range

typedef struct
{
    uint8_t isInitialized       ;
    uint8_t isRunning           ;

    xTaskHandle taskHandle      ;
    void * taskParameters       ;
} IMUSensorTaskState;

static IMUSensorTaskState prvController     = {};


static QueueHandle_t s_queue = {0};
static uint8_t s_desired_processing_data_rate = 50;

// Wrapper functions for read and write
int icm20948_get_data ( uint8_t regAddress, int numBytes, uint8_t * dReturned );
int icm20948_set_data ( uint8_t regAddress, int numBytes, const uint8_t * dBuffer );
int icm20948_get_accel_data ( IMUSensorData * data );
int icm20948_get_gyro_data ( IMUSensorData * data );

void delay_ms(uint32_t period_ms);

// configuration functions for accelerometer and gyroscope
static int8_t accel_config(IMUSensorConfiguration * configParams);
static int8_t gyro_config (IMUSensorConfiguration * configParams);
int select_active_bank ( int bank );

static float imu_sensor_acc2g                ( int16_t acc_value );
static float imu_sensor_g2acc                ( float g );
static float imu_sensor_rot2deg_per_sec      ( int16_t gyro_value );
static float imu_sensor_deg_per_sec2rot      ( float deg_per_sec );


static const struct imu_sensor_configuration s_default_configuration = {

//        .accel_bandwidth                = CONFIG_IMU_SENSOR_DEFAULT_ACC_BANDWIDTH,
//        .accel_output_data_rate         = CONFIG_IMU_SENSOR_DEFAULT_ACC_ODR,
//        .accel_range                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_RANGE,
//        .accel_power                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_POWER,
//
//        .gyro_bandwidth                 = CONFIG_IMU_SENSOR_DEFAULT_GYRO_BANDWIDTH,
//        .gyro_output_data_rate          = CONFIG_IMU_SENSOR_DEFAULT_GYRO_ODR,
//        .gyro_range                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_RANGE,
//        .gyro_power                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_POWER,
};

static struct imu_sensor_configuration s_current_configuration = {

//        .accel_bandwidth                = CONFIG_IMU_SENSOR_DEFAULT_ACC_BANDWIDTH,
//        .accel_output_data_rate         = CONFIG_IMU_SENSOR_DEFAULT_ACC_ODR,
//        .accel_range                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_RANGE,
//        .accel_power                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_POWER,
//
//        .gyro_bandwidth                 = CONFIG_IMU_SENSOR_DEFAULT_GYRO_BANDWIDTH,
//        .gyro_output_data_rate          = CONFIG_IMU_SENSOR_DEFAULT_GYRO_ODR,
//        .gyro_range                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_RANGE,
//        .gyro_power                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_POWER,
};

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


int imu_sensor_configure (IMUSensorConfiguration * parameters )
{
    if(parameters == NULL)
    {
        s_current_configuration = s_default_configuration;
    }
    else
    {
        s_current_configuration = *parameters;
    }

    int status = IMU_OK;

    select_active_bank (0 );
    uint8_t pwrMGMT[1] = {1};

    status = icm20948_set_data (0x06, 1, pwrMGMT );
    if(status != IMU_OK)
    {
        return status;
    }

    status = accel_config(&s_current_configuration);
    if(status != IMU_OK)
    {
        return status;
    }

    status = gyro_config(&s_current_configuration);
    if(status != IMU_OK)
    {
        return status;
    }


    return IMU_OK;
}


int imu_sensor_init()
{
    int status = spi3_init ( );
    if ( status != 0 )
    {
        return IMU_ERR;
    }

    s_queue = xQueueCreate (10, sizeof ( IMUSensorData ) );
    if ( s_queue == NULL )
    {
        return IMU_ERR;
    }

    vQueueAddToRegistry ( s_queue, "bmi088_queue" );

    prvController.isInitialized = 1;

    return IMU_OK;
}

static void prv_imu_sensor_controller_task(void * pvParams)
{
    DEBUG_LINE("prv_imu_sensor_controller_task\r\n");
    IMUSensorConfiguration * configParams = (IMUSensorConfiguration *)pvParams;
    (void)configParams;

    IMUSensorData dataStruct = {};

    //main loop: continuously read sensor data
    //vTaskDelay(pdMS_TO_TICKS(100));//Wait so to make sure the other tasks have started.

    int8_t result_flag;

    TickType_t start_timestamp =  xTaskGetTickCount();

    prvController.isRunning = true;

    while(prvController.isRunning)
    {
        result_flag = icm20948_get_accel_data (&dataStruct);
        if(SPI_OK != result_flag)
        {
            continue;
        }

        result_flag = icm20948_get_gyro_data (&dataStruct);
        if(SPI_OK != result_flag)
        {
            continue;
        }

        dataStruct.timestamp = xTaskGetTickCount() - start_timestamp;

        imu_add_measurement(&dataStruct);

        vTaskDelayUntil(&dataStruct.timestamp, s_desired_processing_data_rate);
    }

    prvController.isRunning = false;
    DISPLAY_LINE( "[INFO]: IMU sensor task has been stopped");
    vTaskDelete( prvController.taskHandle );
}

int imu_sensor_start(void * const param)
{
    DEBUG_LINE( "[INFO]: IMU sensor task has been started");

    //Get the parameters.
    if ( !prvController.isInitialized )
    {
        return IMU_ERR;
    }

    prvController.taskParameters = param;

    if ( pdFALSE == xTaskCreate ( prv_imu_sensor_controller_task, "imu-manager", configMINIMAL_STACK_SIZE, prvController.taskParameters, 5, &prvController.taskHandle ) )
    {
        return IMU_ERR;
    }

    return IMU_OK;
}

bool imu_sensor_is_running     ()
{
    return prvController.isRunning;
}

void imu_sensor_stop           ()
{
    prvController.isRunning = false;
}


bool imu_read(IMUSensorData * buffer)
{
    return pdPASS == xQueueReceive(s_queue, buffer, 0);
}

void imu_sensor_data_pack(IMUSensorData reading, uint8_t* buffer, uint32_t timestamp)
{
    // Make sure time doesn't overwrite type and event bits.
    uint32_t header = (ACC_TYPE | GYRO_TYPE) + (timestamp & 0x0FFF);
    common_write_24(header, &buffer[0]);
    common_write_16(reading.acc_x, &buffer[3]);
    common_write_16(reading.acc_y, &buffer[5]);
    common_write_16(reading.acc_z, &buffer[7]);
    common_write_16(reading.gyro_x, &buffer[9]);
    common_write_16(reading.gyro_y, &buffer[11]);
    common_write_16(reading.gyro_z, &buffer[13]);
}

// set the accelerometer starting configurations
int8_t accel_config(IMUSensorConfiguration * configParams)
{
    select_active_bank(2);

    uint8_t prevSettings [ 2 ];
    icm20948_get_data ( (uint8_t) 0x140000, 2, prevSettings );
    prevSettings [ 0 ] |= 6;
    icm20948_set_data ( (uint8_t) 0x140000, 2, prevSettings );

    select_active_bank(0);
    return IMU_OK;
}

// set the accelerometer starting configurations
int8_t gyro_config(IMUSensorConfiguration * configParams)
{
    select_active_bank (2 );

    uint8_t prevSettings [ 2 ];
    icm20948_get_data ( (uint8_t) 0x100000, 2, prevSettings );
    prevSettings [ 0 ] |= 6;
    icm20948_set_data ( (uint8_t) 0x100000, 2, prevSettings );

    select_active_bank (0 );
    return IMU_OK;
}



void delay_ms ( uint32_t period_ms )
{
    HAL_Delay(period_ms);
}


IMUSensorConfiguration imu_sensor_get_default_configuration ( )
{
    return s_default_configuration;
}

IMUSensorConfiguration imu_sensor_get_current_configuration ( )
{
    return s_current_configuration;
}

void imu_sensor_set_desired_processing_data_rate(uint32_t rate)
{
    s_desired_processing_data_rate = rate;
}

bool imu_add_measurement (IMUSensorData * _data)
{
    return pdTRUE == xQueueSend(s_queue, (void *) _data,0);
}



int icm20948_get_data ( uint8_t regAddress, int numBytes, uint8_t * dReturned )
{
    for ( int i = 0; i < numBytes; i++ )
    {
        // get one byte at a time
        //Every register on IMU is only 8 bits wide
        //So for this function we will always only send 1 byte and have 1 byte returned
        //Because SPI, 2 bytes needs to be sent with the first being a dummy
        //The second byte read will be real data
        uint8_t dataTx [ 2 ];
        uint8_t dataRx [ 2 ];
        dataTx [ 0 ] = regAddress | 0x80; // make the 7th bit high to show its a read op
        dataTx [ 1 ] = 0x80; // setting dummy data

        if ( SPI_OK != spi3_receive ( dataTx, 2, dataRx, 2, 100 ) )
        {
            return IMU_ERR;
        }

        dReturned[i] = dataRx[1];
        regAddress++;
    }

    return IMU_OK;
}


int icm20948_set_data ( uint8_t regAddress, int numBytes, const uint8_t * dBuffer )
{
    for ( int i = 0; i < numBytes; i++ )
    {
        //one byte at a time again
        uint8_t dataTx [ 2 ];
        dataTx [ 0 ] = regAddress;
        dataTx [ 1 ] = dBuffer [ i ];

        if ( SPI_OK != spi3_single_transmit_only ( dataTx, 2, 100 ) )
        {
            return IMU_ERR;
        }

        regAddress++;
    }

    return IMU_OK;
}


int icm20948_get_accel_data ( IMUSensorData * data )
{
    //data is in x,y,z order
    float dBuffer   [ 3 ];
    uint8_t rawData [ 6 ];
    IMUStatus status;

    status = icm20948_get_data ( 45, 6, rawData );
    if ( status != IMU_OK )
    {
        return IMU_ERR;
    }

    for ( int i = 0; i < 6; i = i + 2 )
    {
        int16_t combinedVal = ( int16_t ) ( ( ( ( uint16_t ) rawData [ i ] ) << 8 ) | rawData [ i + 1 ] );
        dBuffer [ i/2 ] = combinedVal / SENSSCALEACCEL; // data sheet says to do this
    }

    memcpy ( &data->acc_x, dBuffer, sizeof ( float ) * 3 );
    return IMU_OK;
}

int icm20948_get_gyro_data ( IMUSensorData * data )
{
    float dBuffer   [ 3 ];
    uint8_t rawData [ 6 ];
    IMUStatus status;

    status = icm20948_get_data ( 51, 6, rawData );
    if ( status != IMU_OK )
    {
        return IMU_ERR;
    }

    for ( int i = 0; i < 6; i = i + 2 )
    {
        int16_t combinedVal = ( int16_t ) ( ( ( ( uint16_t ) rawData [ i ] ) << 8 ) | rawData [ i + 1 ] );
        dBuffer [ i/2 ] = combinedVal / SENSSCALEGYRO; // data sheet says to do this
    }

    memcpy ( &data->gyro_x, dBuffer, sizeof ( float ) * 3 );
    return IMU_OK;
}


int select_active_bank ( int bank )
{
    uint8_t dataOut [ 1 ];
    dataOut[0] = bank << 4;

    return icm20948_set_data (127, 1, dataOut );
}

bool imu_sensor_test ()
{
    int8_t result_flag;
    IMUSensorData dataStruct = {};

    result_flag = icm20948_get_accel_data (&dataStruct);
    if(IMU_OK != result_flag)
    {
        DISPLAY_LINE( "[ERROR]: IMU sensor acquisition failed \r\n");
        return false;
    }

    result_flag = icm20948_get_gyro_data (&dataStruct);
    if(IMU_OK != result_flag)
    {
        DISPLAY_LINE( "[ERROR]: IMU sensor acquisition failed \r\n");
        return false;
    }

    char bufx[IMU_STR_VAL_LENGTH+1];
    char bufy[IMU_STR_VAL_LENGTH+1];
    char bufz[IMU_STR_VAL_LENGTH+1];
    gcvt(dataStruct.acc_x, IMU_STR_VAL_LENGTH, bufx);
    gcvt(dataStruct.acc_y, IMU_STR_VAL_LENGTH, bufy);
    gcvt(dataStruct.acc_z, IMU_STR_VAL_LENGTH, bufz);
    DISPLAY_LINE( "[SUCCESS]: Accel value (x,y,z): (%s, %s, %s) \r\n", bufx, bufy, bufz);
    gcvt(dataStruct.gyro_x, IMU_STR_VAL_LENGTH, bufx);
    gcvt(dataStruct.gyro_y, IMU_STR_VAL_LENGTH, bufy);
    gcvt(dataStruct.gyro_z, IMU_STR_VAL_LENGTH, bufz);
    DISPLAY_LINE( "[SUCCESS]: Gyro value (x,y,z): (%s, %s, %s) \r\n", bufx, bufy, bufz);

    return true;

}
