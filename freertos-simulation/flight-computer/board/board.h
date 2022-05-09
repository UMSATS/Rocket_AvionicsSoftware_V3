#ifndef AVIONICS_BOARD_H
#define AVIONICS_BOARD_H

#include <inttypes.h>

typedef enum
{
    BOARD_OK = 0, BOARD_ERROR = 1, BOARD_BUSY = 2, BOARD_TIMEOUT = 3, BOARD_SYS_CLOCK_CONFIG_ERROR = 4
} BoardStatus;


BoardStatus board_init          ( void);
void        board_error_handler ( const char* file, uint32_t line);
void        board_delay         ( uint32_t ms);
void        board_led_blink     ( uint32_t ms);


#endif //AVIONICS_BOARD_H
