#ifndef MEMORY_MANAGER_QUEUE_H
#define MEMORY_MANAGER_QUEUE_H

#include <inttypes.h>
#include <malloc.h>
#include <memory.h>

#define BUFFER_QUEUE_CAPACITY 25
#define INT_FLOAT_QUEUE_CAPACITY    10
typedef struct
{
    int8_t type;
    uint8_t data [256];
} buffer_item;

typedef struct
{
    uint8_t buffer [sizeof (buffer_item) * BUFFER_QUEUE_CAPACITY ]; // data buffer
    uint8_t *buffer_end;                                            // end of data buffer
    size_t capacity;                                                // maximum number of items in the buffer
    size_t count;                                                   // number of items in the buffer
    size_t sz;                                                      // size of each item in the buffer
    uint8_t *head;                                                  // pointer to head
    uint8_t *tail;                                                  // pointer to tail
} buffer_queue;


void buffer_queue_init     ( buffer_queue      *queue );
int buffer_queue_push_back ( buffer_queue  *cb, buffer_item *item );
int buffer_queue_pop_front ( buffer_queue  *cb, buffer_item *item );


#endif //MEMORY_MANAGER_QUEUE_H
