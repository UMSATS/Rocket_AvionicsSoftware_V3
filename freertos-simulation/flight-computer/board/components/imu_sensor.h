#ifndef SENSOR_AG_H
#define SENSOR_AG_H

#include <inttypes.h>
#include <stdbool.h>
#include "protocols/UART.h"


typedef struct imu_sensor_configuration
{

    uint8_t     accel_bandwidth;
    uint8_t     accel_output_data_rate;
    uint8_t     accel_range;
    uint8_t     accel_power;

    uint8_t     gyro_bandwidth;
    uint8_t     gyro_output_data_rate;
    uint8_t     gyro_range;
    uint8_t     gyro_power;

} IMUSensorConfiguration;

typedef union
{
    struct{
        IMUSensorConfiguration values;
    };
    uint8_t bytes[sizeof(IMUSensorConfiguration)];
} IMUSensorConfigurationU;


typedef enum { IMU_ERR = 0, IMU_OK = 1 } IMUStatus;

//Groups both sensor readings and a time stamp.
typedef struct imu_sensor_data
{
    uint32_t timestamp; // time of sensor reading in ticks.

    float acc_x;
    float acc_y;
    float acc_z;

    float gyro_x;
    float gyro_y;
    float gyro_z;

} IMUSensorData;

typedef union
{
    struct
    {
        IMUSensorData values;
    };
    uint8_t bytes[sizeof ( IMUSensorData )];
} IMUSensorDataU;



int  imu_sensor_test           ();
int  imu_sensor_init           ();
int  imu_sensor_configure      ( IMUSensorConfiguration * parameters );
int  imu_sensor_start          ( void * const param );
bool imu_read                  ( IMUSensorData * buffer );
bool imu_add_measurement       ( IMUSensorData *_data );
bool imu_sensor_is_running     ();
void imu_sensor_stop           ();

IMUSensorConfiguration imu_sensor_get_default_configuration();

IMUSensorConfiguration imu_sensor_get_current_configuration();

void imu_sensor_set_desired_processing_data_rate(uint32_t rate);




#endif // SENSOR_AG_H
