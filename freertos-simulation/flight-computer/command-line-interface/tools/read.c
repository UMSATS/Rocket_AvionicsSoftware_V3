/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "read.h"

#include <stdio.h>
#include <stdint.h>
#include <memory.h>

#include "protocols/UART.h"
#include "board/components/flash.h"


bool cli_tools_read ( char * pcWriteBuffer, size_t xWriteBufferLen )
{
    strcpy ( pcWriteBuffer, "Data transfer will start in 20 seconds. The LED will turn off when the transfer is complete." );

    uint8_t buffer[256 * 5]; //Read 5 pages from flash at a time;

    uint32_t bytesRead      = 0;
    uint32_t currentAddress = FLASH_START_ADDRESS;

    // vTaskDelay(pdMS_TO_TICKS(1000 * 10)); //Delay 10 seconds

    uint32_t endAddress = flash_scan ( );
    while ( bytesRead < endAddress )
    {
        flash_read ( currentAddress, buffer, 256 * 5 );

        uint16_t       empty = 0;
        for ( uint16_t i     = 0; i < 256 * 5; i++ )
        {
            if ( buffer[ i ] == 0xFF )
            {
                empty += 1;
            }
        }

        if ( empty == ( 256 * 5 ) )
        {
            break;
        }

        uart6_transmit_bytes ( buffer, 256 * 5 );

        currentAddress += ( 256 * 5 );
        currentAddress       = currentAddress % FLASH_SIZE_BYTES;

        bytesRead += 256 * 5;
        vTaskDelay ( 1 );
    }

    return true;
}