//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// UMSATS Rocketry 2019
//
// Repository:
//  UMSATS>Avionics-2019
//
//
// File Description:
//  Header file for SPI.c.
//
// History
// 2019-02-06 by Joseph Howarth
// - Created.
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#ifndef SPI_H
#define SPI_H
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

typedef enum {
    SPI_OK = 0, SPI_ERR = 1
} SPIStatus;


int spi1_init ( );

int spi2_init ( );

int spi3_init ( );

// Description:
//  This function reads one or more bytes over the SPI bus, by sending multiple address bytes
//  and then reading multiple bytes.
//
// Parameters:
//     addr_buffer      A pointer to the buffer holding address to read from.
//	   addr_buffer_size The number of bytes in the address/command.
//     rx_buffer        A pointer to where the received bytes should be stored
//     rx_buffer_size   The number of bytes being  received.
//     timeout          The timeout value in milliseconds.
int spi1_receive ( uint8_t * addr_buffer, uint8_t addr_buffer_size, uint8_t * rx_buffer, uint16_t rx_buffer_size,
                   uint32_t timeout );

int spi2_receive ( uint8_t * addr_buffer, uint8_t addr_buffer_size, uint8_t * rx_buffer, uint16_t rx_buffer_size,
                   uint32_t timeout );

int spi3_receive ( uint8_t * addr_buffer, uint8_t addr_buffer_size, uint8_t * rx_buffer, uint16_t rx_buffer_size,
                   uint32_t timeout );


// Description:
//  This function transfers one or more bytes over the SPI bus.
//  It firstly sends multiple register address bytes.
//
// Parameters:
//     addr_buffer     	A pointer to the buffer holding the address to write to.
//	   addr_buffer_size	Number of bytes in the address/command.
//     tx_buffer       	A pointer to the bytes to send.
//     size            	The number of bytes being sent.
//     timeout         	The timeout value in milliseconds.
int spi1_send ( uint8_t * reg_addr, uint8_t reg_addr_size, uint8_t * tx_buffer, uint16_t tx_buffer_size, uint32_t timeout );

int spi2_send ( uint8_t * reg_addr, uint8_t reg_addr_size, uint8_t * tx_buffer, uint16_t tx_buffer_size, uint32_t timeout );

int spi3_send ( uint8_t * reg_addr, uint8_t reg_addr_size, uint8_t * tx_buffer, uint16_t tx_buffer_size, uint32_t timeout );


// Description: DO NOT USE. Will be deleted in future versions of the code!
//  This function reads one or more bytes over the SPI bus, by sending the address
//  and then reading
//
// Parameters:
//     addr_buffer     A pointer to the address to read from.
//     rx_buffer       A pointer to where the received bytes should be stored
//     total_size      The number of bytes being sent and received. (# of bytes read + 1)
//     timeout         The timeout value in milliseconds.

int spi1_read(uint8_t *addr_buffer, uint8_t *rx_buffer, uint16_t total_size, uint32_t timeout);

int spi2_read(uint8_t *addr_buffer, uint8_t *rx_buffer, uint16_t total_size, uint32_t timeout);

int spi3_read(uint8_t *addr_buffer, uint8_t *rx_buffer, uint16_t total_size, uint32_t timeout);


// Description:  DO NOT USE! Will be deleted in future versions of the code!
//  This function transfers one or more bytes over the SPI bus.
//  It firstly sends the register address (hard coded to be a 1 byte address).
//
// Parameters:
//     addr_buffer     A pointer to the address to write to.
//     tx_buffer       A pointer to the bytes to send.
//     size            The number of bytes being sent.
//     timeout         The timeout value in milliseconds.
int spi1_transmit(uint8_t *reg_addr, uint8_t *tx_buffer, uint16_t total_size, uint32_t timeout);

int spi2_transmit(uint8_t *reg_addr, uint8_t *tx_buffer, uint16_t total_size, uint32_t timeout);

int spi3_transmit(uint8_t *reg_addr, uint8_t *tx_buffer, uint16_t total_size, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* SPI_H_ */

