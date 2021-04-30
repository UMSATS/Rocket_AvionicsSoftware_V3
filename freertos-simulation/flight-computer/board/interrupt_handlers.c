//
// Created by vasil on 12/11/2020.
//

#include "interrupt_handlers.h"
#include "configurations/UserConfig.h"

#if (userconf_FREE_RTOS_SIMULATOR_MODE_ON == 0)
#include "stm32f4xx_hal.h"
#else
#include "sim-port/hal_port.h"
#endif

#include "board/board.h"
#include "board/components/buzzer.h"

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim1;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

    /* USER CODE END NonMaskableInt_IRQn 0 */
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler ( void )
{
    /* USER CODE BEGIN HardFault_IRQn 0 */

    /* USER CODE END HardFault_IRQn 0 */
    while ( 1 )
    {
        buzz_delay ( 1000 );
        int count = 1000 * SECOND;
        while ( count != 0 )
        {
            //HAL_GPIO_WritePin (GPIOA, PIN2, GPIO_PIN_RESET); // sets second pin as low
            //count to 10,502.56 for proper delay_ms of 0.125 ms
            TIM2->CNT = 0; //Sets timer_thread_handle count to 0
            TIM2->CR1 |= 1; //Enables Timer
            while ( ( TIM2->SR & 1 ) != 1 )
            { } //Waits for timer_thread_handle to reach specified value
            TIM2->CR1 &= ~1; //Disables Timer
            TIM2->SR &= ~1; //Resets UIF pin
            //HAL_GPIO_WritePin (GPIOA, PIN2, GPIO_PIN_SET); //sets second pin as high
            TIM2->CNT = 0;
            TIM2->CR1 |= 1;
            while ( ( TIM2->SR & 1 ) != 1 )
            { }
            TIM2->CR1 &= ~1;
            TIM2->SR &= ~1;
            count -= 1;
        }
    }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
    /* USER CODE BEGIN MemoryManagement_IRQn 0 */

    /* USER CODE END MemoryManagement_IRQn 0 */
    while (1)
    {
        /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
        buzz_delay(1500);
        HAL_Delay(500);
        /* USER CODE END W1_MemoryManagement_IRQn 0 */
    }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
    /* USER CODE BEGIN BusFault_IRQn 0 */

    /* USER CODE END BusFault_IRQn 0 */
    while (1)
    {
        /* USER CODE BEGIN W1_BusFault_IRQn 0 */
        buzz_delay(2500);
        HAL_Delay(500);
        /* USER CODE END W1_BusFault_IRQn 0 */
    }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
    /* USER CODE BEGIN UsageFault_IRQn 0 */

    /* USER CODE END UsageFault_IRQn 0 */
    while (1)
    {
        /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
        buzz_delay(1000);
        int count = 1000 * SECOND;
        while (count != 0)
        {
            //HAL_GPIO_WritePin (GPIOA, PIN2, GPIO_PIN_RESET); // sets second pin as low
            //count to 10,502.56 for proper delay_ms of 0.125 ms
            TIM2->CNT = 0; //Sets timer_thread_handle count to 0
            TIM2->CR1 |= 1; //Enables Timer
            while((TIM2->SR & 1) != 1){} //Waits for timer_thread_handle to reach specified value
            TIM2->CR1 &= ~1; //Disables Timer
            TIM2->SR &= ~1; //Resets UIF pin
            //HAL_GPIO_WritePin (GPIOA, PIN2, GPIO_PIN_SET); //sets second pin as high
            TIM2->CNT = 0;
            TIM2->CR1 |= 1;
            while((TIM2->SR & 1) != 1){}
            TIM2->CR1 &= ~1;
            TIM2->SR &= ~1;
            count -= 1;
        }
        /* USER CODE END W1_UsageFault_IRQn 0 */
    }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
    /* USER CODE BEGIN DebugMonitor_IRQn 0 */

    /* USER CODE END DebugMonitor_IRQn 0 */
    /* USER CODE BEGIN DebugMonitor_IRQn 1 */

    /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles TIM1 update interrupt and TIM10 global interrupt.
  */
void TIM1_UP_TIM10_IRQHandler(void)
{
    /* USER CODE BEGIN TIM1_UP_TIM10_IRQn 0 */

    /* USER CODE END TIM1_UP_TIM10_IRQn 0 */
    HAL_TIM_IRQHandler(&htim1);
    /* USER CODE BEGIN TIM1_UP_TIM10_IRQn 1 */

    /* USER CODE END TIM1_UP_TIM10_IRQn 1 */
}
