/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "save.h"

#include <stdio.h>
#include <stdint.h>
#include <memory.h>

#include "protocols/UART.h"
#include "board/components/flash.h"
#include "memory-management/memory_manager.h"


bool cli_tools_save ( char * pcWriteBuffer, size_t xWriteBufferLen )
{
    strcpy ( pcWriteBuffer, "Data transfer will start in 20 seconds. The LED will turn off when the transfer is complete." );

    //prvMemorySystemSectorWritePageNow(SystemSectorGlobalConfigurationData, prvGlobalConfigurationDiskSnapshot.bytes);
    //memory_manager_set_system_configurations();

    return true;
}