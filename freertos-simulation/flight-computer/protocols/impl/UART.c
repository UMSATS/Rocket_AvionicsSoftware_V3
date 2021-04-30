//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// UMSATS 2018-2020
//
// Repository:
//  UMSATS>Avionics-2019
//
// File Description:
//  Source file for communicating with STM32 microchip via UART Serial Connection. Handles initialization and transmission/reception.
//
// History
// 2019-02-13 Eric Kapilik
// - Created.
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "protocols/UART.h"
#include <string.h>
#include <stdlib.h>
#include "main.h"

#include "FreeRTOS.h"
#include "portable.h"
#include "board/hardware_definitions.h"
#include <FreeRTOS.h>
#include <task.h>


static uint8_t prvBufftx[ BUFFER_SIZE ] = ""; // uart_transmit buffer
static uint8_t prvBuffrx[ BUFFER_SIZE ] = ""; // receive buffer

static UART_HandleTypeDef uart2 = { 0 };
static UART_HandleTypeDef uart6 = { 0 };


static void Error_Handler_UART ( void );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
int UART_Port2_init ( void )
{
    HAL_StatusTypeDef status;
    HAL_Init ( );
    __HAL_RCC_USART2_CLK_ENABLE( );
    GPIO_InitTypeDef  GPIO_InitStruct;

    // GPIO uses pins 2 & 3

    /* Setup UART2 TX/RX Pins */
    GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3; // USART_TX_Pin
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init ( GPIOA, &GPIO_InitStruct );

    uart2.Instance        = USART2;
    uart2.Init.BaudRate   = 115200;
    uart2.Init.WordLength = UART_WORDLENGTH_8B;
    uart2.Init.StopBits   = UART_STOPBITS_1;
    uart2.Init.Parity     = UART_PARITY_NONE;
    uart2.Init.Mode       = UART_MODE_TX_RX;
    uart2.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
//    uart2.Init.OverSampling = UART_OVERSAMPLING_16;

    status = !HAL_UART_Init ( &uart2 );

    return status;
}

int UART_Port6_init ( void )
{
    HAL_StatusTypeDef status;
    __HAL_RCC_USART6_CLK_ENABLE( );
    GPIO_InitTypeDef  GPIO_InitStruct;

    /* Setup UART6 TX Pin */
    GPIO_InitStruct.Pin       = UART_TX_PIN; //  USART_TX_Pin
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;

    HAL_GPIO_Init ( UART_TX_PORT, &GPIO_InitStruct );

    /* Setup UART6 RX Pin */
    GPIO_InitStruct.Pin       = UART_RX_PIN; //USART_RX_Pin
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init ( UART_RX_PORT, &GPIO_InitStruct );

    uart6.Instance          = USART6;
    uart6.Init.BaudRate     = 115200;
    uart6.Init.WordLength   = UART_WORDLENGTH_8B;
    uart6.Init.StopBits     = UART_STOPBITS_1;
    uart6.Init.Parity       = UART_PARITY_NONE;
    uart6.Init.Mode         = UART_MODE_TX_RX;
    uart6.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    uart6.Init.OverSampling = UART_OVERSAMPLING_16;
    status = !HAL_UART_Init ( &uart6 );

    return status;
}

static int uart_transmit ( UART_HandleTypeDef * huart, const char * message, bool flush )
{
    HAL_StatusTypeDef status;
    portENTER_CRITICAL( );
    {
        memset ( memcpy, 0, BUFFER_SIZE );
        size_t i = strlen ( message );
        memcpy ( &prvBufftx, message, i );

        if ( flush )
        {
            prvBufftx[ i++ ] = '\r';
            prvBufftx[ i++ ] = '\n';
            prvBufftx[ i++ ] = '\0';
        }

        status = HAL_UART_Transmit ( huart, prvBufftx, sizeof ( uint8_t ) * i, TIMEOUT_MAX );
    }
    portEXIT_CRITICAL( );
    return status;
}

int uart6_transmit_debug ( char const * message )
{
#if defined(PRINT_DEBUG_LOG)
    return uart_transmit ( &uart6, message, false );
#else
    return UART_OK;
#endif
}

int uart6_transmit_line_debug ( char const * message )
{
#if defined(PRINT_DEBUG_LOG)
    return uart_transmit ( &uart6, message, true );
#else
    return UART_OK;
#endif
}

static int uart_transmit_bytes ( UART_HandleTypeDef * huart, uint8_t * bytes, uint16_t numBytes )
{
    HAL_StatusTypeDef status;
    status = HAL_UART_Transmit ( huart, bytes, numBytes, TIMEOUT_MAX );
    return status;
}

static char * uart_receive_command ( UART_HandleTypeDef * huart )
{
    uint8_t c; //key pressed character
    size_t  i;

    c = '\0'; //clear out character received
    prvBuffrx[ 0 ] = '\0'; //clear out receive buffer
    i = 0; //start at beginning of index

    while ( i < BUFFER_SIZE )
    {
        //get character (BLOCKING COMMAND)
        if ( HAL_UART_Receive ( huart, &c, 1, 0xFFFF ) != HAL_OK )
        {
            //did not receive character for some reason.
        }

        //print the character back.
        if ( c != '\0' )
        {

            if ( HAL_UART_Transmit ( huart, &c, sizeof ( c ), TIMEOUT_MAX ) != HAL_OK )
            {
                //Do something meaningful here...
            }

            //adjust our buffer
            if ( c == '\r' )
            { //return entered, command is complete
                break;
            }
            else if ( c == 127 )
            { //User hits backspace, clear from buffer and display (backspace is \177 or 127)
                if ( i > 0 )
                {
                    i--;
                } //don't let i become negative
                prvBuffrx[ i ] = '\0';
            }
            else
            { //add character to end of receive buffer
                prvBuffrx[ i++ ] = c;
            }

        }
    }

    //put a new line for user display
    c = '\n';
    if ( HAL_UART_Transmit ( huart, &c, sizeof ( c ), TIMEOUT_MAX ) != HAL_OK )
    {
        //handle transmission error
    }

    prvBuffrx[ i ] = '\0'; //string terminator added to the end of the message

    return ( char * ) prvBuffrx;
}

static int uart_receive ( UART_HandleTypeDef * huart, uint8_t * buf, size_t size )
{
    size_t i = 0; //start at beginning of index
    while ( i < size )
    {
        //get character (BLOCKING COMMAND)
        if ( HAL_UART_Receive ( huart, &buf[ i++ ], 1, 0xFFFF ) != HAL_OK )
        {
            return i == size;
        }
    }

    return true;
}

int uart2_transmit ( const char * message )
{
    return uart_transmit ( &uart2, message, false );
}

int uart2_transmit_line ( const char * message )
{
    return uart_transmit ( &uart2, message, true );
}

int uart2_transmit_line_debug ( char const * message )
{
#if defined(PRINT_DEBUG_LOG)
    return uart_transmit ( &uart2, message, true );
#else
    return UART_OK;
#endif
}

int uart2_transmit_bytes ( uint8_t * bytes, uint16_t numBytes )
{
    return uart_transmit_bytes ( &uart2, bytes, numBytes );
}

char * uart2_receive_command ( )
{
    return uart_receive_command ( &uart2 );
}

int uart2_receive ( uint8_t * buf, size_t size )
{
    return uart_receive ( &uart2, buf, size );
}

int uart6_transmit ( const char * message )
{
    return uart_transmit ( &uart6, message, false );
}

int uart6_transmit_line ( const char * message )
{
    return uart_transmit ( &uart6, message, true );
}

int uart6_transmit_bytes ( uint8_t * bytes, uint16_t numBytes )
{
    return uart_transmit_bytes ( &uart6, bytes, numBytes );
}

char * uart6_receive_command ( )
{
    return uart_receive_command ( &uart6 );
}

int uart6_receive ( uint8_t * buf, size_t size )
{
    return uart_receive ( &uart6, buf, size );
}