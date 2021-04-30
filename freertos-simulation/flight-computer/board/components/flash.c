#include <stdint.h>
#include "board/board.h"
#include <math.h>
#include <string.h>

#include "flash.h"
#include "board/hardware_definitions.h"
#include "protocols/SPI.h"

#include <assert.h>

/*
 *  Macros for flash commands
 */
typedef enum flash_command_t
{
    FLASH_READ_ID             = 0x09F,
    FLASH_WRITE_ENABLE        = 0x06, // Write Enable
    FLASH_WRITE               = 0x02, // Page Program Command (write)
    FLASH_READ                = 0x03,
    FLASH_ERASE_64KB_SECTOR   = 0xD8,
    FLASH_ERASE_4KB_SUBSECTOR = 0x20,
    FLASH_GET_STATUS_REGISTER = 0x05,
    FLASH_ERASE_ENTIRE_DEVICE = 0x60 // Command to erase the whole device.
} FlashCommand;


static FlashReturnType prvExecuteCommand ( uint32_t address, FlashCommand command, uint8_t * data_buffer, uint16_t num_bytes );


static FlashStatus prvWaitForLastOperationToFinish ( )
{
    uint8_t status_reg = 0;
    if ( FLASH_OK != prvExecuteCommand ( 0, FLASH_GET_STATUS_REGISTER, &status_reg, 1 ) )
    {
        return FLASH_ERR;
    }


    while ( FLASH_IS_DEVICE_BUSY ( status_reg ) )
    {
        if ( FLASH_OK != prvExecuteCommand ( 0, FLASH_GET_STATUS_REGISTER, &status_reg, 1 ) )
        {
            return FLASH_ERR;
        }
    }

    return FLASH_OK;
}


static FlashReturnType prvExecuteCommand ( uint32_t address, FlashCommand command, uint8_t * data_buffer, uint16_t num_bytes )
{
    switch ( command )
    {
        case FLASH_READ_ID:
        {
            uint8_t command_buffer = command;
            if ( SPI_OK == spi1_receive ( &command_buffer, 1, data_buffer, num_bytes, 10 ) )
            {
                return FLASH_OK;
            }
            else
            {
                return FLASH_ERR;
            }
        }
        case FLASH_GET_STATUS_REGISTER:
        {
            uint8_t command_buffer = command;
            if ( SPI_OK == spi1_receive ( &command_buffer, 1, data_buffer, num_bytes, 10 ) )
            {
                return FLASH_OK;
            }
            else
            {
                return FLASH_ERR;
            }
        }
        case FLASH_WRITE_ENABLE:
        {
            uint8_t command_buffer = command;
            if ( SPI_OK == spi1_transmit ( &command_buffer, NULL, 1, 10 ) )
            {
                return FLASH_OK;
            }
            else
            {
                return FLASH_ERR;
            }
        }
        case FLASH_READ:
        {
            uint8_t command_buffer[] = {
                    command, ( address & ( FLASH_HIGH_BYTE_MASK_24B ) ) >> 16,
                    ( address & ( FLASH_MID_BYTE_MASK_24B ) ) >> 8, address & ( FLASH_LOW_BYTE_MASK_24B )
            };
            if ( SPI_OK == spi1_receive ( command_buffer, 4, data_buffer, num_bytes, 200 ) )
            {
                return FLASH_OK;
            }
            else
            {
                return FLASH_ERR;
            }
        }
        case FLASH_WRITE:
        {
            if ( FLASH_OK != prvExecuteCommand ( 0, FLASH_WRITE_ENABLE, NULL, 0 ) )
            {
                return FLASH_ERR;
            }

            if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
            {
                return FLASH_ERR;
            }

            uint8_t command_buffer[] = {
                    command, ( address & ( FLASH_HIGH_BYTE_MASK_24B ) ) >> 16,
                    ( address & ( FLASH_MID_BYTE_MASK_24B ) ) >> 8, address & ( FLASH_LOW_BYTE_MASK_24B )
            };
            if ( SPI_OK == spi1_send ( command_buffer, 4, data_buffer, num_bytes, 200 ) )
            {
                return FLASH_OK;
            }
            else
            {
                return FLASH_ERR;
            }
        }
        case FLASH_ERASE_64KB_SECTOR:
        {
            if ( FLASH_OK != prvExecuteCommand ( 0, FLASH_WRITE_ENABLE, NULL, 0 ) )
            {
                return FLASH_ERR;
            }

            if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
            {
                return FLASH_ERR;
            }

            uint8_t command_buffer[] = {
                    command, ( address & ( FLASH_HIGH_BYTE_MASK_24B ) ) >> 16,
                    ( address & ( FLASH_MID_BYTE_MASK_24B ) ) >> 8, address & ( FLASH_LOW_BYTE_MASK_24B )
            };
            if ( SPI_OK == spi1_send ( command_buffer, 4, NULL, 0, 10 ) )
            {
                return FLASH_OK;
            }
            else
            {
                return FLASH_ERR;
            }
        }
        case FLASH_ERASE_4KB_SUBSECTOR:
        {
            if ( FLASH_OK != prvExecuteCommand ( 0, FLASH_WRITE_ENABLE, NULL, 0 ) )
            {
                return FLASH_ERR;
            }

            if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
            {
                return FLASH_ERR;
            }

            uint8_t command_buffer[] = {
                    command, ( address & ( FLASH_HIGH_BYTE_MASK_24B ) ) >> 16,
                    ( address & ( FLASH_MID_BYTE_MASK_24B ) ) >> 8, address & ( FLASH_LOW_BYTE_MASK_24B )
            };
            if ( SPI_OK == spi1_send ( command_buffer, 4, NULL, 0, 10 ) )
            {
                return FLASH_OK;
            }
            else
            {
                return FLASH_ERR;
            }
        }
        case FLASH_ERASE_ENTIRE_DEVICE:
        {
            if ( FLASH_OK != prvExecuteCommand ( 0, FLASH_WRITE_ENABLE, NULL, 0 ) )
            {
                return FLASH_ERR;
            }

            if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
            {
                return FLASH_ERR;
            }

            uint8_t command_buffer = command;
            if ( SPI_OK == spi1_send ( &command_buffer, 1, NULL, 0, 10 ) )
            {
                return FLASH_OK;
            }
            else
            {
                return FLASH_ERR;
            }
        }
    }

    return FLASH_ERR;
}


FlashStatus flash_erase_64kb_sector ( uint32_t address )
{
    if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
    {
        return FLASH_ERR;
    }

    return prvExecuteCommand ( address, FLASH_ERASE_64KB_SECTOR, NULL, 0 );
}


FlashStatus flash_erase_4Kb_subsector ( uint32_t address )
{
    if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
    {
        return FLASH_ERR;
    }

    return prvExecuteCommand ( address, FLASH_ERASE_4KB_SUBSECTOR, NULL, 0 );
}

FlashStatus flash_write_range ( uint32_t begin_address, uint8_t * data, uint32_t size )
{
    assert( size > 0 );

    if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
    {
        return FLASH_ERR;
    }

    if ( size > FLASH_PAGE_SIZE )
    {
        uint32_t numberOfPages = ceil ( ( double ) size / FLASH_PAGE_SIZE );

        for ( int i = 0; i < numberOfPages; i++ )
        {
            begin_address += ( i * FLASH_PAGE_SIZE );

            if ( FLASH_OK != prvExecuteCommand ( begin_address, FLASH_WRITE, data, FLASH_PAGE_SIZE ) )
            {
                return FLASH_ERR;
            }
        }

        uint32_t remainingBytes = size - ( numberOfPages * FLASH_PAGE_SIZE );

        if ( FLASH_OK != prvExecuteCommand ( begin_address, FLASH_WRITE, data, remainingBytes ) )
        {
            return FLASH_ERR;
        }

        return FLASH_OK;
    }
    else
    {
        return prvExecuteCommand ( begin_address, FLASH_WRITE, data, size );
    }
}


FlashReturnType flash_write ( uint32_t address, uint8_t * data_buffer, uint16_t num_bytes )
{
    if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
    {
        return FLASH_ERR;
    }

    return prvExecuteCommand ( address, FLASH_WRITE, data_buffer, num_bytes );
}


FlashReturnType flash_read ( uint32_t address, uint8_t * data_buffer, uint16_t num_bytes )
{
    if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
    {
        return FLASH_ERR;
    }

    return prvExecuteCommand ( address, FLASH_READ, data_buffer, num_bytes );
}


FlashStatus flash_erase_device ( )
{
    if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
    {
        return FLASH_ERR;
    }

    return prvExecuteCommand ( 0, FLASH_ERASE_ENTIRE_DEVICE, NULL, 0 );
}


FlashStatus flash_check_id ( )
{
    if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
    {
        return FLASH_ERR;
    }

    uint8_t id[3] = { 0, 0, 0 };

    if ( FLASH_OK == prvExecuteCommand ( 0, FLASH_READ_ID, id, 3 ) )
    {
        if ( id[ 0 ] == FLASH_MANUFACTURER_ID && id[ 1 ] == FLASH_DEVICE_ID_MSB && id[ 2 ] == FLASH_DEVICE_ID_LSB )
        {
            return FLASH_OK;
        }

        return FLASH_ERR;
    }
    else
    {
        return FLASH_ERR;
    }
}


FlashStatus flash_init ( )
{
    __HAL_RCC_GPIOB_CLK_ENABLE( );
    __HAL_RCC_GPIOC_CLK_ENABLE( );
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    //This configures the write protect pin(Active Low).
    GPIO_InitStruct.Pin       = FLASH_WP_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = 0;

    HAL_GPIO_Init ( FLASH_WP_PORT, &GPIO_InitStruct );

    GPIO_InitTypeDef GPIO_InitStruct2 = { 0 };
    //This configures the hold pin(Active Low).
    GPIO_InitStruct2.Pin       = FLASH_HOLD_PIN;
    GPIO_InitStruct2.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct2.Pull      = GPIO_NOPULL;
    GPIO_InitStruct2.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct2.Alternate = 0;

    HAL_GPIO_Init ( FLASH_HOLD_PORT, &GPIO_InitStruct2 );

    HAL_GPIO_WritePin ( FLASH_WP_PORT, FLASH_WP_PIN, GPIO_PIN_SET );
    HAL_GPIO_WritePin ( FLASH_HOLD_PORT, FLASH_HOLD_PIN, GPIO_PIN_SET );
    //Set up the SPI interface
    int status = spi1_init ( );
    if ( status != SPI_OK )
    {
        return FLASH_ERR;
    }

    HAL_GPIO_WritePin ( FLASH_SPI_CS_PORT, FLASH_SPI_CS_PIN, GPIO_PIN_SET );

    if ( FLASH_ERR == flash_check_id ( ) )
    {
        return FLASH_ERR;
    }

    return FLASH_OK;
}


size_t flash_scan ( )
{
    if ( FLASH_OK != prvWaitForLastOperationToFinish ( ) )
    {
        return FLASH_ERR;
    }

    size_t  result = 0;
    uint8_t dataRX[256];
    size_t  i;
    int     j;
    i = FLASH_START_ADDRESS;
    while ( i < FLASH_SIZE_BYTES )
    {
        FlashStatus status;
        for ( j = 0; j < 256; j++ )
        {
            dataRX[ j ] = 0;
        }

        status = flash_read ( i, dataRX, 256 );
        uint16_t empty = 0xFFFF;
        for ( j = 0; j < 256; j++ )
        {
            if ( dataRX[ j ] != 0xFF )
            {
                empty--;
            }
        }

        if ( empty == 0xFFFF )
        {
            result = i;
            break;
        }

        i = i + 256;
    }

    if ( result == 0 )
    {
        result = FLASH_SIZE_BYTES;
    }

    return result;
}





