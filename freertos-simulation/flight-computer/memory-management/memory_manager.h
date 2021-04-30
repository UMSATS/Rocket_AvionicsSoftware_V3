#ifndef MEMORY_MANAGER_MEMORY_MANAGER_H
#define MEMORY_MANAGER_MEMORY_MANAGER_H

#include <inttypes.h>
#include "board/components/recovery.h"
#include "core/system_configuration.h"
#include "event-detection/event_detector.h"

typedef enum MemoryManagerStatus
{
    MEM_ERR                             = 0,
    MEM_OK                              = 1

} MemoryManagerStatus;

typedef enum MetaDataUpdateFrequencyMode
{

    MetaDataUpdateDataBasedFrequencyMode       = 0,
    MetaDataUpdateTimeBasedFrequencyMode       = 1,
    MetaDataUpdateFrequencyModeCount

} MetaDataUpdateFrequencyMode;

// sectors that can be modified only before the flight
typedef enum SystemSector
{

    SystemSectorGlobalConfigurationData       = 0,
    SystemSectorUserDataSectorMetaData        = 1,
    SystemSectorCount

} SystemSector;

// sectors that can be modified at any time
typedef enum UserDataSector
{

    UserDataSectorGyro                        = 0,
    UserDataSectorAccel                       = 1,
    UserDataSectorMag                         = 2,
    UserDataSectorPressure                    = 3,
    UserDataSectorTemperature                 = 4,
    UserDataSectorContinuity                  = 5,
    UserDataSectorFlightEvent                 = 6,
    UserDataSectorCount

} UserDataSector;

typedef enum MemorySector
{
    
    MemorySystemSectorGlobalConfigurationData = 0,
    MemorySystemSectorUserDataSectorMetaData  = 1,
    MemoryUserDataSectorGyro                  = 2,
    MemoryUserDataSectorAccel                 = 3,
    MemoryUserDataSectorMag                   = 4,
    MemoryUserDataSectorPressure              = 5,
    MemoryUserDataSectorTemperature           = 6,
    MemoryUserDataSectorContinuity            = 7,
    MemoryUserDataSectorFlightEvent           = 8,
    MemorySectorCount                         = 9

} MemorySector;

typedef struct MemorySectorInfo
{
    uint32_t size;
    uint32_t startAddress;
    uint32_t endAddress;
    uint32_t bytesWritten;

} MemorySectorInfo;

typedef struct BufferInfo
{
    uint32_t startAddress;
    uint32_t endAddress;
    uint32_t bytesWritten;

} BufferInfo;

typedef struct MemoryBuffer
{
    BufferInfo info;
    uint8_t data[256];

} MemoryBuffer;

typedef struct MemorySectorBuffer
{
    MemoryBuffer buffers [2];
    MemoryBuffer * read;
    MemoryBuffer * write;

} MemorySectorBuffer;


// frequency multipliers to modify the frequency for different sensors at different stages
typedef struct MemoryManagerConfiguration
{
    // to keep memory alignment right using 4 of 8-bits integers
    // counted as one 32-bit integer
    // 4 bytes
    uint8_t  write_pre_launch_multiplier;             // +
    uint8_t  write_pre_apogee_multiplier;             // +
    uint8_t  write_post_apogee_multiplier;            // +
    uint8_t  write_ground_multiplier;                 // +
    // 4 bytes
    uint16_t write_interval_accelerometer_ms;         // +
    // 4 bytes
    uint16_t write_interval_gyroscope_ms;             // +
    uint16_t write_interval_magnetometer_ms;          // +
    // 4 bytes
    uint16_t write_interval_pressure_ms;              // +
    uint16_t write_interval_altitude_ms;              // +
    // 4 bytes
    uint16_t write_interval_temperature_ms;           // +
    uint16_t write_interval_flight_state_ms;          // +
    // 4 bytes
    uint16_t write_drogue_continuity_ms;              // +
    uint16_t write_main_continuity_ms;                // +

    uint32_t user_data_sector_sizes [ UserDataSectorCount ];

} MemoryManagerConfiguration;

typedef union MemoryManagerConfigurationU
{
    MemoryManagerConfiguration values;
    uint8_t bytes [ sizeof ( MemoryManagerConfiguration ) ];

} MemoryManagerConfigurationU;


typedef union MemoryManagerConfigurationContainer
{
    // can only be reset before the flight
    uint8_t updated;
    MemoryManagerConfigurationU data;

} MemoryManagerConfigurationContainer;

// memory layout metadata entry: describes the sectors locations, sizes, current address for the last known write operation , etc.
// can be used for both UserDataSector and SystemSector
typedef union MemoryLayoutMetaDataU
{
    struct memory_layout_meta_data_values {
        uint8_t signature              [ 12 ]; // for extra validation (security precaution)
        MemorySectorInfo user_sectors  [ UserDataSectorCount ];
    } __attribute__((packed)) values;

    uint8_t bytes [ sizeof ( struct memory_layout_meta_data_values ) ];

} MemoryLayoutMetaDataU;

typedef struct MemoryLayoutMetaDataContainer
{
    uint8_t                updated;
    MemoryLayoutMetaDataU  data;

} MemoryLayoutMetaDataContainer;


typedef union GlobalConfigurationU
{
    struct configuration_values {
        uint8_t signature [ 12 ]; // for extra validation (security precaution)
        MemoryManagerConfiguration memory;
        FlightSystemConfiguration  system;
    } __attribute__((packed)) values;

    uint8_t bytes[ sizeof ( struct configuration_values ) ];

} GlobalConfigurationU;

// can be modified only before the flight
typedef struct GlobalConfigurationContainer
{
    uint8_t               updated;
    GlobalConfigurationU  data;

} GlobalConfigurationContainer;


// ---------------------------------------------------------- //
//-------------------- SENSOR DATA CONTAINERS --------------- //


/*-------------------------- IMU -----------------------------*/
typedef union IMUDataU
{
    struct imu_values {
        uint32_t timestamp;
        float data [ 3 ];
    } __attribute__((packed)) values;

    uint8_t bytes [ sizeof ( struct imu_values ) ];

} IMUDataU;

typedef struct IMUDataContainer
{
    uint8_t     updated;
    IMUDataU    data;

} IMUDataContainer;

/*-----------------------------------------------------------*/

/*-------------------------- Pressure Sensor ----------------*/

typedef union PressureDataU
{
    struct pressure_values {
        uint32_t timestamp;
        float    data;
    } __attribute__((packed)) values;
    uint8_t bytes [ sizeof ( struct pressure_values ) ];

} PressureDataU;

typedef struct PressureDataContainer
{
    uint8_t         updated;
    PressureDataU   data;
} PressureDataContainer;

/*-----------------------------------------------------------*/

/*-------------------------- Continuity Status --------------*/
typedef union ContinuityU
{
    struct continuity_values {
        uint32_t timestamp;
        RecoveryContinuityStatus status [ RecoverySelectCount ] ;
        uint8_t _ [244];
    } __attribute__((packed)) values;

    uint8_t bytes [ sizeof ( struct continuity_values ) ];

} ContinuityU;

typedef struct ContinuityDataContainer
{
    uint8_t     updated;
    ContinuityU data;
} ContinuityDataContainer;
/*-----------------------------------------------------------*/

/*-------------------------- Flight Event ------------------*/
typedef union FlightEventU
{
    struct flight_event_values {
        uint32_t timestamp;
        FlightState status;
        uint8_t _ [248];
    } __attribute__((packed)) values;

    uint8_t bytes [ sizeof ( struct flight_event_values ) ];

} __attribute__((packed)) FlightEventU ;


typedef struct FlightEventDataContainer
{
    uint8_t         updated;
    FlightEventU    data;

} FlightEventDataContainer;
/*-----------------------------------------------------------*/




typedef struct DataContainer
{
    uint32_t                    tmstp;
    IMUDataContainer            gyro;
    IMUDataContainer            acc;
    IMUDataContainer            mag;
    PressureDataContainer       press;
    PressureDataContainer       temp;
    ContinuityDataContainer     cont;
    FlightEventDataContainer    event;
} DataContainer;


typedef union
{
    int64_t pressure;
    uint8_t bytes [ sizeof ( int64_t ) ];
} GroundDataU;


MemoryManagerStatus memory_manager_init ( );
MemoryManagerStatus memory_manager_user_data_update ( DataContainer * _container );
MemoryManagerStatus memory_manager_start ( );
MemoryManagerStatus memory_manager_stop ( );
MemoryManagerStatus memory_manager_get_system_configurations ( FlightSystemConfiguration * systemConfiguration );
MemoryManagerStatus memory_manager_get_memory_configurations ( MemoryManagerConfiguration * memoryConfiguration );
MemoryManagerStatus memory_manager_set_system_configurations ( FlightSystemConfiguration * systemConfiguration );
MemoryManagerStatus memory_manager_set_memory_configurations ( MemoryManagerConfiguration * memoryConfiguration );
MemoryManagerConfiguration memory_manager_get_default_memory_configurations ( );
void memory_manager_set_metadata_update_mode ( MetaDataUpdateFrequencyMode mode );


MemoryManagerStatus memory_manager_get_single_data_entry ( MemorySector sector, void * dst, uint32_t entry_index );
MemoryManagerStatus memory_manager_get_last_data_entry ( MemorySector sector, void * dst );


MemoryManagerStatus memory_manager_get_stats ( char * buffer, size_t xBufferLen );


MemoryManagerStatus memory_manager_erase_configuration_section ( );
MemoryManagerStatus memory_manager_erase_everything ( );



#endif //MEMORY_MANAGER_MEMORY_MANAGER_H
