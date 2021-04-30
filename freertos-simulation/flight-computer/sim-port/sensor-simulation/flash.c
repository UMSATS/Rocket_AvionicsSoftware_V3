//
// Created by vasil on 20/11/2020.
//

#include "board/components/flash.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>


/*
 *  Macros for flash commands
 */
typedef enum flash_command_t
{
    FLASH_READ_ID                         = 0x09F,
    FLASH_WRITE_ENABLE                    = 0x06, // Write Enable
    FLASH_WRITE                           = 0x02, // Page Program Command (write)
    FLASH_READ                            = 0x03,
    FLASH_ERASE_SECTOR                    = 0xD8,
    FLASH_ERASE_PARAM_SECTOR              = 0x20,
    FLASH_GET_STATUS_REGISTER             = 0x05,
    FLASH_ERASE_DEVICE                    = 0x60 // Command to erase the whole device.
} FlashCommand;



#define PAGE_SIZE   256       // 256 bytes per page
#define FLASH_SIZE  0X1FFFFFF // 65 MB

const char FILE_NAME[] = "myFlash.bin"; // The file that will represent the flash


static inline void initialize_flash_disk( void )
{
    if ( access( FILE_NAME, F_OK ) != -1 )
    {
        FILE * writePrt = fopen( FILE_NAME, "ab+" ); // Open the file new, all old contents are erased
        if ( !writePrt )
        {
            perror( "fopen" );
        }
        fseek( writePrt, 0, SEEK_CUR );
        fclose( writePrt );

    } else
    {
        FILE * writePrt = fopen( FILE_NAME, "wb+" ); // Open the file new, all old contents are erased
        if ( !writePrt )
        {
            perror( "fopen" );
        }

        // Create or over write the file with all 0xff for the specified size
        fseek( writePrt, FLASH_SIZE, SEEK_SET );
        fputc( '\0', writePrt );
        fclose( writePrt );
    }
}



static inline uint32_t program_page( uint32_t address, uint8_t * data_buffer, uint16_t num_bytes )
{
    FILE * writePrt = fopen( FILE_NAME, "rb+" ); // File must exist
    int32_t bytesWritten = 0;

    if ( address > FLASH_SIZE )
    {
        return FLASH_ERR;
    }


    // Go to te given position of memory address (inside he file), fseek return zero if able to seek or non-zero if failed
    fseek( writePrt, address, SEEK_SET );

    // Overwrite the block of memory with given contents
    bytesWritten = fwrite( data_buffer, sizeof( uint8_t ), num_bytes, writePrt );

    fclose( writePrt );

    // Return operation flag
    return bytesWritten == num_bytes ? FLASH_OK : FLASH_ERR;
}



static inline uint32_t read_page( uint32_t address, uint8_t * data_buffer, uint16_t num_bytes )
{
    FILE * readPrt = fopen( FILE_NAME, "rb" );

    if ( address > FLASH_SIZE )
    {
        return FLASH_ERR;
    }
    int32_t bytesRead = 0;

    // Go to the given position of memory address
    fseek( readPrt, address, SEEK_SET ); // Go to the position that is number of bytes which is address from the beginning of file

    // Read the given address and number of bytes to read
    bytesRead = fread( data_buffer, sizeof( uint8_t ), num_bytes, readPrt );

    fclose( readPrt );

    perror("read");

    // Return operation flag
    return bytesRead == num_bytes ? FLASH_OK : FLASH_ERR;
}


static FlashReturnType prvExecuteCommand ( uint32_t address, FlashCommand command, uint8_t * data_buffer, uint16_t num_bytes )
{
    if(command == FLASH_WRITE)
    {
        return program_page(address, data_buffer, num_bytes);
    }
    else if (command == FLASH_READ)
    {
        return read_page( address, data_buffer, num_bytes );
    }

    return FLASH_OK;
}




FlashStatus flash_erase_sector( uint32_t address )
{
    return prvExecuteCommand(address, FLASH_ERASE_SECTOR, NULL, 0);
}



FlashStatus flash_erase_param_sector( uint32_t address )
{
    return prvExecuteCommand(address, FLASH_ERASE_PARAM_SECTOR, NULL, 0);
}



FlashReturnType flash_write( uint32_t address, uint8_t * data_buffer, uint16_t num_bytes )
{
    return prvExecuteCommand(address, FLASH_WRITE, data_buffer, num_bytes);
}



FlashReturnType flash_read( uint32_t address, uint8_t * data_buffer, uint16_t num_bytes )
{
    return prvExecuteCommand(address, FLASH_READ, data_buffer, num_bytes);
}



FlashStatus flash_erase_device( )
{
    return prvExecuteCommand(0, FLASH_ERASE_DEVICE, NULL, 0);
}


FlashStatus flash_erase_4Kb_subsector(uint32_t address)
{
    return FLASH_OK;
}


FlashStatus flash_check_id( )
{
    return FLASH_OK;

}



FlashStatus flash_init( )
{
    initialize_flash_disk ( );
    return FLASH_OK;
}



size_t flash_scan( )
{
    return FLASH_SIZE;
}