/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "mem.h"

#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <stdlib.h>


#include "protocols/UART.h"
#include "board/components/flash.h"
#include "core/system_configuration.h"
#include "memory-management/memory_manager.h"


static bool cli_tools_mem_read                           (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_mem_scan                           (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_mem_erase_data_section             (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_mem_erase_config_section           (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_mem_erase_all                      (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_mem_read_imu_index                 (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_mem_read_press_index               (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_mem_read_cont_index                (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_mem_read_flight_event_index        (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_mem_read_configuration_index       (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_mem_stats                          (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);


bool cli_tools_mem ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * cmd_option, const char * str_option_arg )
{
    if ( strcmp ( cmd_option, "mem" ) == 0 )
    {
        sprintf ( pcWriteBuffer, "%s", str_option_arg );
        return true;
    }

    if ( strcmp ( cmd_option, "read_imu_index" ) == 0 )
    {
        return cli_tools_mem_read_imu_index ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "read_press_index" ) == 0 )
    {
        return cli_tools_mem_read_press_index ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "read_cont_index" ) == 0 )
    {
        return cli_tools_mem_read_cont_index ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "read_flight_event_index" ) == 0 )
    {
        return cli_tools_mem_read_flight_event_index ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "read_configuration" ) == 0 )
    {
        return cli_tools_mem_read_configuration_index ( pcWriteBuffer, xWriteBufferLen, "0" );
    }

    if ( strcmp ( cmd_option, "stats" ) == 0 )
    {
        return cli_tools_mem_stats ( pcWriteBuffer, xWriteBufferLen, NULL );
    }

    if ( strcmp ( cmd_option, "read" ) == 0 )
    {
        return cli_tools_mem_read ( pcWriteBuffer, xWriteBufferLen, NULL );
    }

    if ( strcmp ( cmd_option, "scan" ) == 0 )
    {
        return cli_tools_mem_scan ( pcWriteBuffer, xWriteBufferLen, NULL );
    }

    if ( strcmp ( cmd_option, "erase_data_section" ) == 0 )
    {
        return cli_tools_mem_erase_data_section ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "erase_config_section" ) == 0 )
    {
        return cli_tools_mem_erase_config_section ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "erase_all" ) == 0 )
    {
        return cli_tools_mem_erase_all ( pcWriteBuffer, xWriteBufferLen, NULL );
    }

    sprintf ( pcWriteBuffer, "Command [%s] not recognized\r\n", cmd_option );
    return false;
}

bool cli_tools_mem_read ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "read";
    uint32_t value = atoi ( str_option_arg );

    if ( value > 0 && value <= FLASH_END_ADDRESS )
    {
        sprintf ( pcWriteBuffer, "Reading 256 bytes starting at address %lu ...\r\n", value );

        uint8_t     data_rx[FLASH_PAGE_SIZE];
        FlashStatus stat;
        stat = flash_read ( value, data_rx, FLASH_PAGE_SIZE );

        if ( stat == FLASH_OK )
        {
            sprintf ( pcWriteBuffer, "Success!\r\n" );
        }
        else
        {
            sprintf ( pcWriteBuffer, "Failed!\r\n" );
            return false;
        }

        for ( size_t i = 0; i < FLASH_PAGE_SIZE; i++ )
        {
            if ( ( i + 1 ) % 16 == 0 )
            {
                sprintf ( pcWriteBuffer, "0x%02X\r\n", data_rx[ i ] );
            }
            else
            {
                sprintf ( pcWriteBuffer, "0x%02X\r\n", data_rx[ i ] );
            }
        }

        return true;
    }

    sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is in invalid range\r\n", cmd_option, str_option_arg );
    return false;
}


bool cli_tools_mem_scan ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "scan";
    uint32_t value = atoi ( str_option_arg );
    ( void ) cmd_option;
    ( void ) value;

    uint32_t end_Address = flash_scan ( );
    sprintf ( pcWriteBuffer, "End address :%lu \r\n", end_Address );

    return true;
}


bool cli_tools_mem_erase_data_section ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    uint8_t dataRX [ FLASH_PAGE_SIZE ];

    sprintf ( pcWriteBuffer, "Erasing data section ...\r\n" );

    uint32_t    address = FLASH_START_ADDRESS;
    FlashStatus stat    = FLASH_ERR;

    while ( address <= FLASH_END_ADDRESS )
    {

        //if(address > FLASH_PARAM_END_ADDRESS)
        {
            // stat = flash_erase_sector(address);
            // address += FLASH_SECTOR_SIZE;
        }
        //else
        {
            // stat = flash_erase_param_sector(address);
            //address += FLASH_PARAM_SECTOR_SIZE;
        }

//        HAL_GPIO_TogglePin(USR_LED_PORT,USR_LED_PIN);
        sprintf ( pcWriteBuffer, "Erasing sector %lu ...\r\n", address );
    }

    flash_read ( FLASH_START_ADDRESS, dataRX, 256 );
    uint16_t empty = 0xFFFF;
    int      i;
    for ( i = 0; i < 256; i++ )
    {

        if ( dataRX[ i ] != 0xFF )
        {
            empty--;
        }
    }

    if ( empty == 0xFFFF )
    {

        sprintf ( pcWriteBuffer, "Flash Erased Success!\r\n" );
    }

    if ( stat == FLASH_OK )
    {
        sprintf ( pcWriteBuffer, "Success!\r\n" );
    }
    else
    {
        sprintf ( pcWriteBuffer, "Failed!\r\n" );
        return false;
    }

    return true;
}

bool cli_tools_mem_erase_config_section ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "erase_config_section";
    ( void ) cmd_option;

    if ( MEM_OK != memory_manager_erase_configuration_section ( ) )
    {
        sprintf ( pcWriteBuffer, "Failure!\r\n" );
        return false;
    }

    sprintf ( pcWriteBuffer, "Success!\r\n" );
    return true;
}

bool cli_tools_mem_erase_all ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "erase_all";
    ( void ) cmd_option;

    if ( MEM_OK != memory_manager_erase_everything ( ) )
    {
        sprintf ( pcWriteBuffer, "Failure!\r\n" );
        return false;
    }

    sprintf ( pcWriteBuffer, "Success!\r\n" );
    return true;
}


static bool cli_tools_mem_read_imu_index ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{

    const char * cmd_option = "read_imu_index";
    uint32_t value = atoi ( str_option_arg );

    IMUDataU dst = { };
    if ( MEM_OK == memory_manager_get_single_data_entry ( MemoryUserDataSectorGyro, &dst, value ) )
    {
        sprintf ( pcWriteBuffer, "[%s]: timestamp=%lu: gyro=[%f, %f, %f]\n", cmd_option, dst.values.timestamp, dst.values.data[ 0 ], dst.values.data[ 1 ], dst.values.data[ 2 ] );

        return true;
    }

    memset ( &dst, 0, sizeof ( IMUDataU ) );
    if ( MEM_OK == memory_manager_get_single_data_entry ( MemoryUserDataSectorAccel, &dst, value ) )
    {
        sprintf ( pcWriteBuffer, "[%s]: timestamp=%lu: acc=[%f, %f, %f]\n", cmd_option, dst.values.timestamp, dst.values.data[ 0 ], dst.values.data[ 1 ], dst.values.data[ 2 ] );

        return true;
    }

    sprintf ( pcWriteBuffer, "Failure!\r\n" );
    return false;

}

static bool cli_tools_mem_read_press_index ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "read_press_index";
    uint32_t value = atoi ( str_option_arg );

    PressureDataU dst = { };
    if ( MEM_OK == memory_manager_get_single_data_entry ( MemoryUserDataSectorPressure, &dst, value ) )
    {
        sprintf ( pcWriteBuffer, "[%s]: timestamp=%lu, pressure=%f\n", cmd_option, dst.values.timestamp, dst.values.data );
        return true;
    }

    sprintf ( pcWriteBuffer, "Failure!\r\n" );
    return false;
}


static bool cli_tools_mem_read_cont_index ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "read_cont_index";
    uint32_t value = atoi ( str_option_arg );

    ContinuityU dst = { };
    if ( MEM_OK == memory_manager_get_single_data_entry ( MemoryUserDataSectorContinuity, &dst, value ) )
    {
        sprintf ( pcWriteBuffer, "[%s]: timestamp=%lu, status=%i,%i\n", cmd_option, dst.values.timestamp, dst.values.status[ 0 ], dst.values.status[ 1 ] );
        return true;
    }

    sprintf ( pcWriteBuffer, "Failure!\r\n" );
    return false;
}

static bool cli_tools_mem_read_flight_event_index ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "read_flight_event_index";
    uint32_t value = atoi ( str_option_arg );

    FlightEventU dst = { };
    if ( MEM_OK == memory_manager_get_single_data_entry ( MemoryUserDataSectorFlightEvent, &dst, value ) )
    {
        sprintf ( pcWriteBuffer, "[%s]: timestamp=%lu, status=%i\n", cmd_option, dst.values.timestamp, dst.values.status );
        return true;
    }

    sprintf ( pcWriteBuffer, "Failure!\r\n" );
    return false;
}

static bool cli_tools_mem_read_configuration_index ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "read_configuration";
    uint32_t value = atoi ( str_option_arg );

    GlobalConfigurationU dst = { };
    if ( MEM_OK == memory_manager_get_single_data_entry ( MemorySystemSectorGlobalConfigurationData, &dst, value ) )
    {
        snprintf(pcWriteBuffer, xWriteBufferLen,
        "[%s]:\r\n"
        "signature = %s\r\n"
        "\r\n"
        "Memory:\r\n"
        " write_drogue_continuity_ms      = %i\r\n"
        " write_pre_launch_multiplier     = %i\r\n"
        " write_pre_apogee_multiplier     = %i\r\n"
        " write_post_apogee_multiplier    = %i\r\n"
        " write_ground_multiplier         = %i\r\n"
        " write_interval_accelerometer_ms = %i\r\n"
        " write_interval_gyroscope_ms     = %i\r\n"
        " write_interval_magnetometer_ms  = %i\r\n"
        " write_interval_pressure_ms      = %i\r\n"
        " write_interval_altitude_ms      = %i\r\n"
        " write_interval_temperature_ms   = %i\r\n"
        " write_interval_flight_state_ms  = %i\r\n"
        " write_drogue_continuity_ms      = %i\r\n"
        " write_main_continuity_ms        = %i\r\n"
        "\r\n"
        "System:\r\n"
        " landing_rotation_speed_deg_per_sec      = %lu\r\n"
        " backup_time_launch_to_apogee_sec        = %lu\r\n"
        " backup_time_apogee_to_main_sec          = %lu\r\n"
        " backup_time_main_to_ground_sec          = %lu\r\n"
        " ground_pressure                         = %f\r\n"
        " ground_temperature                      = %f\r\n"
        " last_current_system_time                = %lu\r\n"
        " altitude_main_recovery_m                = %i\r\n"
        " last_flight_state                       = %i\r\n"
        " last_power_mode                         = %i\r\n"
        " launch_acceleration_critical_value_m_s2 = %i\r\n"
        " e_match_line_keep_active_for            = %i\r\n"
        " imu_data_needs_to_be_converted          = %i\r\n"
        " pressure_data_needs_to_be_converted     = %i\r\n"
        "\r\n"
        "IMU:\r\n"
        " accel_bandwidth        = %i\r\n"
        " accel_output_data_rate = %i\r\n"
        " accel_range            = %i\r\n"
        " accel_power            = %i\r\n"
        " gyro_bandwidth         = %i\r\n"
        " gyro_output_data_rate  = %i\r\n"
        " gyro_range             = %i\r\n"
        " gyro_power             = %i\r\n"
        "\r\n"
        "Pressure:\r\n"
        " output_data_rate                             = %i\r\n"
        " temperature_oversampling                     = %i\r\n"
        " pressure_oversampling                        = %i\r\n"
        " infinite_impulse_response_filter_coefficient = %i\r\n",

        cmd_option,

        (const char*) dst.values.signature,

        dst.values.memory.write_drogue_continuity_ms,
        dst.values.memory.write_pre_launch_multiplier,
        dst.values.memory.write_pre_apogee_multiplier,
        dst.values.memory.write_post_apogee_multiplier,
        dst.values.memory.write_ground_multiplier,
        dst.values.memory.write_interval_accelerometer_ms,
        dst.values.memory.write_interval_gyroscope_ms,
        dst.values.memory.write_interval_magnetometer_ms,
        dst.values.memory.write_interval_pressure_ms,
        dst.values.memory.write_interval_altitude_ms,
        dst.values.memory.write_interval_temperature_ms,
        dst.values.memory.write_interval_flight_state_ms,
        dst.values.memory.write_drogue_continuity_ms,
        dst.values.memory.write_main_continuity_ms,

        dst.values.system.landing_rotation_speed_deg_per_sec,
        dst.values.system.backup_time_launch_to_apogee_sec,
        dst.values.system.backup_time_apogee_to_main_sec,
        dst.values.system.backup_time_main_to_ground_sec,
        dst.values.system.ground_pressure,
        dst.values.system.ground_temperature,
        0L, // dst.values.system.current_system_time,
        dst.values.system.altitude_main_recovery_m,
        0, // dst.values.system.flight_state,
        0, // dst.values.system.power_mode,
        dst.values.system.launch_acceleration_critical_value_m_s2,
        dst.values.system.e_match_line_keep_active_for,
        dst.values.system.imu_data_needs_to_be_converted,
        dst.values.system.pressure_data_needs_to_be_converted,

        dst.values.system.imu_sensor_configuration.accel_bandwidth,
        dst.values.system.imu_sensor_configuration.accel_output_data_rate,
        dst.values.system.imu_sensor_configuration.accel_range,
        dst.values.system.imu_sensor_configuration.accel_power,
        dst.values.system.imu_sensor_configuration.gyro_bandwidth,
        dst.values.system.imu_sensor_configuration.gyro_output_data_rate,
        dst.values.system.imu_sensor_configuration.gyro_range,
        dst.values.system.imu_sensor_configuration.gyro_power,

        dst.values.system.pressure_sensor_configuration.output_data_rate,
        dst.values.system.pressure_sensor_configuration.temperature_oversampling,
        dst.values.system.pressure_sensor_configuration.pressure_oversampling,
        dst.values.system.pressure_sensor_configuration.infinite_impulse_response_filter_coefficient
        );

        return true;
    }

    sprintf ( pcWriteBuffer, "Failure!\r\n" );
    return false;
}


static bool cli_tools_mem_stats ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "stats";
    uint32_t value = atoi ( str_option_arg );
    ( void ) cmd_option;
    ( void ) value;

    if ( MEM_OK == memory_manager_get_stats ( pcWriteBuffer, xWriteBufferLen ) )
    {
        return true;
    }

    sprintf ( pcWriteBuffer, "Failure!\r\n" );
    return false;
}


