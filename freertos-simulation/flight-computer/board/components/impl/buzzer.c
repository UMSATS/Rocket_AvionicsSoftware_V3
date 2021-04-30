//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// UMSATS 2018-2020
//
// Repository:
//  UMSATS>Avionics-2019
//
// File Description:
//  BUzzer that will buzz for a given amount of time
//
// History
// 2019-01-13 by Cole Wiebe
// - Created.
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// INCLUDES
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "board/components/buzzer.h"
#include "board/hardware_definitions.h"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
//  Initializes the pins being used by the buzzer and the timer_thread_handle
//
// Returns:
//  void
//-------------------------------------------------------------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
//  makes a buzzer buzz for a given amount of seconds
//
// Returns:
//  int seconds
//-------------------------------------------------------------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
//int main2(void)
//{
//    /* Initialize all configured peripherals & timer_thread_handle */
//    Initialization();
//    return(1);
//}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
/**
  * @brief System Clock Configuration
  * @retval None
  */
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
void buzz_delay ( int milliseconds )
{
    int count = milliseconds * SECOND;
    while ( count != 0 )
    {
        HAL_GPIO_WritePin ( BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET ); //sets first pin as high
        //HAL_GPIO_WritePin (GPIOA, PIN2, GPIO_PIN_RESET); // sets second pin as low
        //count to 10,502.56 for proper delay_ms of 0.125 ms
        TIM2->CNT = 0; //Sets timer_thread_handle count to 0
        TIM2->CR1 |= 1; //Enables Timer
        while ( ( TIM2->SR & 1 ) != 1 )
        {
        } //Waits for timer_thread_handle to reach specified value
        TIM2->CR1 &= ~1; //Disables Timer
        TIM2->SR &= ~1; //Resets UIF pin
        HAL_GPIO_WritePin ( BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET ); // sets first pin as low
        //HAL_GPIO_WritePin (GPIOA, PIN2, GPIO_PIN_SET); //sets second pin as high
        TIM2->CNT = 0;
        TIM2->CR1 |= 1;
        while ( ( TIM2->SR & 1 ) != 1 )
        {
        }
        TIM2->CR1 &= ~1;
        TIM2->SR &= ~1;
        count -= 1;
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
void buzzer_init ( void )
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOB_CLK_ENABLE ( );

    //set up output pins.
    GPIO_InitTypeDef GPIOInit;
    GPIOInit.Pin  = BUZZER_PIN;
    GPIOInit.Mode = GPIO_MODE_OUTPUT_PP;

    HAL_GPIO_Init ( BUZZER_PORT, &GPIOInit );

    /* Enables clock for timer_thread_handle */
    __HAL_RCC_TIM2_CLK_ENABLE ( );

    /* set value to count to */
    TIM2->ARR = 10500;
}
