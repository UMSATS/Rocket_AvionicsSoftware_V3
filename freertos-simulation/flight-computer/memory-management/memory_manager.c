#include "memory_manager.h"

#include <stdio.h>
#include <memory.h>
#include <stdbool.h>
#include <string.h>

#include <assert.h>

#include <math.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "protocols/UART.h"
#include "utilities/common.h"
#include "board/components/flash.h"


/* ------------------- This memory manager ic designed for Flash Memory Cypress S25FL064P0XMFA000 ------------------- */
/* The layout of the memory is designed to support recording the real-time IMU (Bosch Sensortec BMI088) and Pressure
 * (Bosch Sensortec BMP388) sensor measurements as well as the flags such as continuity circuit and flight events (e.g.
 * launchpad, pre-apogee, apogee, post-apogee, etc.)
 *
 * The following NOR flash memory chip consists of 128 uniform 64kB
 * sectors with the two (Top or Bottom) 64 kB sectors further split up into thirty-two 4 kB sub sectors, that is in total
 * 8,388,608b --- 8,192Kb -- 8Mb of memory.
 *
 * In order to fit all the data into the memory the memory manager splits the entire memory into chunks called data
 * sectors that store:
 * 1. Global configurations (system-wise and memory-manager-wise),
 * 2. Measurements of the two sensors: IMU and Pressure
 * 3. Two system state flags: continuity circuit state and flight event state.
 *
 * NOTE:
 * One limitation of flash memory is that, although it can be read or programmed a byte or a word at a time in a random
 * access fashion, it can be erased only a block at a time. This generally sets all bits in the block to 1. Starting with
 * freshly erased block, any location within that block can be programmed. However, once a bit has been set to 0, only by
 * erasing the entire block can it be changed back to 1.
 *
 * [1. Global configurations data sector]
 * Because we can only erase in blocks we should avoid erasing for as long as possible. Therefore, since the first data
 * sector stores global configurations that is meant to be mutable (where a user can change the configuration values
 * dynamically in run-time through a command line interface), it was decided to allocate as much as 8,192b -- 8Kb of
 * memory for this section. Given that 8Kb contains 32 of 256-byte pages and configuration update takes about 70 bytes,
 * this then should fit roughly 95 configuration updates that are sequentially placed in memory.
 * For example:
 * [ [original_config] [config_update1] [config_update_2] [config_update3] [config_update4] ... [config_update225] ]
 *
 * Only the last update is to be used in the system while all the previous updates become unusable, but cannot be deleted
 * until we manually erase the entire 8Kb block. The system must not reach the end of the sector, otherwise undefined
 * behaviour is expected.
 *
 * [2, 3. Measurements of the two sensors (IMU and Pressure), Two system state flags (continuity circuit and flight event)]
 * The following four data sectors each takes 16 uniform 64 Kb sectors, that is 1,048,576b -- 1,024Kb -- 1Mb of memory.
 * These are the primary data sectors and they are meant to store the most of the information generated during the flight
 *     ____________________________________________________________________________________________________________
 *     |             |           |           |           | approximate size  | est. updates |       estimated     |
 *     | Data Sector | size (b)  |  offset   |  # pages  | of an update (b)  |   per page   |  updates per sector |
 *     |_____________|___________|___________|___________|___________________|______________|_____________________|
 *     |     IMU     | 1,048,576 |    4096   |    4096   |        28         |      9       |       36,864        |
 *     |_____________|___________|___________|___________|___________________|______________|_____________________|
 *     | Pressure    | 1,048,576 |    4096   |    4096   |        12         |      21      |       86,016        |
 *     |_____________|___________|___________|___________|___________________|______________|_____________________|
 *     | Continuity  | 1,048,576 |    4096   |    4096   |         5         |      51      |       204,800       |
 *     |_____________|___________|___________|___________|___________________|______________|_____________________|
 *     | FlightEvent | 1,048,576 |    4096   |    4096   |         5         |      51      |       204,800       |
 *     |_____________|___________|___________|___________|___________________|______________|_____________________|
 *
 * The first data sector is IMU:
 * */


// define the basic information about the metadata sector
#define RESERVED_SECTORS_BASE_ADDRESS                   0
#define RESERVED_SECTORS_COUNT                          2
#define RESERVED_SECTOR_SUB_SIZE                        FLASH_4KB_SUBSECTOR_SIZE // or 16 pages

// define the basic information about the global configuration sector
#define GLOBAL_CONFIGURATION_SECTOR_BASE                RESERVED_SECTORS_BASE_ADDRESS
#define GLOBAL_CONFIGURATION_SECTOR_SUB_COUNT           1 // 4KB

#define GLOBAL_CONFIGURATION_SECTOR_SIZE                RESERVED_SECTORS_BASE_ADDRESS + RESERVED_SECTOR_SUB_SIZE * GLOBAL_CONFIGURATION_SECTOR_SUB_COUNT // 4KB
#define GLOBAL_CONFIGURATION_SECTOR_OFFSET              RESERVED_SECTORS_BASE_ADDRESS + GLOBAL_CONFIGURATION_SECTOR_SIZE

#define MEMORY_METADATA_SECTOR_BASE                     GLOBAL_CONFIGURATION_SECTOR_OFFSET
#define MEMORY_METADATA_SECTOR_SUB_COUNT                512 // 2 MB

#define MEMORY_METADATA_SECTOR_SIZE                     RESERVED_SECTORS_BASE_ADDRESS + RESERVED_SECTOR_SUB_SIZE * MEMORY_METADATA_SECTOR_SUB_COUNT // 2 MB
#define MEMORY_METADATA_SECTOR_OFFSET                   GLOBAL_CONFIGURATION_SECTOR_OFFSET + MEMORY_METADATA_SECTOR_SIZE

// define the basic information about the data sectors
#define DATA_SECTORS_BASE                               MEMORY_METADATA_SECTOR_OFFSET
#define PAGE_SIZE                                       FLASH_PAGE_SIZE

#define IMU_ENTRIES_PER_PAGE                            ( ( int ) ( PAGE_SIZE / sizeof ( IMUDataU  ) ) )
#define PRESSURE_ENTRIES_PER_PAGE                       ( ( int ) ( PAGE_SIZE / sizeof ( PressureDataU ) ) )

#define CONTINUITY_ENTRIES_PER_PAGE                     ( ( int ) ( PAGE_SIZE / sizeof ( ContinuityU ) ) )
#define FLIGHT_EVENT_ENTRIES_PER_PAGE                   ( ( int ) ( PAGE_SIZE / sizeof ( FlightEventU ) ) )

#define GLOBAL_CONFIGURATION_ENTRIES_PER_PAGE           ( ( int ) ( PAGE_SIZE / sizeof ( GlobalConfigurationU ) ) )
#define MEMORY_METADATA_ENTRIES_PER_PAGE                ( ( int ) ( PAGE_SIZE / sizeof ( MemoryLayoutMetaDataU ) ) )

#define toUserDataSector( memory_sector ) ( UserDataSector ) memory_sector - 2
#define   toSystemSector( memory_sector ) ( SystemSector   ) memory_sector
#define   toMemorySector( user_sector )   ( MemorySector   ) user_sector + 2

// signature sequence used as an identification of the flash memory data validity
// the sequence is written at the beginning of the first or the second 4KB subsector of the flash memory
const char * MEMORY_MANAGER_DATA_INTEGRITY_SIGNATURE = "6e2201ac6e0d";
#define MEMORY_MANAGER_DATA_INTEGRITY_SIGNATURE_BUFFER_LENGTH                           12

// Variable used to control the initialization process of the memory manager to prevent any actions if this flag is not set
static bool prvIsInitialized = { 0 };

// a circular buffer queue is a processing queue that is used to temporarily store the pages of information to be fetched by a flash write monitor
//static buffer_queue prvPageBuffer = { 0 };

QueueHandle_t xPageQueue;


#define METADATA_AUTOSAVE_DATA_BASED_INTERVAL                                           200
#define METADATA_AUTOSAVE_TIME_BASED_INTERVAL                                           pdMS_TO_TICKS(250) // milliseconds to ticks

// used as an up-to-date representation of the meta data sector to be written to flash
// the state of this structure is written to flash for every X any sensor (IMU, Pressure) data updates or every N time
// where X equals to whatever number macro METADATA_AUTOSAVE_DATA_BASED_INTERVAL is.
// where N equals to whatever number macro METADATA_AUTOSAVE_TIME_BASED_INTERVAL is.
// The check is performed in prvMemoryAddNewUserDataSectorEntryToRAMBuffer()
static MemoryLayoutMetaDataU prvMemoryMetaDataFlashSnapshot = { 0 };

// used as a counter that once it reaches CONFIGURATION_AUTOSAVE_INTERVAL, prvGlobalConfigurationDiskSnapshot is then sent to the
// flash write monitor processing queue (prvPageBuffer)
static size_t prvMetadataAutosaveDataBasedCounter = { 0 };
static size_t prvMetadataAutosaveTimeBasedCounter = { 0 };

static MetaDataUpdateFrequencyMode prvMetaDataUpdateMode = MetaDataUpdateTimeBasedFrequencyMode;

// used as an up-to-date representation of the global configuration data sector to be written to flash
// the state of this structure must be written to flash before the flight ONLY
static GlobalConfigurationU prvGlobalConfigurationDiskSnapshot = { 0 };


// used to hold the writing position addresses updated after each write to flash operation
// so that with each subsequent call of prvMemoryAddNewUserDataSectorEntryToRAMBuffer() the memory manager logic could place the data into the right
// position in the temporary buffer, and thus, into the correct position on flash memory later
static MemorySectorBuffer prvCurrentMemoryUserDataSectorRAMBuffers [UserDataSectorCount]   = { 0 };

static uint8_t     is_queue_monitor_running  = { 0 };
static xTaskHandle prvQueueMonitorTaskHandle = { 0 };

typedef struct
{
    int8_t  type;
    uint8_t data[256];
} page_buffer_item;

static int prvLastPageSearchResults [ MemorySectorCount ] = { 0 };


static const MemoryManagerConfiguration prvDefaultMemoryManagerConfiguration = {
        // TODO: to be edited from GUI
        .write_pre_launch_multiplier         = 0,
        .write_pre_apogee_multiplier         = 0,
        .write_post_apogee_multiplier        = 0,
        .write_ground_multiplier             = 0,
        .write_interval_accelerometer_ms     = 0,
        .write_interval_gyroscope_ms         = 0,
        .write_interval_magnetometer_ms      = 0,
        .write_interval_pressure_ms          = 0,
        .write_interval_altitude_ms          = 0,
        .write_interval_temperature_ms       = 0,
        .write_interval_flight_state_ms      = 0,
        .write_drogue_continuity_ms          = 0,
        .write_main_continuity_ms            = 0,

        .user_data_sector_sizes              = {
                0xE00000  /* Gyro  - 14MB  */,
                0xE00000  /* Accel - 14MB  */,
                0x04EC00  /* Mag   - 315KB */,
                0x07E400  /* Press - 505KB */,
                0x07E400  /* Temp  - 505KB */,
                0x028000  /* Cont  - 160KB */,
                0x028000  /* Temp  - 160KB */
        }

};

MemoryManagerConfiguration memory_manager_get_default_memory_configurations ( )
{
    return prvDefaultMemoryManagerConfiguration;
}

void memory_manager_set_metadata_update_mode ( MetaDataUpdateFrequencyMode mode )
{
    prvMetaDataUpdateMode = mode;
}

static void prvQueueMonitorTask ( void * arg );

static uint32_t prvMemorySectorGetSize ( MemorySector sector );
static uint32_t prvMemorySectorGetDataEntriesPerPage ( MemorySector sector );
static uint32_t prvMemorySectorGetDataStructSize ( MemorySector sector );
static uint32_t prvMemorySectorGetAlignedDataStructSize ( MemorySector sector );
static MemoryManagerStatus prvMemorySectorLinearSearchForLastWrittenPageIndex ( MemorySector sector, MemorySectorInfo info, uint32_t * result );
static MemoryManagerStatus prvMemorySectorBinarySearchForLastWrittenPageIndex ( MemorySector sector, MemorySectorInfo info, uint32_t * result );
static MemoryManagerStatus prvMemoryAddNewUserDataSectorEntryToRAMBuffer ( UserDataSector sector, uint8_t * buffer );
static MemoryManagerStatus prvMemoryAsyncWriteAnyMemorySectorIfAvailable ( );
static MemoryManagerStatus prvMemoryWriteAsyncMetaDataSector ( );
static MemoryManagerStatus prvMemoryWriteAsyncGlobalConfigurationSector ( );
static MemoryManagerStatus prvVerifySystemSectorIntegrity ( SystemSector sector, uint8_t * data, bool * status );
static MemoryManagerStatus prvMemoryAccessPage ( MemorySector sector, MemorySectorInfo info, int64_t pageIndex, uint8_t * dest );
static MemoryManagerStatus prvMemorySystemSectorWritePageNow ( SystemSector sector, uint8_t * data );
static MemoryManagerStatus prvMemoryWritePageNow ( MemorySectorInfo info, uint8_t * data );
static MemoryManagerStatus prvMemoryAccessSectorSingleDataEntry ( MemorySector sector, MemorySectorInfo info, uint32_t index, void * dst );
static MemoryManagerStatus prvMemoryAccessLastDataEntry ( MemorySector sector, MemorySectorInfo info, void * dst );
static MemoryManagerStatus prvGetMemorySectorInfo ( MemorySector sector, MemorySectorInfo * info );

MemoryManagerStatus memory_manager_init ( ) /* noexcept */
{

    // can be initialized only once
    if ( prvIsInitialized == true )
    {
        return MEM_ERR;
    }

    // Initialize the memory sector *read and *write pointers for their safe & correct functioning
    for ( UserDataSector sector = UserDataSectorGyro; sector < UserDataSectorCount; sector++ )
    {
        if ( prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].write == NULL )
        {
            prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].write = &prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].buffers[ 0 ];
        }

        if ( prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].read == NULL )
        {
            prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].read = &prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].buffers[ 1 ];
        }
    }

    // important part of initialization is the buffer queue that will hold a page of information to be written to the flash memory
//    buffer_queue_init ( &prvPageBuffer );

    bool isIntegrityOK = false;

    // then find out whether the fetched meta configuration is a valid meta configuration subsector
    if ( ! prvVerifySystemSectorIntegrity ( SystemSectorGlobalConfigurationData, prvGlobalConfigurationDiskSnapshot.bytes, &isIntegrityOK ) )
    {
        return MEM_ERR;
    }

    if ( isIntegrityOK == true )
    {
        // then prvGlobalConfigurationDiskSnapshot already holds correct data and we must not modify it at this point
    }
    else
    {
        // in case if memory was erased, flash memory sets all bits high, and we cannot use such data with ASCII table.
        // Thus, we need to set everything we read to zeros
        memset ( &prvGlobalConfigurationDiskSnapshot.bytes, 0, sizeof ( GlobalConfigurationU ) );

        // meta configuration sector is invalid and thus either the data was invalidated or the memory was erased
        // ---- initiate a fresh start ------
        prvGlobalConfigurationDiskSnapshot.values.memory = prvDefaultMemoryManagerConfiguration;
        prvGlobalConfigurationDiskSnapshot.values.system = get_default_system_configuration ( );
        memcpy ( prvGlobalConfigurationDiskSnapshot.values.signature, MEMORY_MANAGER_DATA_INTEGRITY_SIGNATURE, MEMORY_MANAGER_DATA_INTEGRITY_SIGNATURE_BUFFER_LENGTH );
    }

    // then find out whether the fetched meta configuration is a valid meta configuration subsector
    if ( ! prvVerifySystemSectorIntegrity ( SystemSectorUserDataSectorMetaData, prvMemoryMetaDataFlashSnapshot.bytes, &isIntegrityOK ) )
    {
        return MEM_ERR;
    }

    if ( isIntegrityOK == true )
    {
        // then prvMemoryMetaDataFlashSnapshot already holds correct data and we must not modify it at this point
    }
    else
    {
        // meta configuration sector is invalid and thus either the data was invalidated or the memory was erased
        // ---- initiate a fresh start ------

        // in case if memory was erased, flash memory sets all bits high, and we cannot use such data with ASCII table.
        // Thus, we need to set everything we read to zeros
        memset ( &prvMemoryMetaDataFlashSnapshot.bytes, 0, sizeof ( MemoryLayoutMetaDataU ) );

        // for user data sectors
        int32_t globalOffset = DATA_SECTORS_BASE;
        for ( UserDataSector sectorIndex  = UserDataSectorGyro; sectorIndex < UserDataSectorCount; sectorIndex++ )
        {
            MemorySectorInfo * sectorInfo = &prvMemoryMetaDataFlashSnapshot.values.user_sectors[ sectorIndex ];
            sectorInfo->startAddress = globalOffset;
            sectorInfo->size         = prvDefaultMemoryManagerConfiguration.user_data_sector_sizes[ sectorIndex ];

            globalOffset += sectorInfo->size;

            // it is a data sector, so we initialize it properly using the right offsets and sizes
            sectorInfo->endAddress   = sectorInfo->startAddress + sectorInfo->size;
            sectorInfo->bytesWritten = 0; /* nothing has been written to this memory sector, hence — 0 */
        }

        // storing the signature number
        memcpy ( prvMemoryMetaDataFlashSnapshot.values.signature, MEMORY_MANAGER_DATA_INTEGRITY_SIGNATURE, MEMORY_MANAGER_DATA_INTEGRITY_SIGNATURE_BUFFER_LENGTH );
    }

    xPageQueue = xQueueCreate ( 10, sizeof ( page_buffer_item ) );

    // initialization flag
    prvIsInitialized = true;

    // returns OK status
    return MEM_OK;
}

MemoryManagerStatus memory_manager_start ( )
{
    if ( ! prvIsInitialized )
    {
        return MEM_ERR;
    }

    if ( pdFALSE == xTaskCreate ( prvQueueMonitorTask, "mem-manager", configMINIMAL_STACK_SIZE, NULL, 5, &prvQueueMonitorTaskHandle ) )
    {
        return MEM_ERR;
    }

    return MEM_OK;
}

MemoryManagerStatus memory_manager_stop ( )
{
    is_queue_monitor_running = 0;
    return MEM_OK;
}

MemoryManagerStatus memory_manager_user_data_update ( DataContainer * _container )
{
    if ( prvIsInitialized == false )
    {
        return MEM_ERR;
    }

    if ( _container == NULL )
    {
        return MEM_ERR;
    }

    if ( _container->gyro.updated )
    {
        prvMemoryAddNewUserDataSectorEntryToRAMBuffer ( UserDataSectorGyro, _container->gyro.data.bytes );
        _container->gyro.updated = 0;
    }

    if ( _container->acc.updated )
    {
        prvMemoryAddNewUserDataSectorEntryToRAMBuffer ( UserDataSectorAccel, _container->acc.data.bytes );
        _container->acc.updated = 0;
    }

    if ( _container->mag.updated )
    {
        prvMemoryAddNewUserDataSectorEntryToRAMBuffer ( UserDataSectorMag, _container->mag.data.bytes );
        _container->mag.updated = 0;
    }

    if ( _container->press.updated )
    {
        prvMemoryAddNewUserDataSectorEntryToRAMBuffer ( UserDataSectorPressure, _container->press.data.bytes );
        _container->press.updated = 0;
    }

    if ( _container->temp.updated )
    {
        prvMemoryAddNewUserDataSectorEntryToRAMBuffer ( UserDataSectorTemperature, _container->temp.data.bytes );
        _container->temp.updated = 0;
    }

    if ( _container->cont.updated )
    {
        prvMemoryAddNewUserDataSectorEntryToRAMBuffer ( UserDataSectorContinuity, _container->cont.data.bytes );
        _container->cont.updated = 0;
    }

    if ( _container->event.updated )
    {
        prvMemoryAddNewUserDataSectorEntryToRAMBuffer ( UserDataSectorFlightEvent, _container->event.data.bytes );
        _container->event.updated = 0;
    }

    // of course we are not forgetting to check on the metadata entry whether it is the time to flush it onto the disk
    if ( prvMetaDataUpdateMode == MetaDataUpdateDataBasedFrequencyMode )
    {
        prvMetadataAutosaveDataBasedCounter++;
        if ( prvMetadataAutosaveDataBasedCounter >= METADATA_AUTOSAVE_DATA_BASED_INTERVAL )
        {
            // it is time to update the configurations: signaling the monitor to process the metadata buffer
            prvMemoryWriteAsyncMetaDataSector ( );
            // reset the timer
            prvMetadataAutosaveDataBasedCounter = 0;
        }
    }

    if ( prvMetaDataUpdateMode == MetaDataUpdateTimeBasedFrequencyMode )
    {
        if ( ( xTaskGetTickCount ( ) - prvMetadataAutosaveTimeBasedCounter ) >= METADATA_AUTOSAVE_TIME_BASED_INTERVAL )
        {
            // it is time to update the configurations
            prvMemoryWriteAsyncMetaDataSector ( );

            // reset the timer
            prvMetadataAutosaveTimeBasedCounter = xTaskGetTickCount ( );
//            DEBUG_LINE ( "CURRENT ALTITUDE : %f", event_detector_current_altitude () );

        }
    }

    return MEM_OK;
}

MemoryManagerStatus memory_manager_get_system_configurations ( FlightSystemConfiguration * systemConfiguration )
{
    if ( systemConfiguration == NULL )
    {
        return MEM_ERR;
    }

    memcpy ( systemConfiguration, &prvGlobalConfigurationDiskSnapshot.values.system, sizeof ( FlightSystemConfiguration ) );

    return MEM_OK;
}

MemoryManagerStatus memory_manager_set_system_configurations ( FlightSystemConfiguration * systemConfiguration )
{
    if ( systemConfiguration == NULL )
    {
        return MEM_ERR;
    }

    if ( !prvIsInitialized )
    {
        return MEM_ERR;
    }

    memset ( &prvGlobalConfigurationDiskSnapshot.values.system, 0, sizeof ( FlightSystemConfiguration ) );
    prvGlobalConfigurationDiskSnapshot.values.system = *systemConfiguration;

    prvMemoryWriteAsyncGlobalConfigurationSector ( );
    return MEM_OK;
}

MemoryManagerStatus memory_manager_get_memory_configurations ( MemoryManagerConfiguration * memoryConfiguration )
{
    if ( memoryConfiguration == NULL )
    {
        return MEM_ERR;
    }

    if ( !prvIsInitialized )
    {
        return MEM_ERR;
    }

    memcpy ( memoryConfiguration, &prvGlobalConfigurationDiskSnapshot.values.memory, sizeof ( MemoryManagerConfiguration ) );

    return MEM_OK;
}

MemoryManagerStatus memory_manager_set_memory_configurations ( MemoryManagerConfiguration * memoryConfiguration )
{
    if ( memoryConfiguration == NULL )
    {
        return MEM_ERR;
    }

    memset ( &prvGlobalConfigurationDiskSnapshot.values.memory, 0, sizeof ( MemoryManagerConfiguration ) );
    prvGlobalConfigurationDiskSnapshot.values.memory = *memoryConfiguration;

    prvMemoryWriteAsyncGlobalConfigurationSector ( );

    return MEM_OK;
}

MemoryManagerStatus memory_manager_erase_configuration_section ( )
{
    if ( FLASH_OK != flash_erase_4Kb_subsector ( GLOBAL_CONFIGURATION_SECTOR_BASE ) )
    {
        return MEM_ERR;
    }

    return MEM_OK;
}

MemoryManagerStatus memory_manager_erase_everything ( )
{
    if ( FLASH_OK != flash_erase_device ( ) )
    {
        return MEM_ERR;
    }

    return MEM_OK;
}


MemoryManagerStatus prvVerifySystemSectorIntegrity ( SystemSector sector, uint8_t * data, bool * status )
{
    if ( data == NULL )
    {
        return MEM_ERR;
    }

    *status = false;

    MemorySectorInfo info = { 0 };

    switch ( sector )
    {
        case SystemSectorGlobalConfigurationData:
            info.startAddress = GLOBAL_CONFIGURATION_SECTOR_BASE;
            info.endAddress   = GLOBAL_CONFIGURATION_SECTOR_OFFSET;
            info.size         = GLOBAL_CONFIGURATION_SECTOR_SIZE;
            break;
        case SystemSectorUserDataSectorMetaData:
            info.startAddress = MEMORY_METADATA_SECTOR_BASE;
            info.endAddress   = MEMORY_METADATA_SECTOR_OFFSET;
            info.size         = MEMORY_METADATA_SECTOR_SIZE;
            break;
        case SystemSectorCount:
            return MEM_ERR;
    }

    MemoryBuffer buffer = { };
    // try reading the metadata from flash before all (we first try the very first subsector)
    if ( ! prvMemoryAccessPage ( ( MemorySector ) sector, info, -1, buffer.data ) )
    {
        return MEM_ERR;
    }

    memcpy ( data, buffer.data, prvMemorySectorGetDataStructSize ( ( MemorySector ) sector ) );


    // and then check whether that subsector is valid
    if ( memcmp ( data, MEMORY_MANAGER_DATA_INTEGRITY_SIGNATURE, MEMORY_MANAGER_DATA_INTEGRITY_SIGNATURE_BUFFER_LENGTH ) == 0 ) /* if signature sequences match */
    {
        *status = true;
    }
    else
    {
        *status = false;
    }

    return MEM_OK;
}


MemoryManagerStatus prvMemoryAddNewUserDataSectorEntryToRAMBuffer ( UserDataSector sector, uint8_t * buffer )
{
    if ( prvIsInitialized == false )
    {
        return MEM_ERR;
    }

    if ( buffer == NULL )
    {
        return MEM_ERR;
    }

    uint32_t size = prvMemorySectorGetDataStructSize ( toMemorySector ( sector ) );

    int page_aligned_boundary = prvMemorySectorGetAlignedDataStructSize ( toMemorySector ( sector ) ) ;
    // check whether the number of data entries filled up the page size, such that no more entries of this data type
    // will fit into the page size without over-fitting.
    if ( prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].write->info.bytesWritten >= page_aligned_boundary )
    {
        // if there is no more room for another data entry then we signal to the monitor that it is time to take out this
        // page and flush it ont ot the memory. To do that we swap the read and write buffer, so that the read buffer now
        // contains that full page of data (write buffer), while the write buffer should be come empty and ready for new data
        MemoryBuffer * temp_read = prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].read;
        prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].read  = prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].write;
        prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].write = temp_read;

        // here's the signal to the monitor to process that page.
        prvMemoryAsyncWriteAnyMemorySectorIfAvailable ( );

        // safeguard check: if the read buffer that was supposed to be processed by the monitor has actually been already processed or not yet
        if ( temp_read->info.bytesWritten >= size )
        {
            // if not then the monitor did not manage to do it before we landed here. Maybe the producing data is faster than its consuming?
            DEBUG_LINE ( "The page was not saved. Probably the monitor has stuck, or it ia just too slow for the data producer. Overwriting the page..." );
            // resetting the current data pointer
            prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].write->info.bytesWritten = 0;
        }
    }

    // now that we emptied the write buffer and it is newly fresh and ready, we are copying the incoming data entry to it
    MemoryBuffer * currentBuffer           = prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].write;
    uint8_t      * currWriteBufferPosition = &currentBuffer->data[ currentBuffer->info.bytesWritten ];
    memcpy ( currWriteBufferPosition, buffer, size );
    // and updating its current data pointer
    currentBuffer->info.bytesWritten += size;

    return MEM_OK;
}

MemoryManagerStatus prvMemoryWriteAsyncMetaDataSector ( )
{
    if ( prvIsInitialized == false )
    {
        return MEM_ERR;
    }

    page_buffer_item item = { };
    item.type = MemorySystemSectorUserDataSectorMetaData;
    memmove ( item.data, prvMemoryMetaDataFlashSnapshot.bytes, prvMemorySectorGetDataStructSize ( MemorySystemSectorUserDataSectorMetaData ) );

    return pdPASS == xQueueSend ( xPageQueue, &item, 0 ) ? MEM_OK : MEM_ERR;
}

static MemoryManagerStatus prvMemoryWriteAsyncGlobalConfigurationSector ( )
{
    if ( prvIsInitialized == false )
    {
        return MEM_ERR;
    }

    page_buffer_item item = { };
    item.type = MemorySystemSectorGlobalConfigurationData;
    memmove ( item.data, prvGlobalConfigurationDiskSnapshot.bytes, prvMemorySectorGetDataStructSize ( MemorySystemSectorGlobalConfigurationData ) );

    return pdPASS == xQueueSend ( xPageQueue, &item, 0 ) ? MEM_OK : MEM_ERR;
}

static MemoryManagerStatus  prvMemoryAsyncWriteAnyMemorySectorIfAvailable ( )
{
    // free the read page
    for ( UserDataSector sector = UserDataSectorGyro; sector < UserDataSectorCount; sector++ )
    {
        int page_aligned_boundary = prvMemorySectorGetAlignedDataStructSize ( toMemorySector( sector ) );
        if ( prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].read->info.bytesWritten >= page_aligned_boundary )
        {
            page_buffer_item item = { } ;
            item.type = toMemorySector ( sector ) ;
            memcpy ( item.data, prvCurrentMemoryUserDataSectorRAMBuffers[ sector ].read->data, PAGE_SIZE ) ;

            if ( pdPASS != xQueueSend ( xPageQueue, &item, 0 ) )
            {
                return MEM_ERR;
            }
        }
    }

    return MEM_OK;
}

MemoryManagerStatus prvMemoryWritePageNow ( MemorySectorInfo info, uint8_t * data )
{
    if (prvIsInitialized == false)
    {
        return MEM_ERR;
    }

    if (data == NULL)
    {
        return MEM_ERR;
    }

    const uint32_t offset = info.startAddress + info.bytesWritten;

    return flash_write ( offset, data, PAGE_SIZE ) == FLASH_OK;
}


MemoryManagerStatus prvMemorySystemSectorWritePageNow ( SystemSector sector, uint8_t * data )
{
    if ( data == NULL)
    {
        return MEM_ERR;
    }

    int offset = 0;

    if ( sector == SystemSectorGlobalConfigurationData )
    {

#if ( userconf_FREE_RTOS_SIMULATOR_MODE_ON == 0 )
        memory_manager_erase_configuration_section ( );
#else
        offset = GLOBAL_CONFIGURATION_SECTOR_BASE;
#endif

    }

    else if ( sector == SystemSectorUserDataSectorMetaData )
    {
        offset = MEMORY_METADATA_SECTOR_BASE + prvLastPageSearchResults [ MemorySystemSectorUserDataSectorMetaData ] * PAGE_SIZE;
    }
    else
    {
        // we should not land here!
        return MEM_ERR;
    }

    return flash_write ( offset, data, PAGE_SIZE ) == FLASH_OK;
}

/**
 * @param sector: desired memory sector
 * @param pageIndex: -1 for the last available page in the given sector, 0-N to indicate the page index to be retrieved
 * @param dest: destination buffer (Note: MUST be at least 255 bytes of size)
 * @return MemoryManagerStatus: OK or ERR
 */
static MemoryManagerStatus prvMemoryAccessPage ( MemorySector sector, MemorySectorInfo info, int64_t pageIndex, uint8_t * dest )
{
    uint32_t PAGE_INDEX = 0;

    if ( pageIndex == -1 )
    {
        if ( ! prvMemorySectorBinarySearchForLastWrittenPageIndex ( sector, info, &PAGE_INDEX ) )
        {
            return MEM_ERR;
        }
    }
    else
    {
        PAGE_INDEX = pageIndex;
    }

    uint32_t address = info.startAddress + ( PAGE_INDEX * PAGE_SIZE );

    if ( FLASH_OK != flash_read ( address, dest, PAGE_SIZE ) )
    {
        return MEM_ERR;
    }

    return MEM_OK;
}


MemoryManagerStatus prvMemoryAccessSectorSingleDataEntry ( MemorySector sector, MemorySectorInfo info, uint32_t index, void * dst )
{
    uint8_t entries_per_page = prvMemorySectorGetDataEntriesPerPage ( sector );
    uint8_t struct_size      = prvMemorySectorGetDataStructSize ( sector );

    if ( PAGE_SIZE * trunc ( ( double ) index / prvMemorySectorGetDataEntriesPerPage ( sector ) ) > info.bytesWritten )
    {
        return MEM_ERR;
    }

    if ( dst == NULL )
    {
        return MEM_ERR;
    }

    MemoryBuffer buffer = { };

    uint32_t pageIndex = ceil ( ( double ) index / entries_per_page );

    if ( ! prvMemoryAccessPage ( sector, info, pageIndex, buffer.data ) )
    {
        return MEM_ERR;
    }

    index = ( pageIndex * entries_per_page ) - index;

    memcpy ( dst, &buffer.data [ index ], struct_size );

    return MEM_OK;
}

static MemoryManagerStatus prvMemoryAccessLastDataEntry ( MemorySector sector, MemorySectorInfo info, void * dst )
{
    if ( dst == NULL )
    {
        return MEM_ERR;
    }

    if ( toSystemSector ( sector ) < SystemSectorCount )
    {
        uint32_t lastPageIndex = 0;
        if ( ! prvMemorySectorBinarySearchForLastWrittenPageIndex ( sector, info, &lastPageIndex ) )
        {
            return MEM_ERR;
        }

        if ( MEM_OK != prvMemoryAccessPage ( sector, info, lastPageIndex, dst ) )
        {
            return MEM_ERR;
        }
    }

    else
    {
        uint32_t lastAddress = info.startAddress + info.bytesWritten - PAGE_SIZE +
                               ( ( prvMemorySectorGetDataEntriesPerPage ( sector ) - 1 ) * prvMemorySectorGetDataStructSize ( sector ) );

        if ( FLASH_OK != flash_read ( lastAddress, dst, prvMemorySectorGetDataStructSize ( sector ) ) )
        {
            return MEM_ERR;
        }
    }

    return MEM_OK;
}

void prvQueueMonitorTask ( void * arg )
{
    ( void ) arg;

    is_queue_monitor_running = 1;
    page_buffer_item item = { };
    MemoryManagerStatus status;

    while ( is_queue_monitor_running )
    {
        while ( pdPASS == xQueueReceive ( xPageQueue, &item,  portMAX_DELAY ) )
        {
            if ( item.type < SystemSectorCount )
            {
                // it is a system sector

//                DEBUG_LINE( "SystemSector: %i was flushed!",  toSystemSector ( item.type ) );
                status = prvMemorySystemSectorWritePageNow ( item.type, item.data );
            }
            else
            {
                // switch ( )
                // it is a user data sector
//                DEBUG_LINE( "UserSector: %i was flushed!", toUserDataSector ( item.type ) );
                status = prvMemoryWritePageNow ( prvMemoryMetaDataFlashSnapshot.values.user_sectors [ toUserDataSector ( item.type ) ], item.data );
            }

            if ( status )
            {
                if ( item.type < SystemSectorCount )
                {
                    // it is a system sector

                    if ( item.type == MemorySystemSectorUserDataSectorMetaData )
                    {
                        // update the page counter, so that the next time the data will not go to overwrite the existing page, instead it will go right after it
                        prvLastPageSearchResults [ toSystemSector ( item.type ) ]++;
                    }
                }
                else
                {
                    // it is a user data sector

                    // now start address should point to a page size away from the previous one
                    prvMemoryMetaDataFlashSnapshot.values.user_sectors [ toUserDataSector ( item.type ) ].bytesWritten += PAGE_SIZE;

                    // reset the read RAM buffer cursor
                    // flagging to the other components that this piece of data has been already processed and emptied
                    memset ( prvCurrentMemoryUserDataSectorRAMBuffers [ toUserDataSector ( item.type ) ].read, 0, PAGE_SIZE );
                }
            }
        }
    }

    DISPLAY_LINE( "monitor EXITED!" );
}


MemoryManagerStatus prvGetMemorySectorInfo ( MemorySector sector, MemorySectorInfo * info)
{
    if ( info == NULL )
    {
        return MEM_ERR;
    }

    switch ( sector )
    {

        case MemorySystemSectorGlobalConfigurationData:

            info->startAddress = GLOBAL_CONFIGURATION_SECTOR_BASE;
            info->endAddress   = GLOBAL_CONFIGURATION_SECTOR_OFFSET;
            info->size         = GLOBAL_CONFIGURATION_SECTOR_SIZE;

            break;

        case MemorySystemSectorUserDataSectorMetaData:

            info->startAddress = MEMORY_METADATA_SECTOR_BASE;
            info->endAddress   = MEMORY_METADATA_SECTOR_OFFSET;
            info->size         = MEMORY_METADATA_SECTOR_SIZE;

            break;

        case MemoryUserDataSectorGyro:
        case MemoryUserDataSectorAccel:
        case MemoryUserDataSectorMag:
        case MemoryUserDataSectorPressure:
        case MemoryUserDataSectorTemperature:
        case MemoryUserDataSectorContinuity:
        case MemoryUserDataSectorFlightEvent:

            if ( ! prvIsInitialized )
            {
                return MEM_ERR;
            }

            *info = prvMemoryMetaDataFlashSnapshot.values.user_sectors [ toUserDataSector ( sector ) ];

            break;
        case MemorySectorCount:
        default:
            break;
    }

    return MEM_OK;
}

static uint32_t prvMemorySectorGetDataEntriesPerPage ( MemorySector sector )
{
    switch ( sector )
    {
        case MemorySystemSectorGlobalConfigurationData:
            return GLOBAL_CONFIGURATION_ENTRIES_PER_PAGE;
        case MemorySystemSectorUserDataSectorMetaData:
            return MEMORY_METADATA_ENTRIES_PER_PAGE;
        case MemoryUserDataSectorGyro:
        case MemoryUserDataSectorAccel:
        case MemoryUserDataSectorMag:
            return IMU_ENTRIES_PER_PAGE;
        case MemoryUserDataSectorPressure:
        case MemoryUserDataSectorTemperature:
            return PRESSURE_ENTRIES_PER_PAGE;
        case MemoryUserDataSectorContinuity:
            return CONTINUITY_ENTRIES_PER_PAGE;
        case MemoryUserDataSectorFlightEvent:
            return FLIGHT_EVENT_ENTRIES_PER_PAGE;
        case MemorySectorCount:
        default:
            return 0;
    }
}

static uint32_t prvMemorySectorGetDataStructSize ( MemorySector sector )
{
    switch ( sector )
    {
        case MemorySystemSectorGlobalConfigurationData:
            return sizeof ( GlobalConfigurationU );
        case MemorySystemSectorUserDataSectorMetaData:
            return sizeof ( MemoryLayoutMetaDataU );
        case MemoryUserDataSectorGyro:
        case MemoryUserDataSectorAccel:
        case MemoryUserDataSectorMag:
            return sizeof ( IMUDataU );
        case MemoryUserDataSectorPressure:
        case MemoryUserDataSectorTemperature:
            return sizeof ( PressureDataU );
        case MemoryUserDataSectorContinuity:
            return sizeof ( ContinuityU );
        case MemoryUserDataSectorFlightEvent:
            return sizeof ( FlightEventU );
        case MemorySectorCount:
        default:
            return 0;
    }
}

static uint32_t prvMemorySectorGetAlignedDataStructSize ( MemorySector sector )
{
    switch ( sector )
    {
        case MemorySystemSectorGlobalConfigurationData:
            return trunc ( ( PAGE_SIZE / sizeof ( GlobalConfigurationU ) ) ) * sizeof ( GlobalConfigurationU );
        case MemorySystemSectorUserDataSectorMetaData:
            return trunc ( ( PAGE_SIZE / sizeof ( MemoryLayoutMetaDataU ) ) ) * sizeof ( MemoryLayoutMetaDataU );
        case MemoryUserDataSectorGyro:
        case MemoryUserDataSectorAccel:
        case MemoryUserDataSectorMag:
            return trunc ( ( PAGE_SIZE / sizeof ( IMUDataU ) ) ) * sizeof ( IMUDataU );
        case MemoryUserDataSectorPressure:
        case MemoryUserDataSectorTemperature:
            return trunc ( ( PAGE_SIZE / sizeof ( PressureDataU ) ) ) * sizeof ( PressureDataU );
        case MemoryUserDataSectorContinuity:
            return trunc ( ( PAGE_SIZE / sizeof ( ContinuityU ) ) ) * sizeof ( ContinuityU );
        case MemoryUserDataSectorFlightEvent:
            return trunc ( ( PAGE_SIZE / sizeof ( FlightEventU ) ) ) * sizeof ( FlightEventU );
        case MemorySectorCount:
        default:
            return 0;
    }
}

int prvMemorySectorGetPageCount ( MemorySector sector )
{
    if ( toSystemSector ( sector ) < SystemSectorCount )
    {
        // we are accessing the system sector where their sizes are predetermined
        switch ( sector )
        {
            case SystemSectorGlobalConfigurationData:
                return ( GLOBAL_CONFIGURATION_SECTOR_SIZE / PAGE_SIZE );
            case SystemSectorUserDataSectorMetaData:
                return ( MEMORY_METADATA_SECTOR_SIZE / PAGE_SIZE );
            case SystemSectorCount:
            default:
                return 0;
        }
    }
    else
    {
        // we are accessing the user data sector where the sizes are changed by user and not known before the configuration sector is read
        // thus this function will not work for user data sections without prior call to memory_manager_configure () that was successful
        // the indication of that can be checked with prvIsConfigured variable (true if successful, false, if not successful, or has not been called)
        if ( prvIsInitialized == false )
        {
            // returning 0 makes sense because if everything goes right, there is no way that sector would have 0 pages
            return 0;
        }

        // if prvIsConfigured == true, then we can freely use prvGlobalConfigurationDiskSnapshot variable and can safely use user_data_sector_sizes to
        // find out the user sector sizes.
        return ( prvGlobalConfigurationDiskSnapshot.values.memory.user_data_sector_sizes [ toUserDataSector ( sector ) ] / PAGE_SIZE );
    }
}

static uint32_t prvMemorySectorGetSize ( MemorySector sector )
{
    if ( toSystemSector ( sector ) < SystemSectorCount )
    {
        // we are accessing the system sector where their sizes are predetermined
        switch ( sector )
        {
            case SystemSectorGlobalConfigurationData:
                return ( GLOBAL_CONFIGURATION_SECTOR_SIZE );
            case SystemSectorUserDataSectorMetaData:
                return ( MEMORY_METADATA_SECTOR_SIZE );
            case SystemSectorCount:
            default:
                return 0;
        }
    }
    else
    {
        // we are accessing the user data sector where the sizes are changed by user and not known before the configuration sector is read
        // thus this function will not work for user data sections without prior call to memory_manager_configure () that was successful
        // the indication of that can be checked with prvIsConfigured variable (true if successful, false, if not successful, or has not been called)
        if ( prvIsInitialized == false )
        {
            // returning 0 makes sense because if everything goes right, there is no way that sector would have 0 pages
            return 0;
        }

        // if prvIsConfigured == true, then we can freely use prvGlobalConfigurationDiskSnapshot variable and can safely use user_data_sector_sizes to
        // find out the user sector sizes.
        return prvGlobalConfigurationDiskSnapshot.values.memory.user_data_sector_sizes[ toUserDataSector ( sector ) ];
    }
}

static MemoryManagerStatus prvMemorySectorBinarySearchForLastWrittenPageIndex ( MemorySector sector, MemorySectorInfo info, uint32_t * result )
{
    if ( result == NULL )
    {
        return MEM_ERR;
    }

    int sectorPageCount = prvMemorySectorGetPageCount ( sector );
    // need to search the sector and find the latest record

    MemoryBuffer data = { 0 };

    // set the middle
    int firstPage = 0 ;
    int lastPage  = sectorPageCount - 1;
    int middlePage = ( firstPage + lastPage ) / 2;

    while ( firstPage <= lastPage )
    {
        memset ( data.data, 0, PAGE_SIZE );

        if ( prvMemoryAccessPage ( sector, info, middlePage, data.data ) == MEM_ERR )
        {
            return MEM_ERR;
        }

        if ( common_is_mem_empty ( data.data, PAGE_SIZE ) )
        {
            lastPage = middlePage - 1;
        }
        else
        {
            firstPage = middlePage + 1;
        }

        middlePage = ( firstPage + lastPage ) / 2;
    }

    ( *result ) = middlePage ;

    prvLastPageSearchResults [ sector ] = middlePage;

    return MEM_OK;
}

static MemoryManagerStatus prvMemorySectorLinearSearchForLastWrittenPageIndex ( MemorySector sector, MemorySectorInfo info, uint32_t * result )
{
    if ( result == NULL )
    {
        return MEM_ERR;
    }

    int pageIndex = 0;

    int sectorPageCount = prvMemorySectorGetPageCount ( sector );
    // need to search the sector and find the latest record

    MemoryBuffer data = { 0 };

    while ( pageIndex < sectorPageCount )
    {
        memset ( data.data, 0, PAGE_SIZE );

        if ( prvMemoryAccessPage ( sector, info, pageIndex, data.data ) == MEM_ERR )
        {
            return MEM_ERR;
        }

        if ( common_is_mem_empty ( data.data, PAGE_SIZE ) )
        {
            if ( pageIndex != 0 )
            {
                pageIndex--;
            }

            break;
        }

        pageIndex++;
    }

    ( *result ) = pageIndex;

    prvLastPageSearchResults[ sector ] = pageIndex;

    return MEM_OK;
}

MemoryManagerStatus memory_manager_get_single_data_entry ( MemorySector sector, void * dst, uint32_t entry_index )
{
    if ( dst == NULL )
    {
        return MEM_ERR;
    }

    MemorySectorInfo info = { 0 };

    if ( !prvGetMemorySectorInfo ( sector, &info ) )
    {
        return MEM_ERR;
    }

    return prvMemoryAccessSectorSingleDataEntry ( MemoryUserDataSectorFlightEvent, info, entry_index, dst );
}


MemoryManagerStatus memory_manager_get_last_data_entry ( MemorySector sector, void * dst )
{
    if ( dst == NULL )
    {
        return MEM_ERR;
    }

    MemorySectorInfo info = { 0 };

    if ( !prvGetMemorySectorInfo ( sector, &info ) )
    {
        return MEM_ERR;
    }

    return prvMemoryAccessLastDataEntry ( MemoryUserDataSectorFlightEvent, info, dst );
}


MemoryManagerStatus memory_manager_get_stats ( char * buffer, size_t xBufferLen )
{
    int length = 0;
    length += snprintf ( buffer + length, xBufferLen, "\n----- Memory Statistics -----\r\n" );
    length += snprintf ( buffer + length, xBufferLen, "signature: %s\r\n", prvGlobalConfigurationDiskSnapshot.values.signature );
    length += snprintf ( buffer + length, xBufferLen, "Data Sectors: " );

    for ( UserDataSector sector = UserDataSectorGyro; sector < UserDataSectorCount; sector++ )
    {
        length += snprintf ( buffer + length, xBufferLen, "%i,", sector );
    }

    length += snprintf ( buffer + length, xBufferLen, "\r\n" );

    MemorySectorInfo     dataSector;
    for ( UserDataSector sector = UserDataSectorGyro; sector < UserDataSectorCount; sector++ )
    {
        dataSector = prvMemoryMetaDataFlashSnapshot.values.user_sectors[ sector ];
        length += snprintf ( buffer + length, xBufferLen, "Sector #%i:\r\n", sector );
        length += snprintf ( buffer + length, xBufferLen, "--------------------\r\n" );
        length += snprintf ( buffer + length, xBufferLen, " size:            %lu\r\n", dataSector.size );
        length += snprintf ( buffer + length, xBufferLen, " begin:           %lu\r\n", dataSector.startAddress );
        length += snprintf ( buffer + length, xBufferLen, " end:             %lu\r\n", dataSector.endAddress );
        length += snprintf ( buffer + length, xBufferLen, " size on disk:    %lu\r\n", dataSector.bytesWritten );
        length += snprintf ( buffer + length, xBufferLen, " pages on disk:   %lu\r\n", dataSector.bytesWritten / PAGE_SIZE );
        length += snprintf ( buffer + length, xBufferLen, " entries on disk: %lu\r\n", dataSector.bytesWritten / PAGE_SIZE * prvMemorySectorGetDataEntriesPerPage ( toMemorySector( sector ) ) );

//        uint8_t dst [prvMemorySectorGetDataStructSize(sector)];
//        memset(dst, 0, prvMemorySectorGetDataStructSize(sector));
//
//        if( MEM_OK != prvMemoryAccessSectorSingleDataEntry(&dst, sector, 1) )
//        {
//            return MEM_ERR;
//        }
//        length += snprintf (buffer+length, xBufferLen, " first timestamp: %i\n", common_read_32(&dst[0]));
//
//        memset(dst, 0, prvMemorySectorGetDataStructSize(sector));
//        uint32_t last_index = ( prvImmutableMemorySectorOrdinals[ sector ].info.bytesWritten / PAGE_SIZE * prvMemorySectorGetDataEntriesPerPage(sector) ) - 1;
//        if( MEM_OK != prvMemoryAccessSectorSingleDataEntry(&dst, sector, last_index) )
//        {
//            return MEM_ERR;
//        }
//
//        length += snprintf (buffer+length, xBufferLen, " last timestamp:  %i\n", common_read_32(&dst[0]));
        length += snprintf ( buffer + length, xBufferLen, "--------------------\r\n" );
    }

    return MEM_OK;
}


MemoryManagerStatus memory_manager_get_configurations ( GlobalConfigurationU * pConfigs )
{
    if ( pConfigs == NULL )
    {
        return MEM_ERR;
    }

    // validation
    if ( memcmp ( prvGlobalConfigurationDiskSnapshot.values.signature, MEMORY_MANAGER_DATA_INTEGRITY_SIGNATURE, MEMORY_MANAGER_DATA_INTEGRITY_SIGNATURE_BUFFER_LENGTH ) == 0 ) // signature sequences match)
    {
        pConfigs = &prvGlobalConfigurationDiskSnapshot;
        return MEM_OK;
    }

    ( void ) pConfigs;
    return MEM_ERR;
}

MemoryManagerStatus memory_manager_update_sensors_ground_data ( GroundDataU * _data )
{
    if ( _data == NULL )
    {
        return MEM_ERR;
    }

    prvGlobalConfigurationDiskSnapshot.values.system.ground_pressure = _data->pressure;

    prvMemoryWriteAsyncMetaDataSector ( ) ;
    return MEM_OK;
}