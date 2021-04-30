/*    Avionics Software
 *
 *    File Description:
 *        Main file for the avionics software. The tasks are initialized here and the scheduler is started.
 *        Right now the default task is setup to blink the on-board LED.
 *
 *
 *    History:
 *    - 2019-01-22
 *        Created by Joseph Howarth
 *
 *
 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//  /* USER CODE BEGIN Callback 0 */
//
//  /* USER CODE END Callback 0 */
//  if (htim->Instance == TIM1) {
//    HAL_IncTick();
//  }
//
//}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{

  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/























/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 * NOTE 1: Windows will not be running the FreeRTOS demo threads continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Windows port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Windows
 * port for further information:
 * http://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html
 *
 * NOTE 2:  This project provides two demo applications.  A simple blinky style
 * project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky version.  Console output
 * is used in place of the normal LED toggling.
 *
 * NOTE 3:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, are defined
 * in main.c.
 ******************************************************************************
*/




#include <stddef.h>
#include <stdio.h>
#include "configurations/UserConfig.h"

#include "memory-management/memory_manager.h"

#include "board/board.h"
#include "protocols/UART.h"
#include "board/components/buzzer.h"
#include "board/components/flash.h"

#include "core/system_configuration.h"
#include "board/components/recovery.h"
#include "core/flight_controller.h"
#include "command-line-interface/controller.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"


#include "board/components/imu_sensor.h"
#include "board/components/pressure_sensor.h"
#include "board/hardware_definitions.h"

#if (userconf_FREE_RTOS_SIMULATOR_MODE_ON == 0)
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#endif

/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
int main( void )
{
    if ( board_init( ) != BOARD_OK )
    {
        board_error_handler( __FILE__, __LINE__ );
    }

    if ( ! UART_Port6_init() )
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "UMSATS ROCKETRY FLIGHT COMPUTER");
    }

    buzzer_init( );
    buzz_delay(500);
    DEBUG_LINE( "Buzzer has been set up.");

    recovery_init( );
    DEBUG_LINE( "Recovery GPIO pins have been set up.");

    if ( ! flash_init() )
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "Flash ID read successful");
    }

    if ( ! memory_manager_init( ) )
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "Memory Manager has been initialized and configured!");
    }

    if ( ! pressure_sensor_init( NULL ) )
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "Pressure sensor has been set up.");
    }

    if ( ! imu_sensor_init( NULL ) )
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "IMU sensor has been set up.");
    }

    if ( ! flight_controller_init(NULL) )
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "Flight controller has been set up.");
    }

    if ( ! memory_manager_start ( NULL ) )
    {
        board_error_handler( __FILE__, __LINE__ );
    } else
    {
        DEBUG_LINE( "Memory Manager has been started.");
    }

    command_line_interface_start ( NULL );
    flight_controller_start ( NULL );

    vTaskStartScheduler ( );
    for ( ;; );
}
/*-----------------------------------------------------------*/
























#if (userconf_FLASH_DISK_SIMULATION_ON == 1)

/* Standard includes. */

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The times are converted from
milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define mainTASK_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 200UL )
#define mainTIMER_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 2000UL )

/* The number of items the queue can hold at once. */
#define mainQUEUE_LENGTH					( 2 )

/* The values sent to the queue receive task from the queue send task and the
queue send software timer respectively. */
#define mainVALUE_SENT_FROM_TASK			( 100UL )
#define mainVALUE_SENT_FROM_TIMER			( 200UL )

/*-----------------------------------------------------------*/

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvQueueReceiveTask( void *pvParameters );
static void prvQueueSendTask( void *pvParameters );

/*
 * The callback function executed when the software timer expires.
 */
static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle );

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;

/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;


/* When configSUPPORT_STATIC_ALLOCATION is set to 1 the application writer can
use a callback function to optionally provide the memory required by the idle
and timer tasks.  This is the stack that will be used by the timer task.  It is
declared here, as a global, so it can be checked by a test that is implemented
in a different file. */
#define configTIMER_TASK_STACK_DEPTH			( configMINIMAL_STACK_SIZE * 2 )

StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

/* Notes if the trace is running or not. */
static BaseType_t xTraceRunning = pdTRUE;

/*-----------------------------------------------------------*/

/*
 * Prototypes for the standard FreeRTOS application hook (callback) functions
 * implemented within this file.  See http://www.freertos.org/a00016.html .
 */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );
















static void prvQueueSendTask( void *pvParameters )
{
TickType_t xNextWakeTime;
const TickType_t xBlockTime = mainTASK_SEND_FREQUENCY_MS;
const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TASK;

	/* Prevent the compiler warning about the unused parameter. */
	( void ) pvParameters;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for( ;; )
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil( &xNextWakeTime, xBlockTime );

		/* Send to the queue - causing the queue receive task to unblock and
		write to the console.  0 is used as the block time so the send operation
		will not block - it shouldn't need to block as the queue should always
		have at least one space at this point in the code. */
		xQueueSend( xQueue, &ulValueToSend, 0U );
	}
}
/*-----------------------------------------------------------*/

static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle )
{
const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

	/* This is the software timer callback function.  The software timer has a
	period of two seconds and is reset each time a key is pressed.  This
	callback function will execute if the timer expires, which will only happen
	if a key is not pressed for two seconds. */

	/* Avoid compiler warnings resulting from the unused parameter. */
	( void ) xTimerHandle;

	/* Send to the queue - causing the queue receive task to unblock and
	write out a message.  This function is called from the timer/daemon task, so
	must not block.  Hence the block time is set to 0. */
	xQueueSend( xQueue, &ulValueToSend, 0U );
}
/*-----------------------------------------------------------*/

static void prvQueueReceiveTask( void *pvParameters )
{
uint32_t ulReceivedValue;

	/* Prevent the compiler warning about the unused parameter. */
	( void ) pvParameters;

	for( ;; )
	{
		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h.  It will not use any CPU time while it is in the
		Blocked state. */
		xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

		/* To get here something must have been received from the queue, but
		is it an expected value?  Normally calling printf() from a task is not
		a good idea.  Here there is lots of stack space and only one task is
		using console IO so it is ok.  However, note the comments at the top of
		this file about the risks of making Windows system calls (such as
		console output) from a FreeRTOS task. */
		if( ulReceivedValue == mainVALUE_SENT_FROM_TASK )
		{
			printf( "Message received from task\r\n" );
		}
		else if( ulReceivedValue == mainVALUE_SENT_FROM_TIMER )
		{
			printf( "Message received from software timer\r\n" );
		}
		else
		{
			printf( "Unexpected message\r\n" );
		}

		fflush( stdout );
	}
}
/*-----------------------------------------------------------*/
void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then the
	size of the	heap available to pvPortMalloc() is defined by
	configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
	API function can be used to query the size of free heap space that remains
	(although it does not provide information on how the remaining heap might be
	fragmented).  See http://www.freertos.org/a00111.html for more
	information. */
	vAssertCalled( __LINE__, __FILE__ );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If application tasks make use of the
	vTaskDelete() API function to delete themselves then it is also important
	that vApplicationIdleHook() is permitted to return to its calling function,
	because it is the responsibility of the idle task to clean up memory
	allocated by the kernel to any task that has since deleted itself. */

	/* Uncomment the following code to allow the trace to be stopped with any
	key press.  The code is commented out by default as the kbhit() function
	interferes with the run time behaviour. */
	/*
		if( _kbhit() != pdFALSE )
		{
			if( xTraceRunning == pdTRUE )
			{
				vTraceStop();
				prvSaveTraceFile();
				xTraceRunning = pdFALSE;
			}
		}
	*/
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.  This function is
	provided as an example only as stack overflow checking does not function
	when running the FreeRTOS Windows port. */
	vAssertCalled( __LINE__, __FILE__ );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	/* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */
}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{
	/* This function will be called once only, when the daemon task starts to
	execute	(sometimes called the timer task).  This is useful if the
	application includes initialisation code that would benefit from executing
	after the scheduler has been started. */
}
/*-----------------------------------------------------------*/


/*-----------------------------------------------------------*/

static void prvSaveTraceFile( void )
{
	/* Tracing is not used when code coverage analysis is being performed. */
	#if( projCOVERAGE_TEST != 1 )
	{
//		FILE* pxOutputFile;
//
//		vTraceStop();
//
//		pxOutputFile = fopen( "Trace.dump", "wb");
//
//		if( pxOutputFile != NULL )
//		{
//			fwrite( RecorderDataPtr, sizeof( RecorderDataType ), 1, pxOutputFile );
//			fclose( pxOutputFile );
//			printf( "\r\nTrace output saved to Trace.dump\r\n" );
//		}
//		else
//		{
//			printf( "\r\nFailed to create trace dump file\r\n" );
//		}
	}
	#endif
}
/*-----------------------------------------------------------*/

static void  prvInitialiseHeap( void )
{
///* The Windows demo could create one large heap region, in which case it would
//be appropriate to use heap_4.  However, purely for demonstration purposes,
//heap_5 is used instead, so start by defining some heap regions.  No
//initialisation is required when any other heap implementation is used.  See
//http://www.freertos.org/a00111.html for more information.
//
//The xHeapRegions structure requires the regions to be defined in start address
//order, so this just creates one big array, then populates the structure with
//offsets into the array - with gaps in between and messy alignment just for test
//purposes. */
//static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
//volatile uint32_t ulAdditionalOffset = 19; /* Just to prevent 'condition is always true' warnings in configASSERT(). */
//HeapStats_t xHeapStats;
//const HeapStats_t xZeroHeapStats = { 0 };
//const HeapRegion_t xHeapRegions[] =
//{
//	/* Start address with dummy offsets						Size */
//	{ ucHeap + 1,											mainREGION_1_SIZE },
//	{ ucHeap + 15 + mainREGION_1_SIZE,						mainREGION_2_SIZE },
//	{ ucHeap + 19 + mainREGION_1_SIZE + mainREGION_2_SIZE,	mainREGION_3_SIZE },
//	{ NULL, 0 }
//};
//
//	/* Sanity check that the sizes and offsets defined actually fit into the
//	array. */
//	configASSERT( ( ulAdditionalOffset + mainREGION_1_SIZE + mainREGION_2_SIZE + mainREGION_3_SIZE ) < configTOTAL_HEAP_SIZE );
//
//	/* Prevent compiler warnings when configASSERT() is not defined. */
//	( void ) ulAdditionalOffset;
//
//	/* The heap has not been initialised yet so expect stats to all be zero. */
//	vPortGetHeapStats( &xHeapStats );
//	configASSERT( memcmp( &xHeapStats, &xZeroHeapStats, sizeof( HeapStats_t ) ) == 0 );
//
//	vPortDefineHeapRegions( xHeapRegions );
//
//	/* Sanity check vTaskGetHeapStats(). */
//	prvExerciseHeapStats();
}
/*-----------------------------------------------------------*/

//static void prvExerciseHeapStats( void )
//{
//HeapStats_t xHeapStats;
//size_t xInitialFreeSpace = xPortGetFreeHeapSize(), xMinimumFreeBytes;
//size_t xMetaDataOverhead, i;
//void *pvAllocatedBlock;
//const size_t xArraySize = 5, xBlockSize = 1000UL;
//void *pvAllocatedBlocks[ xArraySize ];
//
//	/* Check heap stats are as expected after initialisation but before any
//	allocations. */
//	vPortGetHeapStats( &xHeapStats );
//
//	/* Minimum ever free bytes remaining should be the same as the total number
//	of bytes as nothing has been allocated yet. */
//	configASSERT( xHeapStats.xMinimumEverFreeBytesRemaining == xHeapStats.xAvailableHeapSpaceInBytes );
//	configASSERT( xHeapStats.xMinimumEverFreeBytesRemaining == xInitialFreeSpace );
//
//	/* Nothing has been allocated or freed yet. */
//	configASSERT( xHeapStats.xNumberOfSuccessfulAllocations == 0 );
//	configASSERT( xHeapStats.xNumberOfSuccessfulFrees == 0 );
//
//	/* Allocate a 1000 byte block then measure what the overhead of the
//	allocation in regards to how many bytes more than 1000 were actually
//	removed from the heap in order to store metadata about the allocation. */
//	pvAllocatedBlock = pvPortMalloc( xBlockSize );
//	configASSERT( pvAllocatedBlock );
//	xMetaDataOverhead = ( xInitialFreeSpace - xPortGetFreeHeapSize() ) - xBlockSize;
//
//	/* Free the block again to get back to where we started. */
//	vPortFree( pvAllocatedBlock );
//	vPortGetHeapStats( &xHeapStats );
//	configASSERT( xHeapStats.xAvailableHeapSpaceInBytes == xInitialFreeSpace );
//	configASSERT( xHeapStats.xNumberOfSuccessfulAllocations == 1 );
//	configASSERT( xHeapStats.xNumberOfSuccessfulFrees == 1 );
//
//	/* Allocate blocks checking some stats value on each allocation. */
//	for( i = 0; i < xArraySize; i++ )
//	{
//		pvAllocatedBlocks[ i ] = pvPortMalloc( xBlockSize );
//		configASSERT( pvAllocatedBlocks[ i ] );
//		vPortGetHeapStats( &xHeapStats );
//		configASSERT( xHeapStats.xMinimumEverFreeBytesRemaining == ( xInitialFreeSpace - ( ( i + 1 ) * ( xBlockSize + xMetaDataOverhead ) ) ) );
//		configASSERT( xHeapStats.xMinimumEverFreeBytesRemaining == xHeapStats.xAvailableHeapSpaceInBytes );
//		configASSERT( xHeapStats.xNumberOfSuccessfulAllocations == ( 2Ul + i ) );
//		configASSERT( xHeapStats.xNumberOfSuccessfulFrees == 1 ); /* Does not increase during allocations. */
//	}
//
//	configASSERT( xPortGetFreeHeapSize() == xPortGetMinimumEverFreeHeapSize() );
//	xMinimumFreeBytes = xPortGetFreeHeapSize();
//
//	/* Free the blocks again. */
//	for( i = 0; i < xArraySize; i++ )
//	{
//		vPortFree( pvAllocatedBlocks[ i ] );
//		vPortGetHeapStats( &xHeapStats );
//		configASSERT( xHeapStats.xAvailableHeapSpaceInBytes == ( xInitialFreeSpace - ( ( ( xArraySize - i - 1 ) * ( xBlockSize + xMetaDataOverhead ) ) ) ) );
//		configASSERT( xHeapStats.xNumberOfSuccessfulAllocations == ( xArraySize + 1 ) ); /* Does not increase during frees. */
//		configASSERT( xHeapStats.xNumberOfSuccessfulFrees == ( 2UL + i ) );
//	}
//
//	/* The minimum ever free heap size should not change as blocks are freed. */
//	configASSERT( xMinimumFreeBytes == xPortGetMinimumEverFreeHeapSize() );
//}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

	/* Pass out a pointer to the StaticTask_t structure in which the Idle task's
	state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
static BaseType_t xPrinted = pdFALSE;
volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

	/* Called if an assertion passed to configASSERT() fails.  See
	http://www.freertos.org/a00110.html#configASSERT for more information. */

	/* Parameters are not used. */
	( void ) ulLine;
	( void ) pcFileName;


 	taskENTER_CRITICAL();
	{
		/* Stop the trace recording. */
		if( xPrinted == pdFALSE )
		{
			xPrinted = pdTRUE;
			if( xTraceRunning == pdTRUE )
			{
				prvSaveTraceFile();
			}
		}

		/* You can step out of this function to debug the assertion by using
		the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
		value. */
		while( ulSetToNonZeroInDebuggerToContinue == 0 )
		{
//			__asm volatile( "NOP" );
//			__asm volatile( "NOP" );
		}
	}
	taskEXIT_CRITICAL();
}


#endif


