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
#include "board/components/imu_sensor.h"
#include "board-hardware-drivers/BMI08x-Sensor-API/Inc/bmi088.h"
#include "protocols/SPI.h"
#include "board-hardware-drivers/BMI08x-Sensor-API/Inc/bmi08x.h"
#include "cmsis_os.h"
#include "utilities/common.h"
#include "math.h"

#define INTERNAL_ERROR -127
#define ACC_TYPE                                        0x800000
#define GYRO_TYPE                                       0x400000

#define ACC_LENGTH                                      6 // Length of a accelerometer measurement in bytes.
#define GYRO_LENGTH                                     6 // Length of a gyroscope measurement in bytes.

#define CONFIG_IMU_SENSOR_DEFAULT_ACC_BANDWIDTH	        BMI08X_ACCEL_BW_NORMAL
#define CONFIG_IMU_SENSOR_DEFAULT_ACC_ODR			    BMI08X_ACCEL_ODR_100_HZ
#define CONFIG_IMU_SENSOR_DEFAULT_ACC_RANGE		        BMI088_ACCEL_RANGE_12G
#define CONFIG_IMU_SENSOR_DEFAULT_ACC_POWER			    BMI08X_ACCEL_PM_ACTIVE

#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_BANDWIDTH	    BMI08X_GYRO_BW_23_ODR_200_HZ
#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_ODR		        BMI08X_GYRO_BW_23_ODR_200_HZ
#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_RANGE		    BMI08X_GYRO_RANGE_1000_DPS
#define CONFIG_IMU_SENSOR_DEFAULT_GYRO_POWER		    BMI08X_GYRO_PM_NORMAL

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

//Wrapper functions for read and write
int8_t user_spi_read (uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
int8_t user_spi_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);

void delay_ms(uint32_t period_ms);

// configuration functions for accelerometer and gyroscope
static int8_t accel_config(IMUSensorConfiguration * configParams);
static int8_t gyro_config (IMUSensorConfiguration * configParams);
static float imu_sensor_acc2g                ( int16_t acc_value );
static float imu_sensor_g2acc                ( float g );
static float imu_sensor_rot2deg_per_sec      ( int16_t gyro_value );
static float imu_sensor_deg_per_sec2rot      ( float deg_per_sec );

static struct bmi08x_dev s_device = {
        .accel_id = 0,
        .gyro_id = 1,
        .intf = BMI08X_SPI_INTF, // determines if we use SPI or I2C
        .read = user_spi_read,   //a function pointer to our spi read function
        .write = user_spi_write, //a function pointer to our spi write function
        .delay_ms = delay_ms//user_delay_milli_sec
};

static const struct imu_sensor_configuration s_default_configuration = {

        .accel_bandwidth                = CONFIG_IMU_SENSOR_DEFAULT_ACC_BANDWIDTH,
        .accel_output_data_rate         = CONFIG_IMU_SENSOR_DEFAULT_ACC_ODR,
        .accel_range                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_RANGE,
        .accel_power                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_POWER,

        .gyro_bandwidth                 = CONFIG_IMU_SENSOR_DEFAULT_GYRO_BANDWIDTH,
        .gyro_output_data_rate          = CONFIG_IMU_SENSOR_DEFAULT_GYRO_ODR,
        .gyro_range                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_RANGE,
        .gyro_power                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_POWER,
};

static struct imu_sensor_configuration s_current_configuration = {

        .accel_bandwidth                = CONFIG_IMU_SENSOR_DEFAULT_ACC_BANDWIDTH,
        .accel_output_data_rate         = CONFIG_IMU_SENSOR_DEFAULT_ACC_ODR,
        .accel_range                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_RANGE,
        .accel_power                    = CONFIG_IMU_SENSOR_DEFAULT_ACC_POWER,

        .gyro_bandwidth                 = CONFIG_IMU_SENSOR_DEFAULT_GYRO_BANDWIDTH,
        .gyro_output_data_rate          = CONFIG_IMU_SENSOR_DEFAULT_GYRO_ODR,
        .gyro_range                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_RANGE,
        .gyro_power                     = CONFIG_IMU_SENSOR_DEFAULT_GYRO_POWER,
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

    int status = BMI08X_OK;

    status = accel_config(&s_current_configuration);
    if(status != BMI08X_OK)
    {
        return status;
    }

    status = gyro_config(&s_current_configuration);
    if(status != BMI08X_OK)
    {
        return status;
    }


    return BMI08X_OK;
}


int imu_sensor_init()
{
    int status = spi3_init();
    if(status != 0)
    {
        return status;
    }

    status = bmi088_init(&s_device); // bosch API initialization method
    if(status != BMI08X_OK)
        return status;

    s_queue = xQueueCreate(10, sizeof(IMUSensorData));
    if (s_queue == NULL) {
        return 2;
    }

    vQueueAddToRegistry(s_queue, "bmi088_queue");


    prvController.isInitialized = 1;
    return BMI08X_OK;
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
    struct bmi08x_sensor_data container;

    TickType_t start_timestamp =  xTaskGetTickCount();

    prvController.isRunning = true;

    while(prvController.isRunning)
    {
        result_flag = bmi08a_get_data(&container, &s_device);
        if(BMI08X_E_NULL_PTR == result_flag)
        {
            continue;
        }
        dataStruct.acc_x = container.x,
        dataStruct.acc_y = container.y,
        dataStruct.acc_z = container.z;

        result_flag = bmi08g_get_data(&container, &s_device);
        if(BMI08X_E_NULL_PTR == result_flag)
        {
            continue;
        }

        dataStruct.gyro_x = container.x,
        dataStruct.gyro_y = container.y,
        dataStruct.gyro_z = container.z;

        dataStruct.timestamp = xTaskGetTickCount() - start_timestamp;

        imu_add_measurement(&dataStruct);
        vTaskDelayUntil(&dataStruct.timestamp, s_desired_processing_data_rate);
    }

    prvController.isRunning = false;
}

int imu_sensor_start(void * const param)
{
    DEBUG_LINE("pressure_sensor_start\r\n");
    //Get the parameters.
    if ( !prvController.isInitialized )
    {
        return IMU_ERR;
    }

    prvController.taskParameters = param;

//    TaskHandle_t xTask = controller.taskHandle;
//    TaskStatus_t pxTaskStatus;
//    BaseType_t xGetFreeStackSpace = 0;
//    eTaskState eState = eInvalid;
//    vTaskGetInfo(xTask, &pxTaskStatus, xGetFreeStackSpace, eState);


    if ( pdFALSE == xTaskCreate(prv_imu_sensor_controller_task, "imu-manager", configMINIMAL_STACK_SIZE, prvController.taskParameters, 5, &prvController.taskHandle ) )
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

//set the accelerometer starting configurations
int8_t accel_config(IMUSensorConfiguration * configParams){
    uint8_t data = 0;
    int8_t rslt;

    /* Read accel chip id */
    rslt = bmi08a_get_regs(BMI08X_ACCEL_CHIP_ID_REG, &data, 1, &s_device);
    if(rslt != BMI08X_OK)
        return rslt;
    
    /* Assign the desired configurations */
    //Not sure yet what configurations we want
    s_device.accel_cfg.bw    = configParams->accel_bandwidth;
    s_device.accel_cfg.odr   = configParams->accel_output_data_rate;
    s_device.accel_cfg.range = configParams->accel_range;
    s_device.accel_cfg.power = configParams->accel_power;

    rslt                     = bmi08a_set_power_mode(&s_device);
    if(rslt != BMI08X_OK)
        return rslt;
    /* Wait for 10ms to switch between the power modes - delay_ms taken care inside the function*/

    rslt = bmi08a_set_meas_conf(&s_device);
    if(rslt != BMI08X_OK)
        return rslt;

    return rslt;
}

//set the accelerometer starting configurations
int8_t gyro_config(IMUSensorConfiguration * configParams)
{
    uint8_t data = 0;
    int8_t rslt;

    /* Read gyro chip id */
    rslt = bmi08g_get_regs(BMI08X_GYRO_CHIP_ID_REG, &data, 1, &s_device);
    if(rslt != BMI08X_OK)
        return rslt;
    
    
    //set power mode
    s_device.gyro_cfg.power = configParams->gyro_power;
    rslt                    = bmi08g_set_power_mode(&s_device);
    if(rslt != BMI08X_OK)
        return rslt;
    
    /* Wait for 30ms to switch between the power modes - delay_ms taken care inside the function*/
    /* Assign the desired configurations */
   s_device.gyro_cfg.odr    = configParams->gyro_output_data_rate;
   s_device.gyro_cfg.range  = configParams->gyro_range;
   s_device.gyro_cfg.bw     = configParams->gyro_bandwidth;
   s_device.gyro_id         = data;

    rslt = bmi08g_set_meas_conf(&s_device);
    if(rslt != BMI08X_OK)
        return rslt;
    
    return rslt;
}

int8_t user_spi_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    int status;
    //debug removeHAL_GPIO_WritePin(USR_LED_PORT,USR_LED_PIN, GPIO_PIN_SET);
    if(dev_addr == 0x00|| dev_addr == 0x1E){
        //Accelerometer.
        if((status = spi3_receive(&reg_addr,1, data, len, 10)) != 0) // The register address will always be 1.
        {
            return status;
        }
    }
    else if(dev_addr == 0x01|| dev_addr == 0x0F){
//        //Gyroscope.
        if((status = spi3_receive(&reg_addr,1, data, len, 11)) != 0) // The register address will always be 1.
        {
            return status;
        }
    }

    //delay_ms(500);
    //HAL_GPIO_WritePin(USR_LED_PORT,USR_LED_PIN, GPIO_PIN_RESET);
    //delay_ms(500);
    return BMI08X_OK;
}

int8_t user_spi_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len){

    if(dev_addr == 0x00 || dev_addr == 0x1E){
    spi3_send(&reg_addr,1, data, len, 10);
    }
    else if(dev_addr == 0x01 || dev_addr == 0x0F){
        spi3_send(&reg_addr,1, data, len, 11);
    }
    return BMI08X_OK;
}

void delay_ms ( uint32_t period_ms )
{
    if ( taskSCHEDULER_NOT_STARTED == xTaskGetSchedulerState() )
    {
        board_delay(period_ms);
    }
    else
    {
        vTaskDelay(pdMS_TO_TICKS ( period_ms ) );
    }
}


static int8_t __imu_config(IMUSensorConfiguration * parameters)
{
    int8_t result = accel_config(parameters);
    if(result != BMI08X_OK)
    {
        return false;
    }

    result = gyro_config(parameters);
    if(result != BMI08X_OK)
    {
        return false;
    }
    
    return result == BMI08X_OK;
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

int imu_sensor_test()
{
//    int8_t result;
//    result = bmi08a_perform_selftest(s_bmp3_sensor->bmi088_ptr);
//    if(result != BMI08X_OK)
//        return false;
//
//    result = bmi08g_perform_selftest(s_bmp3_sensor->bmi088_ptr);
//    if(result != BMI08X_OK)
//        return false;
    
    char res = 0;
    uint8_t id = 0x1E;
    
    uint8_t command[] = {0x80};
    uint8_t id_read[] = {0x00,0x00,0x00,0x00};
    uint8_t id_dummy[] = {0x00,0x00};
    
    
    spi3_receive(command,1,id_dummy,2,10);
    spi3_receive(command,1,id_read,2,10);
    
    if(id_read[1] == id)
    {
        res += 1;
    }
    
    spi3_receive(command,1,id_read,2,11);
    
    if(id_read[0] == 0x0F)
    {
        res += 1;
    }
    
    if(res == 2)
    {
        return 1;
    }
    
    return 0;
}

bool imu_add_measurement (IMUSensorData * _data)
{
    return pdTRUE == xQueueSend(s_queue, _data,0);
}


