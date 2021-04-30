#include "queue.h"
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <assert.h>
#include <protocols/UART.h>
#include "utilities/common.h"


void buffer_queue_init(buffer_queue *cb)
{
    cb->capacity = BUFFER_QUEUE_CAPACITY;
    cb->count = 0;
    cb->sz = sizeof( buffer_item );
    cb->head = cb->buffer;
    cb->tail = cb->buffer;
    cb->buffer_end = &cb->buffer[cb->capacity * cb->sz];
}

int buffer_queue_push_back(buffer_queue *cb, buffer_item *item)
{
    if(cb->count == cb->capacity)
    {
        return 1;
    }

    taskENTER_CRITICAL();
    {
        memcpy( cb->head, item, cb->sz );
        cb->head = ( uint8_t * ) cb->head + cb->sz;
        if ( cb->head == cb->buffer_end )
            cb->head = cb->buffer;
        cb->count++;
    }
    taskEXIT_CRITICAL();

    return 0;
}

int buffer_queue_pop_front(buffer_queue *cb, buffer_item *item)
{
    if(cb->count == 0)
    {
        return 0;
    }

    taskENTER_CRITICAL();
    {
        memcpy( item, cb->tail, cb->sz );

        cb->tail = ( uint8_t * ) cb->tail + cb->sz;
        if ( cb->tail == cb->buffer_end )
            cb->tail = cb->buffer;
        cb->count--;
    }
    taskEXIT_CRITICAL();

    return 1;
}