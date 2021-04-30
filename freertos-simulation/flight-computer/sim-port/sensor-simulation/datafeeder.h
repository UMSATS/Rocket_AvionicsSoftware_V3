#ifndef __DATA_FEEDER_H
#define __DATA_FEEDER_H

#include <inttypes.h>
#include "configurations/UserConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (userconf_USE_COTS_DATA == 1)

typedef struct
{
    float timestamp;
/*! X-axis sensor data */
    float x;
/*! Y-axis sensor data */
    float y;
/*! Z-axis sensor data */
    float z;
} xyz_data;

typedef struct{
    float timestamp;
    /*! Compensated temperature */
    float temperature;
    /*! Compensated pressure */
    int64_t pressure;
}press_data;


#else
typedef struct
{
    uint32_t timestamp;
/*! X-axis sensor data */
    int16_t x;
/*! Y-axis sensor data */
    int16_t y;
/*! Z-axis sensor data */
    int16_t z;
} xyz_data;

typedef struct{
    uint32_t timestamp;
    /*! Compensated temperature */
    int64_t temperature;
    /*! Compensated pressure */
    int64_t pressure;
}press_data;

#endif


int datafeeder_get_gyro(xyz_data * data);
int datafeeder_get_acc(xyz_data * data);
int datafeeder_get_press(press_data * data);

int data_feeder_start(const char * file);
int data_feeder_is_running();
void data_feeder_stop();

void data_feeder_join();

#ifdef __cplusplus
}
#endif

#endif // __DATA_FEEDER_H

