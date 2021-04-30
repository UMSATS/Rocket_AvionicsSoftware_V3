//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// UMSATS Rocketry 2019
//
// File Description:
//  Functions for sending and reading over SPI.
//
// History
// 2019-02-06 by Joseph Howarth
// - Created.
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// INCLUDES
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "protocols/SPI.h"
#include "board/hardware_definitions.h"
#include "FreeRTOS.h"
#include "portable.h"


#define SPI1_CS_PIN     FLASH_SPI_CS_PIN
#define SPI1_CS_PORT    FLASH_SPI_CS_PORT

#define SPI2_CS_PIN     PRES_SPI_CS_PIN
#define SPI2_CS_PORT    PRES_SPI_CS_PORT

// Right now the timeout value is used to select between chip selects.
// We should really find a better way to do this!
// Currently only works with the send and receive functions.
// timeout=10 for acc
// timeout=other for gyro

#define SPI3_CS1_PIN    IMU_SPI_ACC_CS_PIN
#define SPI3_CS1_PORT   IMU_SPI_ACC_CS_PORT

#define SPI3_CS2_PIN    IMU_SPI_GYRO_CS_PIN
#define SPI3_CS2_PORT   IMU_SPI_GYRO_CS_PORT

static SPI_HandleTypeDef hspi1 = { .Instance = 0 };
static SPI_HandleTypeDef hspi2 = { .Instance = 0 };
static SPI_HandleTypeDef hspi3 = { .Instance = 0 };;


int spi1_init ( void )
{
    HAL_StatusTypeDef status;
    __HAL_RCC_SPI1_CLK_ENABLE( );
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial     = 10;

    status = HAL_SPI_Init ( &hspi1 );
    if ( status != HAL_OK )
    {
        return SPI_ERR;
    }


    /*SPI1 GPIO Configuration
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI
    PA9     ------> SPI1_CS
    */
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    __HAL_RCC_GPIOA_CLK_ENABLE( );
    __HAL_RCC_GPIOC_CLK_ENABLE( );

    //Setup the SPI MOSI,MISO and SCK. These pins are fixed.
    GPIO_InitStruct.Pin       = FLASH_SPI_SCK_PIN | FLASH_SPI_MOSI_PIN | FLASH_SPI_MISO_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init ( FLASH_SPI_PORT, &GPIO_InitStruct );

    //Setup the SPI CS. This can be any pin.
    GPIO_InitStruct.Pin       = SPI1_CS_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = 0;

    HAL_GPIO_Init ( SPI1_CS_PORT, &GPIO_InitStruct );
    HAL_GPIO_WritePin ( SPI1_CS_PORT, SPI1_CS_PIN, GPIO_PIN_SET );

    return SPI_OK;
}

int spi2_init ( void )
{
    __HAL_RCC_SPI2_CLK_ENABLE( );
    __HAL_RCC_GPIOB_CLK_ENABLE( );

    hspi2.Instance               = SPI2;
    hspi2.Init.Mode              = SPI_MODE_MASTER;
    hspi2.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi2.Init.NSS               = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial     = 10;
    if ( HAL_SPI_Init ( &hspi2 ) != HAL_OK )
    {
        return SPI_ERR;
    }


    /*SPI2 GPIO Configuration
    PB13     ------> SPI1_SCK
    PB14     ------> SPI1_MISO
    PB15     ------> SPI1_MOSI
    PC7     ------> SPI1_CS
    */
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    __HAL_RCC_GPIOC_CLK_ENABLE( );

    //Setup the SPI MOSI,MISO and SCK. These pins are fixed.
    GPIO_InitStruct.Pin       = PRES_SPI_SCK_PIN | PRES_SPI_MOSI_PIN | PRES_SPI_MISO_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init ( PRES_SPI_PORT, &GPIO_InitStruct );

    //Setup the SPI CS. This can be any pin.
    GPIO_InitStruct.Pin       = SPI2_CS_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = 0;

    HAL_GPIO_Init ( SPI2_CS_PORT, &GPIO_InitStruct );

    return SPI_OK;
}

int spi3_init ( void )
{
    __HAL_RCC_SPI3_CLK_ENABLE( );
    __HAL_RCC_GPIOC_CLK_ENABLE( );

    hspi3.Instance               = SPI3;
    hspi3.Init.Mode              = SPI_MODE_MASTER;
    hspi3.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi3.Init.NSS               = SPI_NSS_SOFT;
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi3.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial     = 10;
    if ( HAL_SPI_Init ( &hspi3 ) != HAL_OK )
    {
        return SPI_ERR;
    }

    /*SPI3 GPIO Configuration
    PC10     ------> SPI1_SCK
    PC11     ------> SPI1_MISO
    PC12     ------> SPI1_MOSI
    */

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    __HAL_RCC_GPIOC_CLK_ENABLE( );
    __HAL_RCC_GPIOB_CLK_ENABLE( );

    //Setup the SPI MOSI,MISO and SCK. These pins are fixed.
    GPIO_InitStruct.Pin       = IMU_SPI_SCK_PIN | IMU_SPI_MOSI_PIN | IMU_SPI_MISO_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;

    HAL_GPIO_Init ( IMU_SPI_PORT, &GPIO_InitStruct );
    GPIO_InitTypeDef GPIO_InitStruct2 = { 0 };
    //Setup the SPI CS. This can be any pin.
    GPIO_InitStruct2.Pin = SPI3_CS1_PIN | SPI3_CS2_PIN;

    GPIO_InitStruct2.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct2.Pull      = GPIO_NOPULL;
    GPIO_InitStruct2.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct2.Alternate = 0;

    HAL_GPIO_Init ( SPI3_CS1_PORT, &GPIO_InitStruct2 );

    //Make sure both CS are high. Undefined behavior if these are not high be for communication begins.
    HAL_GPIO_WritePin ( SPI3_CS1_PORT, SPI3_CS1_PIN, GPIO_PIN_SET );
    HAL_GPIO_WritePin ( SPI3_CS2_PORT, SPI3_CS2_PIN, GPIO_PIN_SET );

    return SPI_OK;
}

static SPIStatus transmit ( SPI_HandleTypeDef * hspi, uint8_t * reg_addr, uint8_t * tx_buffer, uint16_t size, uint32_t timeout )
{
    HAL_SPI_StateTypeDef statSPI = HAL_SPI_STATE_RESET;
    HAL_StatusTypeDef    statHAL = HAL_ERROR;
    GPIO_TypeDef         * port  = SPI1_CS_PORT;
    uint16_t             pin     = 0;

    if ( hspi->Instance == SPI1 )
    {
        port = SPI1_CS_PORT;
        pin  = SPI1_CS_PIN;
    }
    else if ( hspi->Instance == SPI2 )
    {
        port = SPI2_CS_PORT;
        pin  = SPI2_CS_PIN;
    }
    else if ( hspi->Instance == SPI3 )
    {
        port = SPI3_CS1_PORT;
        pin  = SPI3_CS1_PIN;
    }
    else
    {
        return SPI_ERR;
    }

    // Write the CS low (lock)
    HAL_GPIO_WritePin ( port, pin, GPIO_PIN_RESET );

    /* Select the slave register (**1 byte address**) first via a uart_transmit */
    statHAL = HAL_SPI_Transmit ( hspi, reg_addr, 1, timeout );
    if ( statHAL != HAL_OK )
    {
        while ( statSPI != HAL_SPI_STATE_READY )
        {
            statSPI = HAL_SPI_GetState ( hspi );
        }
    }
    /* Send the tx_buffer to slave */

    if ( size > 1 )
    {
        statHAL = HAL_SPI_Transmit ( hspi, tx_buffer, size, timeout );
        if ( statHAL != HAL_OK )
        {
            while ( statSPI != HAL_SPI_STATE_READY )
            {
                statSPI = HAL_SPI_GetState ( hspi );
            }
        }
    }

    // Write the CS hi (release)
    HAL_GPIO_WritePin ( port, pin, GPIO_PIN_SET );

    return SPI_OK;
}


static int receive ( SPI_HandleTypeDef * hspi, uint8_t * addr_buffer, uint8_t addr_buffer_size, uint8_t * rx_buffer, uint16_t rx_buffer_size, uint32_t timeout )
{
    HAL_SPI_StateTypeDef statSPI = HAL_SPI_STATE_RESET;
    HAL_StatusTypeDef    statHAL = HAL_ERROR;
    GPIO_TypeDef         * port  = SPI1_CS_PORT;
    uint16_t             pin     = 0;
    HAL_StatusTypeDef    stat;

    if ( hspi->Instance == SPI1 )
    {
        port = SPI1_CS_PORT;
        pin  = SPI1_CS_PIN;
    }
    else if ( hspi->Instance == SPI2 )
    {
        port = SPI2_CS_PORT;
        pin  = SPI2_CS_PIN;
    }
    else if ( hspi->Instance == SPI3 )
    {

        if ( timeout == 10 )
        {
            port = SPI3_CS1_PORT;
            pin  = SPI3_CS1_PIN;
        }
        else
        {
            port = SPI3_CS2_PORT;
            pin  = SPI3_CS2_PIN;
        }

    }
    else
    {
        return 1;
    }
    //Write the CS low
    HAL_GPIO_WritePin ( port, pin, GPIO_PIN_RESET );

    //Could also use HAL_TransmittReceive.

    //Send the address to read from.
    if ( addr_buffer_size > 0 )
    {
        statHAL = HAL_SPI_Transmit ( hspi, addr_buffer, addr_buffer_size, timeout );
        if ( statHAL != HAL_OK )
        {
            while ( statSPI != HAL_SPI_STATE_READY )
            {
                statSPI = HAL_SPI_GetState ( hspi );
            }
        }
    }
    //Read in the specified number of bytes.
    if ( rx_buffer_size > 0 )
    {
        statHAL = HAL_SPI_Receive ( hspi, rx_buffer, rx_buffer_size, timeout );
        if ( statHAL != HAL_OK )
        {
            while ( statSPI != HAL_SPI_STATE_READY )
            {
                statSPI = HAL_SPI_GetState ( hspi );
            }
        }
    }

    HAL_GPIO_WritePin ( port, pin, GPIO_PIN_SET );
    return SPI_OK;
}

static SPIStatus read ( SPI_HandleTypeDef * hspi, uint8_t * addr_buffer, uint8_t * rx_buffer, uint16_t total_size, uint32_t timeout )
{
    HAL_SPI_StateTypeDef statSPI = HAL_SPI_STATE_RESET;
    HAL_StatusTypeDef    statHAL = HAL_ERROR;

    GPIO_TypeDef      * port = SPI1_CS_PORT;
    uint16_t          pin    = 0;
    HAL_StatusTypeDef stat;

    if ( hspi->Instance == SPI1 )
    {
        port = SPI1_CS_PORT;
        pin  = SPI1_CS_PIN;
    }
    else if ( hspi->Instance == SPI2 )
    {
        port = SPI2_CS_PORT;
        pin  = SPI2_CS_PIN;
    }
    else if ( hspi->Instance == SPI3 )
    {
        port = SPI3_CS1_PORT;
        pin  = SPI3_CS1_PIN;
    }
    else
    {
        return SPI_ERR;
    }
    //Write the CS low
    HAL_GPIO_WritePin ( port, pin, GPIO_PIN_RESET );

    //Could also use HAL_TransmittReceive.

    //Send the address to read from.
    statHAL = HAL_SPI_Transmit ( hspi, addr_buffer, 1, timeout );
    if ( statHAL != HAL_OK )
    {
        while ( statSPI != HAL_SPI_STATE_READY )
        {
            statSPI = HAL_SPI_GetState ( hspi );
        }
    }

    //Read in the specified number of bytes.
    statHAL = HAL_SPI_Receive ( hspi, rx_buffer, total_size - 1, timeout );
    if ( statHAL != HAL_OK )
    {
        while ( statSPI != HAL_SPI_STATE_READY )
        {
            statSPI = HAL_SPI_GetState ( hspi );
        }
    }

    HAL_GPIO_WritePin ( port, pin, GPIO_PIN_SET );

    return SPI_OK;
}


static int send ( SPI_HandleTypeDef * hspi, uint8_t * reg_addr, uint8_t reg_addr_size, uint8_t * tx_buffer, uint16_t tx_buffer_size, uint32_t timeout )
{
    HAL_SPI_StateTypeDef statSPI = HAL_SPI_STATE_RESET;
    HAL_StatusTypeDef    statHAL = HAL_ERROR;
    GPIO_TypeDef         * port  = SPI1_CS_PORT;
    uint16_t             pin     = 0;

    if ( hspi->Instance == SPI1 )
    {
        port = SPI1_CS_PORT;
        pin  = SPI1_CS_PIN;
    }
    else if ( hspi->Instance == SPI2 )
    {
        port = SPI2_CS_PORT;
        pin  = SPI2_CS_PIN;
    }
    else if ( hspi->Instance == SPI3 )
    {
        if ( timeout == 10 )
        {
            port = SPI3_CS1_PORT;
            pin  = SPI3_CS1_PIN;
        }
        else
        {
            port = SPI3_CS2_PORT;
            pin  = SPI3_CS2_PIN;
        }
    }
    else
    {
        return SPI_ERR;
    }

    //Write the CS low (lock)
    HAL_GPIO_WritePin ( port, pin, GPIO_PIN_RESET );

    /* Select the slave register (**1 byte address**) first via a uart_transmit */
    if ( reg_addr_size > 0 )
    {
        statHAL = HAL_SPI_Transmit ( hspi, reg_addr, reg_addr_size, timeout );
        if ( statHAL != HAL_OK )
        {
            while ( statSPI != HAL_SPI_STATE_READY )
            {
                statSPI = HAL_SPI_GetState ( hspi );
            }
        }
    }
    /* Send the tx_buffer to slave */
    if ( tx_buffer_size > 0 )
    {
        statHAL = HAL_SPI_Transmit ( hspi, tx_buffer, tx_buffer_size, timeout );
        if ( statHAL != HAL_OK )
        {
            while ( statSPI != HAL_SPI_STATE_READY )
            {
                statSPI = HAL_SPI_GetState ( hspi );
            }
        }
    }

    //Write the CS hi (release)
    HAL_GPIO_WritePin ( port, pin, GPIO_PIN_SET );

    return SPI_OK;
}

int spi1_transmit ( uint8_t * addr_buffer, uint8_t * tx_buffer, uint16_t total_size, uint32_t timeout )
{
    return transmit ( &hspi1, addr_buffer, tx_buffer, total_size, timeout );
}

int spi1_read ( uint8_t * addr_buffer, uint8_t * rx_buffer, uint16_t total_size, uint32_t timeout )
{
    return read ( &hspi1, addr_buffer, rx_buffer, total_size, timeout );
}

int spi1_send ( uint8_t * reg_addr, uint8_t reg_addr_size, uint8_t * tx_buffer, uint16_t tx_buffer_size, uint32_t timeout )
{
    return send ( &hspi1, reg_addr, reg_addr_size, tx_buffer, tx_buffer_size, timeout );
}

int spi1_receive ( uint8_t * addr_buffer, uint8_t addr_buffer_size, uint8_t * rx_buffer, uint16_t rx_buffer_size, uint32_t timeout )
{
    return receive ( &hspi1, addr_buffer, addr_buffer_size, rx_buffer, rx_buffer_size, timeout );
}


int spi2_transmit ( uint8_t * addr_buffer, uint8_t * tx_buffer, uint16_t total_size, uint32_t timeout )
{
    return transmit ( &hspi2, addr_buffer, tx_buffer, total_size, timeout );
}

int spi2_read ( uint8_t * addr_buffer, uint8_t * rx_buffer, uint16_t total_size, uint32_t timeout )
{
    return read ( &hspi2, addr_buffer, rx_buffer, total_size, timeout );
}

int spi2_send ( uint8_t * reg_addr, uint8_t reg_addr_size, uint8_t * tx_buffer, uint16_t tx_buffer_size, uint32_t timeout )
{
    return send ( &hspi2, reg_addr, reg_addr_size, tx_buffer, tx_buffer_size, timeout );
}

int spi2_receive ( uint8_t * addr_buffer, uint8_t addr_buffer_size, uint8_t * rx_buffer, uint16_t rx_buffer_size, uint32_t timeout )
{
    return receive ( &hspi2, addr_buffer, addr_buffer_size, rx_buffer, rx_buffer_size, timeout );
}


int spi3_transmit ( uint8_t * addr_buffer, uint8_t * tx_buffer, uint16_t total_size, uint32_t timeout )
{
    return transmit ( &hspi3, addr_buffer, tx_buffer, total_size, timeout );
}

int spi3_read ( uint8_t * addr_buffer, uint8_t * rx_buffer, uint16_t total_size, uint32_t timeout )
{
    return read ( &hspi3, addr_buffer, rx_buffer, total_size, timeout );
}

int spi3_send ( uint8_t * reg_addr, uint8_t reg_addr_size, uint8_t * tx_buffer, uint16_t tx_buffer_size, uint32_t timeout )
{
    return send ( &hspi3, reg_addr, reg_addr_size, tx_buffer, tx_buffer_size, timeout );
}

int spi3_receive ( uint8_t * addr_buffer, uint8_t addr_buffer_size, uint8_t * rx_buffer, uint16_t rx_buffer_size, uint32_t timeout )
{
    return receive ( &hspi3, addr_buffer, addr_buffer_size, rx_buffer, rx_buffer_size, timeout );
}


