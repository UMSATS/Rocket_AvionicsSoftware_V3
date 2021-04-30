/*
    FreeRTOS V7.1.0 - Copyright (C) 2011 Real Time Engineers Ltd.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************
    This file is part of the FreeRTOS distribution.
    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.
    1 tab == 4 spaces!
    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.
    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.
    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

/**
 * This is the simplest implementation of all. It does not permit memory to be
 * freed once it has been allocated. Despite this, heap_1.c is appropriate for
 * a large number of embedded applications. This is because many small and deeply
 * embedded applications create all the tasks, queues, semaphores, etc. required
 * when the system boots, and then use all of these objects for the lifetime of
 * program (until the application is switched off again, or is rebooted). Nothing
 * ever gets deleted.
 *
 * The implementation simply subdivides a single array into smaller blocks as RAM
 * is requested. The total size of the array (the total size of the heap) is set
 * by configTOTAL_HEAP_SIZE – which is defined in FreeRTOSConfig.h.
 * The configAPPLICATION_ALLOCATED_HEAP FreeRTOSConfig.h configuration constant is
 * provided to allow the heap to be placed at a specific address in memory.
 *
 * The xPortGetFreeHeapSize() API function returns the total amount of heap space
 * that remains unallocated, allowing the configTOTAL_HEAP_SIZE setting to be optimised.
 *
 * The heap_1 implementation:
 * Can be used if your application never deletes a task, queue, semaphore, mutex, etc.
 * (which actually covers the majority of applications in which FreeRTOS gets used).
 * Is always deterministic (always takes the same amount of time to execute) and cannot
 * result in memory fragmentation. Is very simple and allocated memory from a statically
 * allocated array, meaning it is often suitable for use in applications that do not permit
 * true dynamic memory allocation.
 */

/*
 * The simplest possible implementation of pvPortMalloc().  Note that this
 * implementation does NOT allow allocated memory to be freed again.
 *
 * See heap_2.c and heap_3.c for alternative implementations, and the memory
 * management pages of http://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* Allocate the memory for the heap.  The struct is used to force byte
alignment without using any non-portable code. */
static union xRTOS_HEAP
{
#if portBYTE_ALIGNMENT == 8
    volatile portDOUBLE dDummy;
#else
    volatile unsigned long ulDummy;
#endif
    unsigned char ucHeap[ configTOTAL_HEAP_SIZE ];
} xHeap;

static size_t xNextFreeByte = ( size_t ) 0;
/*-----------------------------------------------------------*/

void *pvPortMalloc( size_t xWantedSize )
{
    void *pvReturn = NULL;

    /* Ensure that blocks are always aligned to the required number of bytes. */
#if portBYTE_ALIGNMENT != 1
    if( xWantedSize & portBYTE_ALIGNMENT_MASK )
    {
        /* Byte alignment required. */
        xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
    }
#endif

    vTaskSuspendAll();
    {
        /* Check there is enough room left for the allocation. */
        if( ( ( xNextFreeByte + xWantedSize ) < configTOTAL_HEAP_SIZE ) &&
            ( ( xNextFreeByte + xWantedSize ) > xNextFreeByte )	)/* Check for overflow. */
        {
            /* Return the next free byte then increment the index past this
            block. */
            pvReturn = &( xHeap.ucHeap[ xNextFreeByte ] );
            xNextFreeByte += xWantedSize;
        }
    }
    xTaskResumeAll();

#if( configUSE_MALLOC_FAILED_HOOK == 1 )
    {
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
	}
#endif

    return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void *pv )
{
    /* Memory cannot be freed using this scheme.  See heap_2.c and heap_3.c
    for alternative implementations, and the memory management pages of
    http://www.FreeRTOS.org for more information. */
    ( void ) pv;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
    /* Only required when static memory is not cleared. */
    xNextFreeByte = ( size_t ) 0;
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
    return ( configTOTAL_HEAP_SIZE - xNextFreeByte );
}