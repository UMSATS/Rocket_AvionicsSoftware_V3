/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "configure.h"

#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <stdlib.h>


#include "protocols/UART.h"
#include "board/components/flash.h"
#include "core/system_configuration.h"
#include "memory-management/memory_manager.h"


static bool cli_tools_configure_set_data_rate                  (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_initial_time               (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_record_to_flash            (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_accel_bw                   (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_accel_range                (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_accel_odr                  (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_gyro_bw                    (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_gyro_bw                    (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_gyro_range                 (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_press_odr                  (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_press_os                   (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_temp_os                    (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_set_press_iir_coeff            (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);
static bool cli_tools_configure_show                           (char* pcWriteBuffer, size_t xWriteBufferLen, const char* str_option_arg);


bool cli_tools_configure ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * cmd_option, const char * str_option_arg )
{
    if ( strcmp ( cmd_option, "configure" ) == 0 )
    {
        sprintf ( pcWriteBuffer, "%s", str_option_arg );
        return true;
    }

    if ( strcmp ( cmd_option, "set_data_rate" ) == 0 )
    {
        return cli_tools_configure_set_data_rate ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_initial_time" ) == 0 )
    {
        return cli_tools_configure_set_initial_time ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_record_to_flash" ) == 0 )
    {
        return cli_tools_configure_set_record_to_flash ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_accel_bw" ) == 0 )
    {
        return cli_tools_configure_set_accel_bw ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_accel_range" ) == 0 )
    {
        return cli_tools_configure_set_accel_range ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_accel_odr" ) == 0 )
    {
        return cli_tools_configure_set_accel_odr ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_gyro_bw" ) == 0 )
    {
        return cli_tools_configure_set_gyro_bw ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_gyro_range" ) == 0 )
    {
        return cli_tools_configure_set_gyro_range ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_press_odr" ) == 0 )
    {
        return cli_tools_configure_set_press_odr ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_press_os" ) == 0 )
    {
        return cli_tools_configure_set_press_os ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_temp_os" ) == 0 )
    {
        return cli_tools_configure_set_temp_os ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "set_press_iir_coeff" ) == 0 )
    {
        return cli_tools_configure_set_press_iir_coeff ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    if ( strcmp ( cmd_option, "show" ) == 0 )
    {
        return cli_tools_configure_show ( pcWriteBuffer, xWriteBufferLen, str_option_arg );
    }

    sprintf ( pcWriteBuffer, "Command [%s] not recognized\n", cmd_option );
    return false;
}


bool cli_tools_configure_set_data_rate ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "set_data_rate";
    uint8_t value = atoi ( str_option_arg );

    if ( value > 0 && value <= 100 )
    {
        sprintf ( pcWriteBuffer, "Setting data rate to %d Hz\n", value );
        FlightSystemConfiguration configuration;
        if ( MEM_OK == memory_manager_get_system_configurations ( &configuration ) )
        {
//                transmit_line(uart,output);
//                config->values.data_rate = 1000/value;
            return true;
        }
        sprintf ( pcWriteBuffer, "[%s]: Failed to read system configurations! Abort...\n", cmd_option );
        return false;
    }

    sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is in invalid range", cmd_option, str_option_arg );
    return false;
}


bool cli_tools_configure_set_initial_time ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "set_data_rate";
    uint32_t value = atoi ( str_option_arg );

    if ( value > 0 && value <= 10000000 )
    {

        sprintf ( pcWriteBuffer, "Setting initial time to wait to %lu ms\n", value );
        FlightSystemConfiguration configuration;
        if ( MEM_OK == memory_manager_get_system_configurations ( &configuration ) )
        {
//                transmit_line(uart,output);
//                config->values.initial_time_to_wait = value;
            return true;
        }

        sprintf ( pcWriteBuffer, "[%s]: Failed to read system configurations! Abort...\n", cmd_option );
        return false;
    }

    sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is in invalid range", cmd_option, str_option_arg );
    return false;
}


bool cli_tools_configure_set_record_to_flash ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option         = "set_record_to_flash";
    FlightSystemConfiguration configuration;
    uint8_t                   value = atoi ( str_option_arg );

    sprintf ( pcWriteBuffer, "Setting initial time to wait to %d ms\n", value );

    switch ( value )
    {
        case 0:
            sprintf ( pcWriteBuffer, "Turning off flash recording\n" );
            if ( MEM_OK == memory_manager_get_system_configurations ( &configuration ) )
            {
//                    transmit_line(uart,output);
//                    config->values.flags &= ~(0x02);
                return true;
            }

            sprintf ( pcWriteBuffer, "[%s]: Failed to read system configurations! Abort...\n", cmd_option );
            return false;
        case 1:
            sprintf ( pcWriteBuffer, "Turning on flash recording\n" );
            if ( MEM_OK == memory_manager_get_system_configurations ( &configuration ) )
            {
//                    transmit_line(uart,output);
//                    config->values.flags |= (0x02);
                return true;
            }

            sprintf ( pcWriteBuffer, "[%s]: Failed to read system configurations! Abort...\n", cmd_option );
            return false;
        default:
            sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.", cmd_option, str_option_arg );
            return false;
    }
}

bool cli_tools_configure_set_accel_bw ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "set_accel_bw";
    uint8_t value = atoi ( str_option_arg );

    switch ( value )
    {

        case 0:
            sprintf ( pcWriteBuffer, "Setting accelerometer to no over-sampling\n" );
//                transmit_line(uart,output);
//                config->values.ac_bw = BMI08X_ACCEL_BW_NORMAL;
            return true;
        case 2:
            sprintf ( pcWriteBuffer, "Setting accelerometer to 2x over-sampling\n" );
//                transmit_line(uart,output);
//                config->values.ac_bw = BMI08X_ACCEL_BW_OSR2;
            return true;
        case 4:
            sprintf ( pcWriteBuffer, "Setting accelerometer to 4x over-sampling\n" );
//                transmit_line(uart,output);
//                config->values.ac_bw = BMI08X_ACCEL_BW_OSR4;
            return true;
        default:
            sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.", cmd_option, str_option_arg );
            return false;
    }
}

bool cli_tools_configure_set_accel_range ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "set_accel_range";
    uint8_t value = atoi ( str_option_arg );

    switch ( value )
    {

        case 3:
            sprintf ( pcWriteBuffer, "Setting accelerometer range to %dg\n", value );
//                transmit_line(uart,output);
//                config->values.ac_range = BMI088_ACCEL_RANGE_3G;
            return true;

        case 6:
            sprintf ( pcWriteBuffer, "Setting accelerometer range to %dg\n", value );
//                transmit_line(uart,output);
//                config->values.ac_range = BMI088_ACCEL_RANGE_6G;
            return true;

        case 12:
            sprintf ( pcWriteBuffer, "Setting accelerometer range to %dg\n", value );
//                transmit_line(uart,output);
//                config->values.ac_range = BMI088_ACCEL_RANGE_12G;
            return true;

        case 24:
            sprintf ( pcWriteBuffer, "Setting accelerometer range to %dg\n", value );
//                transmit_line(uart,output);
//                config->values.ac_range = BMI088_ACCEL_RANGE_24G;
            return true;
        default:
            sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.", cmd_option, str_option_arg );
            return false;
    }
}

bool cli_tools_configure_set_accel_odr ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "set_accel_odr";
    uint16_t value = atoi ( str_option_arg );

    switch ( value )
    {

        case 12:
            sprintf ( pcWriteBuffer, "Setting accelerometer odr to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.ac_odr = BMI08X_ACCEL_ODR_12_5_HZ;
            return true;

        case 25:
            sprintf ( pcWriteBuffer, "Setting accelerometer odr to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.ac_odr = BMI08X_ACCEL_ODR_25_HZ;
            return true;

        case 50:
            sprintf ( pcWriteBuffer, "Setting accelerometer odr to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.ac_odr = BMI08X_ACCEL_ODR_50_HZ;
            return true;

        case 100:
            sprintf ( pcWriteBuffer, "Setting accelerometer odr to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.ac_odr = BMI08X_ACCEL_ODR_100_HZ;
            return true;

        case 200:
            sprintf ( pcWriteBuffer, "Setting accelerometer odr to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.ac_odr = BMI08X_ACCEL_ODR_200_HZ;
            return true;

        case 400:
            sprintf ( pcWriteBuffer, "Setting accelerometer odr to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.ac_odr = BMI08X_ACCEL_ODR_400_HZ;
            return true;

        case 800:
            sprintf ( pcWriteBuffer, "Setting accelerometer odr to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.ac_odr = BMI08X_ACCEL_ODR_800_HZ;
            return true;

        case 1600:
            sprintf ( pcWriteBuffer, "Setting accelerometer odr to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.ac_odr = BMI08X_ACCEL_ODR_1600_HZ;
            return true;
        default:
            sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.", cmd_option, str_option_arg );
            return false;
    }
}

bool cli_tools_configure_set_gyro_bw ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "set_gyro_bw ";
    uint8_t value = atoi ( str_option_arg );

    switch ( value )
    {

        case 1:
            sprintf ( pcWriteBuffer, "Setting gyroscope bandwidth to BW_32_ODR_100_HZ\n" );
//                transmit_line(uart,pcWriteBuffer);
//                config->values.gy_odr = BMI08X_GYRO_BW_32_ODR_100_HZ;
//                config->values.gy_bw = BMI08X_GYRO_BW_32_ODR_100_HZ;
            return true;

        case 2:
            sprintf ( pcWriteBuffer, "Setting gyroscope bandwidth to BW_64_ODR_200_HZ\n" );
//                transmit_line(uart,pcWriteBuffer);
//                config->values.gy_odr = BMI08X_GYRO_BW_64_ODR_200_HZ;
//                config->values.gy_bw = BMI08X_GYRO_BW_64_ODR_200_HZ;
            return true;

        case 3:
            sprintf ( pcWriteBuffer, "Setting gyroscope bandwidth to BW_12_ODR_100_HZ\n" );
//                transmit_line(uart,pcWriteBuffer);
//                config->values.gy_odr = BMI08X_GYRO_BW_12_ODR_100_HZ;
//                config->values.gy_bw = BMI08X_GYRO_BW_12_ODR_100_HZ;
            return true;

        case 4:
            sprintf ( pcWriteBuffer, "Setting gyroscope bandwidth to BW_23_ODR_200_HZ\n" );
//                transmit_line(uart,pcWriteBuffer);
//                config->values.gy_odr = BMI08X_GYRO_BW_23_ODR_200_HZ;
//                config->values.gy_bw = BMI08X_GYRO_BW_23_ODR_200_HZ;
            return true;

        case 5:
            sprintf ( pcWriteBuffer, "Setting gyroscope bandwidth to BW_47_ODR_400_HZ\n" );
//                transmit_line(uart,pcWriteBuffer);
//                config->values.gy_odr = BMI08X_GYRO_BW_47_ODR_400_HZ;
//                config->values.gy_bw = BMI08X_GYRO_BW_47_ODR_400_HZ;
            return true;

        case 6:
            sprintf ( pcWriteBuffer, "Setting gyroscope bandwidth to BW_116_ODR_1000_HZ\n" );
//                transmit_line(uart,pcWriteBuffer);
//                config->values.gy_odr = BMI08X_GYRO_BW_116_ODR_1000_HZ;
//                config->values.gy_bw = BMI08X_GYRO_BW_116_ODR_1000_HZ;
            return true;

        case 7:
            sprintf ( pcWriteBuffer, "Setting gyroscope bandwidth to BW_230_ODR_2000_HZ\n" );
//                transmit_line(uart,pcWriteBuffer);
//                config->values.gy_odr = BMI08X_GYRO_BW_230_ODR_2000_HZ;
//                config->values.gy_bw = BMI08X_GYRO_BW_230_ODR_2000_HZ;
            return true;

        case 8:
            sprintf ( pcWriteBuffer, "Setting gyroscope bandwidth to BW_532_ODR_2000_HZ\n" );
//                transmit_line(uart,pcWriteBuffer);
//                config->values.gy_odr = BMI08X_GYRO_BW_532_ODR_2000_HZ;
//                config->values.gy_bw = BMI08X_GYRO_BW_532_ODR_2000_HZ;
            return true;
        default:
            sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.", cmd_option, str_option_arg );
            return false;
    }
}

bool cli_tools_configure_set_gyro_range ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "set_gyro_range ";
    int value = atoi ( str_option_arg );

    switch ( value )
    {

        case 125:
            sprintf ( pcWriteBuffer, "Setting gyroscope range to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.gy_range = BMI08X_GYRO_RANGE_125_DPS;
            return true;

        case 250:
            sprintf ( pcWriteBuffer, "Setting gyroscope range to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.gy_range = BMI08X_GYRO_RANGE_250_DPS;
            return true;

        case 500:
            sprintf ( pcWriteBuffer, "Setting gyroscope range to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.gy_range = BMI08X_GYRO_RANGE_500_DPS;
            return true;

        case 1000:
            sprintf ( pcWriteBuffer, "Setting gyroscope range to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.gy_range = BMI08X_GYRO_RANGE_1000_DPS;
            return true;

        case 2000:
            sprintf ( pcWriteBuffer, "Setting gyroscope range to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.gy_range = BMI08X_GYRO_RANGE_2000_DPS;
            return true;
        default:
            sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.", cmd_option, str_option_arg );
            return false;
    }
}

bool cli_tools_configure_set_press_odr ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "set_press_odr";
    int value = atoi ( str_option_arg );

    switch ( value )
    {

        case 1:
            sprintf ( pcWriteBuffer, "Setting bmp output data rate to 1.5Hz\n" );
//                transmit_line(uart,output);
//                config->values.bmp_odr = BMP3_ODR_1_5_HZ;
            return true;

        case 12:
            sprintf ( pcWriteBuffer, "Setting bmp output data rate to 12.5Hz\n" );
//                transmit_line(uart,output);
//                config->values.bmp_odr = BMP3_ODR_12_5_HZ;
            return true;

        case 25:
            sprintf ( pcWriteBuffer, "Setting bmp output data rate to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.bmp_odr = BMP3_ODR_25_HZ;
            return true;

        case 50:
            sprintf ( pcWriteBuffer, "Setting bmp output data rate to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.bmp_odr = BMP3_ODR_50_HZ;
            return true;

        case 100:
            sprintf ( pcWriteBuffer, "Setting bmp output data rate to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.bmp_odr = BMP3_ODR_100_HZ;
            return true;

        case 200:
            sprintf ( pcWriteBuffer, "Setting bmp output data rate to %dHz\n", value );
//                transmit_line(uart,output);
//                config->values.bmp_odr = BMP3_ODR_200_HZ;
            return true;
        default:
            sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.", cmd_option, str_option_arg );
            return false;
    }
}

bool cli_tools_configure_set_press_os ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "set_press_os";
    int value = atoi ( str_option_arg );

    switch ( value )
    {

        case 0:
            sprintf ( pcWriteBuffer, "Setting pressure oversampling to 0x Hz\n" );
//                transmit_line(uart,output);
//                config->values.pres_os =  BMP3_NO_OVERSAMPLING;
            return true;

        case 2:
            sprintf ( pcWriteBuffer, "Setting pressure oversampling to 2x Hz\n" );
//                transmit_line(uart,output);
//                config->values.pres_os = BMP3_OVERSAMPLING_2X;
            return true;

        case 4:
            sprintf ( pcWriteBuffer, "Setting pressure oversampling to 4x Hz\n" );
//                transmit_line(uart,output);
//                config->values.pres_os = BMP3_OVERSAMPLING_4X;
            return true;

        case 8:
            sprintf ( pcWriteBuffer, "Setting pressure oversampling to 8x Hz\n" );
//                transmit_line(uart,output);
//                config->values.pres_os = BMP3_OVERSAMPLING_8X;
            return true;

        case 16:
            sprintf ( pcWriteBuffer, "Setting pressure oversampling to 16x Hz\n" );
//                transmit_line(uart,output);
//                config->values.pres_os = BMP3_OVERSAMPLING_16X;
            return true;

        case 32:
            sprintf ( pcWriteBuffer, "Setting pressure oversampling to 32x Hz\n" );
//                transmit_line(uart,output);
//                config->values.pres_os = BMP3_OVERSAMPLING_32X;
            return true;
        default:
            sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.", cmd_option, str_option_arg );
            return false;
    }
}

bool cli_tools_configure_set_temp_os ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "set_temp_os";
    int value = atoi ( str_option_arg );

    switch ( value )
    {

        case 0:
            sprintf ( pcWriteBuffer, "Setting temperature oversampling 0x\n" );
//                transmit_line(uart,output);
//                config->values.temp_os =  BMP3_NO_OVERSAMPLING;
            return true;

        case 2:
            sprintf ( pcWriteBuffer, "Setting temperature oversampling 2x \n" );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_OVERSAMPLING_2X;
            return true;

        case 4:
            sprintf ( pcWriteBuffer, "Setting temperature oversampling 4x\n" );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_OVERSAMPLING_4X;
            return true;

        case 8:
            sprintf ( pcWriteBuffer, "Setting temperature oversampling to 8x Hz\n" );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_OVERSAMPLING_8X;
            return true;

        case 16:
            sprintf ( pcWriteBuffer, "Setting temperature oversampling to 16x Hz\n" );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_OVERSAMPLING_16X;
            return true;

        case 32:
            sprintf ( pcWriteBuffer, "Setting temperature oversampling to 32x Hz\n" );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_OVERSAMPLING_32X;
            return true;
        default:
            sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.", cmd_option, str_option_arg );
            return false;
    }
}

bool cli_tools_configure_set_press_iir_coeff ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "press_iir_coeff";
    int value = atoi ( str_option_arg );

    switch ( value )
    {

        case 0:
            sprintf ( pcWriteBuffer, "Setting bmp IIR filter coefficient to off\n" );
//                transmit_line(uart,output);
//                config->values.temp_os =  BMP3_IIR_FILTER_DISABLE;
            return true;

        case 1:
            sprintf ( pcWriteBuffer, "Setting bmp IIR filter coefficient to %d\n", value );
//                transmit_line(uart,output);
//                config->values.temp_os =  BMP3_IIR_FILTER_COEFF_1;
            return true;

        case 3:
            sprintf ( pcWriteBuffer, "Setting bmp IIR filter coefficient to %d\n", value );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_IIR_FILTER_COEFF_3;
            return true;

        case 7:
            sprintf ( pcWriteBuffer, "Setting bmp IIR filter coefficient to %d\n", value );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_IIR_FILTER_COEFF_7;
            return true;

        case 15:
            sprintf ( pcWriteBuffer, "Setting bmp IIR filter coefficient to %d\n", value );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_IIR_FILTER_COEFF_15;
            return true;

        case 31:
            sprintf ( pcWriteBuffer, "Setting bmp IIR filter coefficient to %d\n", value );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_IIR_FILTER_COEFF_31;
            return true;

        case 63:
            sprintf ( pcWriteBuffer, "Setting bmp IIR filter coefficient to %d\n", value );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_IIR_FILTER_COEFF_63;
            return true;


        case 127:
            sprintf ( pcWriteBuffer, "Setting bmp IIR filter coefficient to %d\n", value );
//                transmit_line(uart,output);
//                config->values.temp_os = BMP3_IIR_FILTER_COEFF_127;
            return true;
        default:
            sprintf ( pcWriteBuffer, "[%s]: Argument [%s] is invalid.", cmd_option, str_option_arg );
            return false;
    }
}

bool cli_tools_configure_show ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * str_option_arg )
{
    const char * cmd_option = "show";
    sprintf ( pcWriteBuffer, "The current settings (not in flash):\n" );
//        transmit_line(uart,output);

//        sprintf(pcWriteBuffer,"ID: %d \tIntitial Time To Wait: %ld \r\n", config->values.id, config->values.initial_time_to_wait);
//        transmit_line(uart,output);

//        sprintf(pcWriteBuffer,"data rate: %d Hz \tSet to record: %d \r\n", 1000 / config->values.data_rate, IS_RECORDING (config->values.flags));
//        transmit_line(uart,output);

//        sprintf(pcWriteBuffer,"start of data: %ld \tend of data: %ld \r\n",config->values.start_data_address,config->values.end_data_address);
//        transmit_line(uart,output);

//        sprintf(pcWriteBuffer,"reference altitude: %ld \t reference pressure: %ld \r\n",(uint32_t)config->values.ref_alt,(uint32_t)config->values.ref_pres);
//        transmit_line(uart,output);
    return true;
}