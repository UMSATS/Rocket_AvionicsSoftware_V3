/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "e-match.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "protocols/UART.h"
#include "board/components/recovery.h"
#include "core/system_configuration.h"


TickType_t xDelay;


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


bool cli_tools_EMatch ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * cmd_option, const char * str_option_arg )
{
    if ( strcmp ( cmd_option, "e-match" ) == 0 )
    {
        sprintf ( pcWriteBuffer, "%s", str_option_arg );
        return true;
    }

    if ( strcmp ( cmd_option, "check_continuity_drogue" ) == 0 )
    {
        RecoveryContinuityStatus status = recoveryCheckContinuity(RecoverySelectDrogueParachute);
        sprintf ( pcWriteBuffer, "Recovery drogue continuity status: ");
        char * result;
        if (status == RecoveryContinuityStatusOpenCircuit)
        {
            result = "Open Circuit";
        }
        else if (status == RecoveryContinuityStatusShortCircuit)
        {
            result = "Closed Circuit";
        }
        else
        {
            result = "Not Obtained";
            sprintf ( pcWriteBuffer, "%s\n", result);
            return false;
        }
        sprintf ( pcWriteBuffer, "%s\n", result);
        return true;
    }

    if ( strcmp ( cmd_option, "check_continuity_main" ) == 0 )
    {
        RecoveryContinuityStatus status = recoveryCheckContinuity(RecoverySelectMainParachute);
        sprintf ( pcWriteBuffer, "Recovery main continuity status: ");
        char * result;
        if (status == RecoveryContinuityStatusOpenCircuit)
        {
            result = "Open Circuit";
        }
        else if (status == RecoveryContinuityStatusShortCircuit)
        {
            result = "Closed Circuit";
        }
        else
        {
            result = "Not Obtained";
            sprintf ( pcWriteBuffer, "%s\n", result);
            return false;
        }
        sprintf ( pcWriteBuffer, "%s\n", result);
        return true;
    }

    if ( strcmp ( cmd_option, "check_overcurrent_drogue" ) == 0 )
    {
        RecoveryOverCurrentStatus status = recoveryCheckOverCurrent(RecoverySelectDrogueParachute);
        sprintf ( pcWriteBuffer, "Recovery drogue continuity status: ");
        char * result;
        if (status == RecoveryOverCurrentStatusNoOverCurrent)
        {
            result = "No Over Current";
        }
        else if (status == RecoveryOverCurrentStatusOverCurrent)
        {
            result = "Over Current";
        }
        else
        {
            result = "Not Obtained";
            sprintf ( pcWriteBuffer, "%s\n", result);
            return false;
        }
        sprintf ( pcWriteBuffer, "%s\n", result);
        return true;
    }

    if ( strcmp ( cmd_option, "check_overcurrent_main" ) == 0 )
    {
        RecoveryOverCurrentStatus status = recoveryCheckOverCurrent(RecoverySelectMainParachute);
        sprintf ( pcWriteBuffer, "Recovery main continuity status: ");
        char * result;
        if (status == RecoveryOverCurrentStatusNoOverCurrent)
        {
            result = "No Over Current";
        }
        else if (status == RecoveryOverCurrentStatusOverCurrent)
        {
            result = "Over Current";
        }
        else
        {
            result = "Not Obtained";
            sprintf ( pcWriteBuffer, "%s\n", result);
            return false;
        }
        sprintf ( pcWriteBuffer, "%s\n", result);
        return true;
    }

    if ( strcmp ( cmd_option, "enable_drogue" ) == 0 )
    {
        recoveryEnableMOSFET(RecoverySelectDrogueParachute);
        sprintf ( pcWriteBuffer, "Recovery drogue enabled\n");
        return true;
    }

    if ( strcmp ( cmd_option, "enable_main" ) == 0 )
    {
        recoveryEnableMOSFET(RecoverySelectMainParachute);
        sprintf ( pcWriteBuffer, "Recovery main enabled\n");
        return true;
    }

    if ( strcmp ( cmd_option, "fire_drogue" ) == 0 )
    {
        vTaskDelay(xDelay);
        recoveryActivateMOSFET(RecoverySelectDrogueParachute);
        sprintf ( pcWriteBuffer, "Recovery drogue fired\n");
        return true;
    }

    if ( strcmp ( cmd_option, "fire_main" ) == 0 )
    {
        vTaskDelay(xDelay);
        recoveryActivateMOSFET(RecoverySelectMainParachute);
        sprintf ( pcWriteBuffer, "Recovery main fired\n");
        return true;
    }

    if ( strcmp ( cmd_option, "set_delay" ) == 0 )
    {
        int arg_val = atoi ( str_option_arg );
        if (arg_val == 0)
        {
            sprintf ( pcWriteBuffer, "Invalid parameter: %s\n", str_option_arg);
            return false;
        }
        else
        {
            xDelay = pdMS_TO_TICKS (arg_val); //atoui ( str_option_arg ) / portTICK_PERIOD_MS;
            sprintf ( pcWriteBuffer, "E-Match delay set to %dHz\n", arg_val );
        }

        return true;
    }

    sprintf ( pcWriteBuffer, "Command [%s] not recognized\r\n", cmd_option );
    return false;
}