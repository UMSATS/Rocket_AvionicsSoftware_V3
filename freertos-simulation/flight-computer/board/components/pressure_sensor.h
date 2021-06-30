#ifndef PRESSURE_SENSOR_BMP3_H
#define PRESSURE_SENSOR_BMP3_H


#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>
#include "protocols/UART.h"


typedef struct pressure_sensor_configuration
{

    uint8_t     output_data_rate;
    uint8_t     temperature_oversampling;
    uint8_t     pressure_oversampling;
    uint8_t     infinite_impulse_response_filter_coefficient;

} PressureSensorConfiguration;


typedef union
{
    struct{
        PressureSensorConfiguration values;
    };
    uint8_t bytes[sizeof(PressureSensorConfiguration)];
} PressureSensorConfigurationU;


//Groups a time stamp with the reading.
typedef struct pressure_sensor_data
{
    uint32_t timestamp; // time of sensor reading in ticks.
    /*! Compensated temperature */
    float temperature;
    /*! Compensated pressure */
    float pressure;
} PressureSensorData;

typedef union
{
    struct{
        PressureSensorData values;
    };
    uint8_t bytes[sizeof(PressureSensorData)];
} PressureSensorDataU;

typedef enum { PRESS_SENSOR_OK = 0, PRESS_SENSOR_ERR = 1 } PressureSensorStatus;


int     pressure_sensor_init                ();
int     pressure_sensor_configure           ( PressureSensorConfiguration * parameters );
int     pressure_sensor_start               ( void * const pvParameters );
bool    pressure_sensor_test                ( void );
bool    pressure_sensor_read                ( PressureSensorData * buffer );
bool    pressure_sensor_add_measurement     ( PressureSensorData * _data );
bool    pressure_sensor_is_running          ();
void    pressure_sensor_stop                ();


PressureSensorConfiguration pressure_sensor_get_default_configuration ( );
PressureSensorConfiguration pressure_sensor_get_current_configuration ( );
bool pressure_sensor_recalibrate ( );
void pressure_sensor_set_desired_processing_data_rate ( uint32_t rate );


#ifdef __cplusplus
}
#endif


#endif // PRESSURE_SENSOR_BMP3_H
