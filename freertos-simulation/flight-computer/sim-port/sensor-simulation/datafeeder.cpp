#include"datafeeder.h"
#include <string>
#include <memory>
#include <cassert>
#include <pthread.h>
#include <thread>
#include <mutex>
#include <deque>
#include <unistd.h>
#include <stddef.h>
#include <iostream>
#include <time.h>
#include <errno.h>

#include <FreeRTOS.h>
#include <task.h>

#include "csv.h"
#include "flight-computer/protocols/UART.h"

namespace
{
    const int MAX_ITEMS             = 100;

    pthread_t                       worker;

    static std::deque < xyz_data >   gyro_queue;
    static std::deque < xyz_data >   acc_queue;
    static std::deque < press_data > press_queue;
    static std::string               csv_file_name;

    xTaskHandle                     handle;

    int msleep( long msec )
    {
        struct timespec ts { };
        int res;

        if ( msec < 0 )
        {
            errno = EINVAL;
            return -1;
        }

        ts.tv_sec = msec / 1000;
        ts.tv_nsec = ( msec % 1000 ) * 1000000;

        do
        {
            res = nanosleep( &ts, &ts );
        } while ( res && errno == EINTR );

        return res;
    }

    static int isRunning = 0;
}

#ifdef __cplusplus
extern "C" {
#endif


#if (userconf_USE_COTS_DATA == 1)
uint32_t timestamp_uint;
void * worker_function( void * arg )
{
    double timestamp { };
    xyz_data gyro, acc, mag;
    press_data press;
    double altMSL;
    uint8_t flags[4];



    io::CSVReader<17> reader (csv_file_name);

    // time,acceleration,pres,altMSL,temp,latxacc,latyacc,gyrox,gyroy,gyroz,magx,magy,magz,launch_detect,apogee_detect,Aon,Bon

    DEBUG_LINE("C++ DataFeeder has successfully started.");
    isRunning = 1;
    while (isRunning && reader.read_row(timestamp, acc.x, press.pressure, altMSL, press.temperature, acc.y, acc.z, gyro.x, gyro.y, gyro.z, mag.x, mag.y, mag.z, flags[0], flags[1], flags[2], flags[3]))
    {
        timestamp_uint += 50;
        acc.timestamp   = timestamp;
        gyro.timestamp  = timestamp;
        press.timestamp = timestamp;

        taskENTER_CRITICAL( );

        if ( acc_queue.size( ) >= MAX_ITEMS )
            acc_queue.pop_front( );
        acc_queue.push_back( acc );

        if ( gyro_queue.size( ) >= MAX_ITEMS )
            gyro_queue.pop_front( );
        gyro_queue.push_back( gyro );

        if ( press_queue.size( ) >= MAX_ITEMS )
            press_queue.pop_front( );
        press_queue.push_back( press );

        taskEXIT_CRITICAL( );

        msleep( 5 );
    }

    DEBUG_LINE("C++ DataFeeder has successfully exited.");
    isRunning = 0;

    return nullptr;
}

#else
uint32_t timestamp_uint;
void * worker_function( void * arg )
{
    DEBUG_LINE("C++ DataFeeder has successfully started.");

    isRunning = 1;
    double timestamp { };
    xyz_data gyro, acc;
    press_data press;
    double alt;
    uint32_t ev;

    io::CSVReader<9> reader (csv_file_name);

    // time,accx,accy,accz,rotx,roty,rotz,temp,pres,alt,Flags
    DEBUG_LINE("C++ DataFeeder has successfully started.");
    isRunning = 1;

    while (isRunning && reader.read_row(timestamp, acc.x, acc.y, acc.z, gyro.x, gyro.y, gyro.z, press.data, press.temperature))
    {
        timestamp_uint += 50;
        acc.timestamp   = timestamp_uint;
        gyro.timestamp  = timestamp_uint;
        press.timestamp = timestamp_uint;

        taskENTER_CRITICAL( );

        if ( acc_queue.size( ) >= MAX_ITEMS )
            acc_queue.pop_front( );
        acc_queue.push_back( acc );

        if ( gyro_queue.size( ) >= MAX_ITEMS )
            gyro_queue.pop_front( );
        gyro_queue.push_back( gyro );

        if ( press_queue.size( ) >= MAX_ITEMS )
            press_queue.pop_front( );
        press_queue.push_back( press );

        taskEXIT_CRITICAL( );

        msleep( 50 );
    }

    DEBUG_LINE("C++ DataFeeder has successfully exited.");

    isRunning = 0;
    return nullptr;
}
#endif

int data_feeder_is_running( )
{
    return isRunning;
}

void data_feeder_stop()
{
    isRunning = false;
}


#ifdef __cplusplus
}







#endif



void prv_task_fnc( void * pvParams )
{
    try
    {
        if ( pthread_create( &worker, nullptr, worker_function, nullptr ) )
        {
            fprintf( stderr, "Error creating thread\n" );
            return;
        }
    }
    catch ( const char * s )
    {
        printf( "Error: %s", s );
        return;
    }
}



int data_feeder_start( const char * file )
{
    if( ! isRunning )
    {
        csv_file_name = std::string(file, strlen(file));
        if (pdFALSE == xTaskCreate(prv_task_fnc, "fake-sensor-data", configMINIMAL_STACK_SIZE, nullptr, 5, &handle)) {
            return 1;
        }
    }

    return 0;
}



int datafeeder_get_gyro( xyz_data * data )
{
    if ( !gyro_queue.empty( ) )
    {
        taskENTER_CRITICAL( );

        auto _data = &gyro_queue.front( );
        data->timestamp = _data->timestamp;
        data->x = _data->x;
        data->y = _data->y;
        data->z = _data->z;
        gyro_queue.pop_front( );

        taskEXIT_CRITICAL( );

        return 1;
    }

    return 0;
}



int datafeeder_get_acc( xyz_data * data )
{
    if ( !acc_queue.empty( ) )
    {
        taskENTER_CRITICAL( );

        auto _data = &acc_queue.front( );
        data->timestamp = _data->timestamp;
        data->x = _data->x;
        data->y = _data->y;
        data->z = _data->z;
        acc_queue.pop_front( );

        taskEXIT_CRITICAL( );

        return 1;
    }

    return 0;
}



int datafeeder_get_press( press_data * data )
{
    if ( !press_queue.empty( ) )
    {
        taskENTER_CRITICAL( );

        auto _data = &press_queue.front( );
        data->timestamp = _data->timestamp;
        data->temperature = _data->temperature;
        data->pressure = _data->pressure;
        press_queue.pop_front( );

        taskEXIT_CRITICAL( );

        return 1;
    }

    return 0;
}



void data_feeder_join( )
{
    pthread_join( worker, nullptr );
}


