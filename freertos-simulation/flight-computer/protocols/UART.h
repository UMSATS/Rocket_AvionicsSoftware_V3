#ifndef STM32F4XX_HAL_UART_CLI_H
#define STM32F4XX_HAL_UART_CLI_H
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// UMSATS 2018-2020
//
// Repository:
//  UMSATS>Avionics-2019
//
// File Description:
//  Header file for communicating with STM32 microchip via UART Serial Connection. Handles initialization and transmission/reception.
//
// History
// 2019-02-13 Eric Kapilik
// - Created.

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>

#define TIMEOUT_MAX 0xFFFF
#define BUFFER_SIZE 2048

typedef enum
{
    UART_OK = 0, UART_ERR = 1
} UARTStatus;

char PRINT_BUFFER[BUFFER_SIZE];
#define DISPLAY( format, ... )                                                               \
            sprintf(PRINT_BUFFER, format, ##__VA_ARGS__);                                    \
            uart6_transmit(PRINT_BUFFER);                                                    \

#define DEBUG( format, ... )                                                                 \
                sprintf(PRINT_BUFFER, format, ##__VA_ARGS__);                                \
                uart6_transmit_debug(PRINT_BUFFER);                                          \

#define DEBUG_LINE( format, ... )                                                            \
                sprintf(PRINT_BUFFER, format, ##__VA_ARGS__);                                \
                uart6_transmit_line_debug(PRINT_BUFFER);                                     \

#define DISPLAY_LINE( format, ... )                                                          \
                sprintf(PRINT_BUFFER, format, ##__VA_ARGS__);                                \
                uart6_transmit_line(PRINT_BUFFER);                                           \


int UART_Port2_init ( void );
int uart2_receive_command ( char * pData );
int uart2_transmit ( char const * message );
int uart2_transmit_bytes ( uint8_t * bytes, uint16_t numBytes );
int uart2_transmit_line_debug ( char const * message );
int uart2_transmit_line ( char const * message );

int UART_Port6_init ( void );
int uart6_transmit ( char const * message );
int uart6_transmit_line ( char const * message );
int uart6_transmit_line_debug ( char const * message );
int uart6_transmit_bytes ( uint8_t * bytes, uint16_t numBytes );
int uart6_transmit_debug ( char const * message );
int uart6_receive_command ( char * pData );

int uart2_receive ( uint8_t * buf, size_t size );
int uart6_receive ( uint8_t * buf, size_t size );

#endif //STM32F4XX_HAL_UART_CLI_H


#ifdef __cplusplus
}
#endif