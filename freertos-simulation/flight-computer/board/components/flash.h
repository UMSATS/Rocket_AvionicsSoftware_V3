/**
 * @file flash.h
 * @author UMSATS Rocketry Division. Joseph Howarth
 * @date 2019-03-28
 * @brief Header file for the flash memory interface.
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
 * @see https://github.com/UMSATS/Avionics-2019
 */

#ifndef FLASH_H
#define FLASH_H

#include <stdio.h>
#include <inttypes.h>

/**
 * So far flash.h must depend on SPI.h because it initializes SPI_HandleTypeDef
 * @todo: maybe in the future we can use a pointer instead then?
 */


/**
 *  @brief Macros for constants
 *  @todo I strongly consider changing all the macros that represent constants
 *  to static const type to ensure type safety! This requires a discussion
 */
#define FLASH_MANUFACTURER_ID          0x01
#define FLASH_DEVICE_ID_MSB            0x02
#define FLASH_DEVICE_ID_LSB            0x16
#define FLASH_HIGH_BYTE_MASK_24B       0x00FF0000
#define FLASH_MID_BYTE_MASK_24B        0x0000FF00
#define FLASH_LOW_BYTE_MASK_24B        0x000000FF
#define FLASH_PAGE_SIZE                256
#define FLASH_4KB_SUBSECTOR_SIZE       ( FLASH_PAGE_SIZE * 16  )
#define FLASH_64KB_SECTOR_SIZE         ( FLASH_PAGE_SIZE * 256 )
#define FLASH_START_ADDRESS            ( 0x00000000 )
#define FLASH_END_ADDRESS              ( 0x7FFFFF )
#define FLASH_SIZE_BYTES               ( FLASH_END_ADDRESS + 1 )

/*
 *  Status Reg. Bits
 */
#define FLASH_P_ERR_BIT                0x06 // Programming Error Bit.
#define FLASH_E_ERR_BIT                0x05 // Erase Error Bit.
#define FLASH_WEL_BIT                  0x01 // Write Enable Latch Bit.
#define FLASH_WIP_BIT                  0x00 // Write In Progress Bit.

#define FLASH_WAS_PROGRAMING_ERROR(x) ( ( x >> FLASH_P_ERR_BIT) & 0x01 )
#define FLASH_WAS_ERASE_ERROR(x)      ( ( x >> FLASH_E_ERR_BIT) & 0x01 )
#define FLASH_IS_WRITE_ENABLE(x)      ( ( x >> FLASH_WEL_BIT) & 0x01 )
#define FLASH_IS_DEVICE_BUSY(x)       ( ( x >> FLASH_WIP_BIT) & 0x01 )

/**
 * @brief Enumeration to indicate the result status of the operation on flash
 */
typedef enum flash_status_t
{
    FLASH_ERR = 0, /**< If the lat operation failed. */
    FLASH_OK  = 1, /**< If the last operation was successful. */
    FLASH_BUSY    /**< If the last operation couldn't start because the device was busy. */
} FlashStatus;


#if ( userconf_FREE_RTOS_SIMULATOR_MODE_ON == 0 )
typedef FlashStatus FlashReturnType;
#else
typedef uint32_t FlashReturnType;
#endif

/**
 * @brief Flash structure that holds a communication handler
 */

/**
 * @brief
 * This function sets up the flash memory.
 * Right now, this consists of setting up the SPI interface and
 * checking the ID of the flash. We could also check to make sure memory is not full etc.
 * @param p_flash Pointer to @c Flash structure
 * @return @c FLASH_OK if the setup is successful, @c FLASH_ERROR otherwise.
 * @see https://github.com/UMSATS/Avionics-2019/
 */
FlashStatus flash_init ( );

/**
 * @brief
 * This function reads the manufacturer and device IDs of the flash memory.
 * The values are checked against the correct values.
 * @param p_flash Pointer to @c Flash structure
 * @return @c FlashStatus
 * @see https://github.com/UMSATS/Avionics-2019/
 */
FlashStatus flash_check_id ( );

/**
 * @brief
 * This writes up to 256 bytes (one page) to a specified location in the flash memory.
 * If the device is busy the function exits early and returns FLASH_BUSY.
 * @param p_flash Pointer to @c Flash structure
 * @return @c FlashStatus
 * Will be FLASH_BUSY if there is another operation in progress, FLASH_OK otherwise.
 * @see https://github.com/UMSATS/Avionics-2019/
 * @note The address should be 3 bytes long (0x000000 to 0x7FFFFF).
 * @warning If the LSB of the address is not all 0, then data written past the page will wrap around!
 */
FlashReturnType flash_write ( uint32_t address, uint8_t * data_buffer, uint16_t num_bytes );

/**
 * @brief
 * This reads from a specified location in the flash memory.
 * The whole memory array may be read using a single read command.

 * @param p_flash Pointer to @c Flash structure
 * @return @c FlashStatus. Will be FLASH_BUSY if there is another operation in progress, FLASH_OK otherwise.
 * Will be FLASH_BUSY if there is another operation in progress, FLASH_OK otherwise.
 * @see https://github.com/UMSATS/Avionics-2019/
 * @note If the device is busy the function exits early and returns FLASH_BUSY.
 */
FlashReturnType flash_read ( uint32_t address, uint8_t * data_buffer, uint16_t num_bytes );

/**
 * @brief
 * This erases a specified sector(64 kb) in the flash memory. Will take up to 2 seconds.
 * The address can be any address in the desired sector.
 * @param p_flash Pointer to @c Flash structure
 * @return @c FlashStatus
 * Will be FLASH_BUSY if there is another operation in progress, FLASH_OK otherwise.
 * @see https://github.com/UMSATS/Avionics-2019/
 * @note If the device is busy the function exits early and returns FLASH_BUSY.
 */
FlashStatus flash_erase_64kb_sector ( uint32_t address );

/**
 * @brief
 * This erases a specified parameter sector(4 kb) in the flash memory. Theses are located at the start(0x00000000) of the address space.
 * Will take up to 2 seconds.
 * The address can be any address in the desired sector.
 * @param p_flash Pointer to @c Flash structure
 * @return @c FlashStatus
 * Will be FLASH_BUSY if there is another operation in progress, FLASH_OK otherwise.
 * @see https://github.com/UMSATS/Avionics-2019/
 * @note If the device is busy the function exits early and returns FLASH_BUSY.
 */
FlashStatus flash_erase_4Kb_subsector ( uint32_t address );

FlashStatus flash_write_range ( uint32_t begin_address, uint8_t * data, uint32_t size );

/**
 * @brief
 * This erases the whole flash memory. Will take up to 128 seconds.
 * @param p_flash Pointer to @c Flash structure
 * @return @c FlashStatus
 * Will be FLASH_BUSY if there is another operation in progress, FLASH_OK otherwise.
 * @see https://github.com/UMSATS/Avionics-2019/
 * @note If the device is busy the function exits early and returns FLASH_BUSY.
 */
FlashStatus flash_erase_device ( );

/**
 * @brief
 * This returns the address of the first empty page in memory.
 * Assumes continuous block of memory used.
 * @param p_flash Pointer to @c Flash structure
 * @return @c uint32_t
 * The address value (32 bits)
 * @see https://github.com/UMSATS/Avionics-2019/
 */
size_t flash_scan ( );

#endif // FLASH_H
