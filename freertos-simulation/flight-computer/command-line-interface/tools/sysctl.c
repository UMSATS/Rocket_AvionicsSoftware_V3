/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "sysctl.h"

#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include "core/flight_controller.h"
#include "board/components/imu_sensor.h"
#include "board/components/pressure_sensor.h"
#include "sim-port/sensor-simulation/datafeeder.h"


#include "protocols/UART.h"
#include "board/components/flash.h"
#include "core/system_configuration.h"
#include "memory-management/memory_manager.h"


static bool cli_tools_sysctl_fl    ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg );
static bool cli_tools_sysctl_imu   ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg );
static bool cli_tools_sysctl_press ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg );
static bool cli_tools_sysctl_df    ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg );


bool cli_tools_sysctl ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * cmd_option, const char * str_option_arg )
{
    if ( strcmp ( cmd_option, "sysctl" ) == 0 )
    {
        sprintf ( pcWriteBuffer, "%s", str_option_arg );
        return true;
    }

    if ( strcmp ( cmd_option, "fl" ) == 0 )
    {
        return cli_tools_sysctl_fl ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "imu" ) == 0 )
    {
        return cli_tools_sysctl_imu ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "press" ) == 0 )
    {
        return cli_tools_sysctl_press ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "df" ) == 0 )
    {
        return cli_tools_sysctl_df ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    sprintf ( pcWriteBuffer, "Command [%s] not recognized\r\n", cmd_option );
    return false;
}


static bool cli_tools_sysctl_fl ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "read";
    bool value = false;

    if ( strcmp ( str_option_arg, "enable" ) == 0 )
    {
        value = true;
    }
    else if ( strcmp ( str_option_arg, "disable" ) == 0 )
    {
        value = false;
    }
    else if ( strcmp ( str_option_arg, "0" ) == 0 )
    {
        value = false;
    }
    else if ( strcmp ( str_option_arg, "1" ) == 0 )
    {
        value = true;
    }
    else
    {
        sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.\r\n", cmd_option, str_option_arg );
        return false;
    }

    if ( value )
    {
        flight_controller_start ( NULL );
    }
    else
    {
        flight_controller_stop ( NULL );
    }

    sprintf ( pcWriteBuffer, "Success!\r\n" );
    return true;
}

static bool cli_tools_sysctl_imu ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "read";
    bool value = false;

    if ( strcmp ( str_option_arg, "enable" ) == 0 )
    {
        value = true;
    }
    else if ( strcmp ( str_option_arg, "disable" ) == 0 )
    {
        value = false;
    }
    else if ( strcmp ( str_option_arg, "0" ) == 0 )
    {
        value = false;
    }
    else if ( strcmp ( str_option_arg, "1" ) == 0 )
    {
        value = true;
    }
    else
    {
        sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.\r\n", cmd_option, str_option_arg );
        return false;
    }

    if ( value )
    {
        imu_sensor_start ( NULL );
    }
    else
    {
        imu_sensor_stop ( NULL );
    }

    sprintf ( pcWriteBuffer, "Success!\r\n" );
    return true;
}

static bool cli_tools_sysctl_press ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "read";
    bool value = false;

    if ( strcmp ( str_option_arg, "enable" ) == 0 )
    {
        value = true;
    }
    else if ( strcmp ( str_option_arg, "disable" ) == 0 )
    {
        value = false;
    }
    else if ( strcmp ( str_option_arg, "0" ) == 0 )
    {
        value = false;
    }
    else if ( strcmp ( str_option_arg, "1" ) == 0 )
    {
        value = true;
    }
    else
    {
        sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.\r\n", cmd_option, str_option_arg );
        return false;
    }

    if ( value )
    {
        pressure_sensor_start ( NULL );
    }
    else
    {
        pressure_sensor_stop ( NULL );
    }


    sprintf ( pcWriteBuffer, "Success!\r\n" );
    return true;
}

static bool cli_tools_sysctl_df ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "read";
    bool value = false;

    if ( strcmp ( str_option_arg, "enable" ) == 0 )
    {
        value = true;
    }
    else if ( strcmp ( str_option_arg, "disable" ) == 0 )
    {
        value = false;
    }
    else if ( strcmp ( str_option_arg, "0" ) == 0 )
    {
        value = false;
    }
    else if ( strcmp ( str_option_arg, "1" ) == 0 )
    {
        value = true;
    }
    else
    {
        sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.\r\n", cmd_option, str_option_arg );
        return false;
    }

    if ( value )
    {
#if ( userconf_FREE_RTOS_SIMULATOR_MODE_ON )
#define MAKE_STR(x) _MAKE_STR(x)
#define _MAKE_STR(x) #x
#if (userconf_USE_COTS_DATA == 1)
        const char *CSV_FILE_PATH = MAKE_STR(COTS_CSV_FILE_PATH) ;
        data_feeder_start ( CSV_FILE_PATH );
        sprintf(pcWriteBuffer, "Success!\n");
        return true;
#else
        const char *CSV_FILE_PATH = MAKE_STR(SRAD_CSV_FILE_PATH) ;
        data_feeder_start ( CSV_FILE_PATH );
        sprintf(pcWriteBuffer, "Success!\n");
        return true;
#endif
#endif
    }
    else
    {
#if ( userconf_FREE_RTOS_SIMULATOR_MODE_ON )
        data_feeder_stop();
        sprintf(pcWriteBuffer, "Success!\n");
        return true;
#endif
    }


    sprintf ( pcWriteBuffer, "Failure!\r\n" );
    return false;
}




