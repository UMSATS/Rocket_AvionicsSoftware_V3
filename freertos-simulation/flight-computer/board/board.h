#ifndef AVIONICS_BOARD_H
#define AVIONICS_BOARD_H

#include <inttypes.h>

typedef enum
{
    BOARD_OK,
    BOARD_ERROR,
    BOARD_BUSY,
    BOARD_TIMEOUT,
    BOARD_SYS_CLOCK_CONFIG_ERROR
} BoardStatus;


BoardStatus board_init          ( void);
void        board_error_handler ( const char* file, uint32_t line);
void        board_delay         ( uint32_t ms);
void        board_led_blink     ( uint32_t ms);


#endif //AVIONICS_BOARD_H
