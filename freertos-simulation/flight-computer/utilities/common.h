#ifndef AVIONICS_COMMON_H
#define AVIONICS_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "configurations/UserConfig.h"


#define DESERIALIZE( type, size )                                       \
    static inline type common_to_##type(uint8_t* src) {                 \
        union {                                                         \
            type value;                                                 \
            char bytes[size];                                           \
        } u;                                                            \
        memcpy(u.bytes, src, size);                                     \
        return u.value;                                                 \
    }                                                                   \

#define SERIALIZE( type, size )                                         \
    static inline void common_from_##type(type value, uint8_t* buf) {   \
        union {                                                         \
            type value;                                                 \
            char bytes[size];                                           \
        } u;                                                            \
        u.value = value;                                                \
        memcpy(buf, u.bytes, size);                                     \
    }                                                                   \


SERIALIZE( double, sizeof ( double ) )
SERIALIZE( float, sizeof ( float ) )
SERIALIZE( uint32_t, sizeof ( uint32_t ) )
SERIALIZE( int32_t, sizeof ( int32_t ) )
SERIALIZE( int16_t, sizeof ( int16_t ) )

DESERIALIZE( double, sizeof ( double ) )
DESERIALIZE( float, sizeof ( float ) )
DESERIALIZE( uint32_t, sizeof ( uint32_t ) )
DESERIALIZE( int32_t, sizeof ( int32_t ) )
DESERIALIZE( int16_t, sizeof ( int16_t ) )


static inline void common_write_32 ( uint32_t src, uint8_t * dest )
{
    dest[ 0 ] = ( uint8_t ) ( ( src >> 24 ) & 0xFF );
    dest[ 1 ] = ( uint8_t ) ( ( src >> 16 ) & 0xFF );
    dest[ 2 ] = ( uint8_t ) ( ( src >> 8 ) & 0xFF );
    dest[ 3 ] = ( uint8_t ) ( ( src >> 0 ) & 0xFF );
}

static inline void common_write_24 ( uint32_t src, uint8_t * dest )
{
    dest[ 1 ] = ( uint8_t ) ( ( src >> 16 ) & 0xFF );
    dest[ 2 ] = ( uint8_t ) ( ( src >> 8 ) & 0xFF );
    dest[ 3 ] = ( uint8_t ) ( ( src >> 0 ) & 0xFF );
}

static inline void common_write_16 ( uint16_t src, uint8_t * dest )
{
    dest[ 0 ] = ( uint8_t ) ( ( src >> 8 ) & 0xFF );
    dest[ 1 ] = ( uint8_t ) ( ( src >> 0 ) & 0xFF );
}

static inline uint32_t common_read_32 ( const uint8_t * src )
{
    return ( ( ( ( uint8_t ) src[ 0 ] ) << 24 ) & 0xFF ) |
           ( ( ( ( uint8_t ) src[ 1 ] ) << 16 ) & 0xFF ) |
           ( ( ( ( uint8_t ) src[ 2 ] ) << 8 ) & 0xFF )  |
           ( ( ( ( uint8_t ) src[ 3 ] ) << 0 ) & 0xFF );
}

static inline uint32_t common_read_24 ( const uint8_t * src )
{
    return ( ( ( ( uint8_t ) src[ 1 ] ) << 16 ) & 0xFF ) |
           ( ( ( ( uint8_t ) src[ 2 ] ) << 8 ) & 0xFF )  |
           ( ( ( ( uint8_t ) src[ 3 ] ) << 0 ) & 0xFF );
}

static inline uint16_t common_read_16 ( const uint8_t * src )
{
    return ( ( ( ( uint8_t ) src[ 0 ] ) << 8 ) & 0xFF ) |
           ( ( ( ( uint8_t ) src[ 1 ] ) << 0 ) & 0xFF );
}

static inline bool common_is_mem_empty ( uint8_t * buffer, size_t size )
{
#if ( userconf_FREE_RTOS_SIMULATOR_MODE_ON == 1 )
    const int EMPTY_VALUE = 0;
#else
    const uint8_t EMPTY_VALUE = 0xFF;
#endif

    uint16_t     emptyByteCounter = 0;
    for ( size_t i                = 0; i < size; i++ )
    {
        if ( buffer[ i ] == EMPTY_VALUE )
        {
            emptyByteCounter++;
        }
    }

    if ( emptyByteCounter == size )
    {
        return true;
    }

    return false;
}

static inline void common_clear_mem ( uint8_t * buffer, size_t size )
{
    memset ( buffer, 0, size );
}

static inline void common_float2bytes ( float float_variable, uint8_t * bytes_temp )
{
    union
    {
        float         a;
        unsigned char bytes[4];
    } thing;

    thing.a = float_variable;
    memcpy ( bytes_temp, thing.bytes, 4 );
}

#endif //AVIONICS_COMMON_H
