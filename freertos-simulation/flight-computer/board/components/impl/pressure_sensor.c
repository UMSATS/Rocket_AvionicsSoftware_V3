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
#include "board/components/pressure_sensor.h"
#include <math.h>
#include <stdbool.h>
#include "board/components/pressure_sensor.h"
#include "board/board.h"

#include "board-hardware-drivers/BMI08x-Sensor-API/Inc/bmp3.h"
#include <cmsis_os.h>
#include "protocols/SPI.h"
#include "core/system_configuration.h"
#include "protocols/UART.h"
#include "utilities/common.h"

#include "event-detection/event_detector.h"

#define PRES_TYPE           0x200000
#define TEMP_TYPE           0x100000

#define TIMEOUT             100 // milliseconds

#define GND_ALT             0
#define GND_PRES            101325

#define CONFIG_PRESSURE_SENSOR_DEFAULT_ODR	                     BMP3_ODR_50_HZ
#define CONFIG_PRESSURE_SENSOR_DEFAULT_PRESSURE_OVERSAMPLING     BMP3_OVERSAMPLING_4X
#define CONFIG_PRESSURE_SENSOR_DEFAULT_TEMPERATURE_OVERSAMPLING  BMP3_OVERSAMPLING_4X
#define CONFIG_PRESSURE_SENSOR_DEFAULT_IIR_FILTER_COEFF          BMP3_IIR_FILTER_COEFF_15

typedef struct
{
    uint8_t isInitialized       ;
    uint8_t isRunning           ;

    xTaskHandle taskHandle      ;
    void * taskParameters       ;
} PressureSensorTaskState;

static PressureSensorTaskState prvController     = {};


static char buf[128];

static QueueHandle_t s_queue = {0};
static struct bmp3_data s_data = {0};
static uint8_t s_desired_processing_data_rate = 50;

static void     delay_ms                    (uint32_t period_ms);
static int8_t   spi_reg_write               (uint8_t cs, uint8_t reg_addr, uint8_t *reg_data, uint16_t length);
static int8_t   spi_reg_read                (uint8_t cs, uint8_t reg_addr, uint8_t *reg_data, uint16_t length);
static int8_t   get_sensor_data             (struct bmp3_data *data);
static void     bmp3_print_result           (const char *api_name, int8_t rslt);


static struct bmp3_dev s_device = {
        .delay_ms = delay_ms,
        /* Select the interface mode as SPI */
        .intf = BMP3_SPI_INTF,
        .read = spi_reg_read,
        .write = spi_reg_write,
        .dev_id = 0
};

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


int pressure_sensor_configure ( PressureSensorConfiguration * parameters )
{
    if ( parameters == NULL )
    {
        s_current_configuration = s_default_configuration;
    }
    else
    {
        s_current_configuration = *parameters;
    }

    int8_t result = 0;

    /* Used to select the settings user needs to change */
    uint16_t settings_sel;

    /* Select the pressure and temperature sensor to be enabled */
    s_device.settings.press_en              = BMP3_ENABLE;
    s_device.settings.temp_en               = BMP3_ENABLE;
    /* Select the output data rate and oversampling settings for pressure and temperature */
    s_device.settings.odr_filter.press_os   = s_current_configuration.pressure_oversampling;
    s_device.settings.odr_filter.temp_os    = s_current_configuration.temperature_oversampling;
    s_device.settings.odr_filter.odr        = s_current_configuration.output_data_rate;
    s_device.settings.odr_filter.iir_filter = s_current_configuration.infinite_impulse_response_filter_coefficient;
    /* Assign the settings which needs to be set in the sensor */
    settings_sel = BMP3_PRESS_EN_SEL | BMP3_TEMP_EN_SEL | BMP3_PRESS_OS_SEL | BMP3_TEMP_OS_SEL | BMP3_ODR_SEL |
                   BMP3_IIR_FILTER_SEL;

    result = bmp3_set_sensor_settings ( settings_sel, &s_device );
    if ( BMP3_OK == result )
    {
        return BMP3_OK;
    }

    /* Set the power mode to normal mode */
    s_device.settings.op_mode = BMP3_NORMAL_MODE;

    result = bmp3_set_op_mode ( &s_device );
    if ( BMP3_OK != result )
    {
        return result;
    }

    return BMP3_OK;
}


int pressure_sensor_init ( )
{
    int status = spi2_init ( );
    if ( status != 0 )
    {
        return status;
    }

    status = bmp3_init ( &s_device ); // bosch API initialization method
    if ( status != BMP3_OK )
    {
        return status;
    }

    s_queue = xQueueCreate( 10, sizeof ( PressureSensorData ) );
    if ( s_queue == NULL )
    {
        return 2;
    }

    vQueueAddToRegistry ( s_queue, "bmp3_queue" );

    prvController.isInitialized = true;
    return status;
}


float pressure_sensor_calculate_altitude ( PressureSensorData * reading, float ground_pressure, float ground_altitude )
{
    if ( reading == NULL )
    {
        return 0.0;
    }

    float p_term = powf ( ( ground_pressure / ( reading->pressure / 100 ) ), ( 1 / 5.257F ) ) - 1;
    float t_term = ( reading->temperature / 100 ) + 273.15F;
    return ( uint32_t ) ( p_term * t_term ) / 0.0065F + ground_altitude;
}

int8_t get_sensor_data ( struct bmp3_data * data )
{
    return bmp3_get_sensor_data ( BMP3_PRESS | BMP3_TEMP, data, &s_device );
}

static void prv_pressure_sensor_controller_task ( void * pvParams )
{
    DEBUG_LINE( "prv_pressure_sensor_controller_task" );
    PressureSensorConfiguration * configParams = ( PressureSensorConfiguration * ) pvParams;
    ( void ) configParams;
    /* Variable used to store the compensated data */
    PressureSensorData dataStruct;

    TickType_t start_timestamp = xTaskGetTickCount ( );

    int8_t result_flag;
    prvController.isRunning = true;

    while ( prvController.isRunning )
    {
        result_flag = get_sensor_data ( &s_data );
        if ( BMP3_E_NULL_PTR == result_flag )
        {
            continue;
        }

        dataStruct.pressure    = s_data.pressure;
        dataStruct.temperature = s_data.temperature;
        dataStruct.timestamp   = xTaskGetTickCount ( ) - start_timestamp;

        pressure_sensor_add_measurement ( &dataStruct );

        vTaskDelayUntil ( &dataStruct.timestamp, s_desired_processing_data_rate );
    }

    prvController.isRunning = false;
}

int pressure_sensor_start ( void * const pvParameters )
{
    DEBUG_LINE( "pressure_sensor_start" );
    //Get the parameters.
    if ( !prvController.isInitialized )
    {
        return IMU_ERR;
    }

    prvController.taskParameters = pvParameters;

    if ( pdFALSE == xTaskCreate ( prv_pressure_sensor_controller_task, "ps--manager", configMINIMAL_STACK_SIZE, prvController.taskParameters, 5, &prvController.taskHandle ) )
    {
        return IMU_ERR;
    }

    return IMU_OK;
}

bool pressure_sensor_is_running ( )
{
    return prvController.isRunning;
}

void pressure_sensor_stop ( )
{
    prvController.isRunning = false;
}

static void delay_ms ( uint32_t period_ms )
{
    if ( taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState ( ) )
    {
        board_delay ( period_ms );
    }
    else
    {
        vTaskDelay ( pdMS_TO_TICKS( period_ms ) );
    }
}

/*!
 *  @brief Function for writing the sensor's registers through SPI bus.
 *
 *  @param[in] cs           : Chip select to enable the sensor.
 *  @param[in] reg_addr     : Register address.
 *  @param[in] reg_data : Pointer to the data buffer whose data has to be written.
 *  @param[in] length       : No of bytes to write.
 *
 *  @return Status of execution
 *  @retval 0 -> Success
 *  @retval >0 -> Failure Info
 *
 */
static int8_t spi_reg_write ( uint8_t cs, uint8_t reg_addr, uint8_t * reg_data, uint16_t length )
{
    int8_t status = 0; //assume success
    status = spi2_send ( &reg_addr, 1, reg_data, length, TIMEOUT );
    if ( status != SPI_OK )
    {
        return BMP3_FATAL_ERR;
    }

    return BMP3_OK;
}
/*!
 *  @brief Function for reading the sensor's registers through SPI bus.
 *
 *  @param[in] cs       : Chip select to enable the sensor.
 *  @param[in] reg_addr : Register address.
 *  @param[out] reg_data    : Pointer to the data buffer to store the read data.
 *  @param[in] length   : No of bytes to read.
 *
 *  @return Status of execution
 *  @retval 0 -> Success
 *  @retval >0 -> Failure Info
 *
 */
static int8_t spi_reg_read ( uint8_t cs, uint8_t reg_addr, uint8_t * reg_data, uint16_t length )
{
    int status = 0; // assume success
    status = spi2_receive ( &reg_addr, 1, reg_data, length, TIMEOUT );
    if ( status != SPI_OK )
    {
        return BMP3_FATAL_ERR;
    }

    return BMP3_OK;
}
/*!
 *  @brief Prints the execution status of the APIs.
 *
 *  @param[in] api_name : name of the API whose execution status has to be printed.
 *  @param[in] rslt     : error code returned by the API whose execution status has to be printed.
 *
 *  @return void.
 */
void bmp3_print_result ( const char * api_name, int8_t rslt )
{
    if ( rslt != BMP3_OK )
    {
        char error_msg[64];
        if ( rslt == BMP3_E_NULL_PTR )
        {
            sprintf ( error_msg, "Null pointer error" );
        }
        else if ( rslt == BMP3_E_DEV_NOT_FOUND )
        {
            sprintf ( error_msg, "Device not found" );
        }
        else if ( rslt == BMP3_E_INVALID_ODR_OSR_SETTINGS )
        {
            sprintf ( error_msg, "Invalid ODR OSR settings" );
        }
        else if ( rslt == BMP3_E_CMD_EXEC_FAILED )
        {
            sprintf ( error_msg, "Command execution failed" );
        }
        else if ( rslt == BMP3_E_CONFIGURATION_ERR )
        {
            sprintf ( error_msg, "Configuration error" );
        }
        else if ( rslt == BMP3_E_INVALID_LEN )
        {
            sprintf ( error_msg, "Invalid length" );
        }
        else if ( rslt == BMP3_E_COMM_FAIL )
        {
            sprintf ( error_msg, "Communication failure" );
        }
        else if ( rslt == BMP3_E_FIFO_WATERMARK_NOT_REACHED )
        {
            sprintf ( error_msg, "FIFO Watermark not reached" );
        }
        else
        {
            //For more error codes refer "bmp3_defs.h"
            sprintf ( error_msg, "Unknown error code" );
        }

        sprintf ( buf, "\r\nERROR [%d] %s : %s\r\n", rslt, api_name, error_msg );
    }

    uart6_transmit ( buf );
}


bool pressure_sensor_test ( void )
{
    char    result = 0;
    uint8_t id     = 0x50;

    uint8_t command[] = { 0x80 };
    uint8_t id_read[] = { 0x00, 0x00 };

    int status = spi2_receive ( command, 1, id_read, 2, 10 );

    if ( status != 0 )
    {
        return false;
    }

    if ( id_read[ 1 ] == id )
    {
        result = 1;
    }

    return result == 1;
}

bool pressure_sensor_read ( PressureSensorData * buffer )
{
    return pdPASS == xQueueReceive( s_queue, buffer, 0 );
}


void pressure_sensor_data_pack ( PressureSensorData bmp_reading, float ground_pressure, float ground_temperature, uint8_t * bytes )
{
    //Update the header bytes.
    uint32_t header = ( bytes[ 0 ] << 16 ) + ( bytes[ 1 ] << 8 ) + bytes[ 2 ];
    header |= PRES_TYPE | TEMP_TYPE;

    common_write_24 ( header, &bytes[ 0 ] );
    common_write_24 ( bmp_reading.pressure, &bytes[ 15 ] );
    common_write_24 ( bmp_reading.temperature, &bytes[ 18 ] );
    float altitude = pressure_sensor_calculate_altitude ( &bmp_reading, ground_pressure, ground_temperature );
    common_float2bytes ( altitude, &bytes[ 21 ] );
}

bool pressure_sensor_add_measurement ( PressureSensorData * _data )
{
    return pdTRUE == xQueueSend( s_queue, _data, 0 );
}

PressureSensorConfiguration pressure_sensor_get_default_configuration ( )
{
    return s_default_configuration;
}


PressureSensorConfiguration pressure_sensor_get_current_configuration ( )
{
    return s_current_configuration;
}

void pressure_sensor_set_desired_processing_data_rate ( uint32_t rate )
{
    s_desired_processing_data_rate = rate;
}