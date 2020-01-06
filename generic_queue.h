//
// Created by mkcin on 28.12.2019.
//

#ifndef ASYNC_GENERIC_QUEUE_H
#define ASYNC_GENERIC_QUEUE_H

#include <stddef.h>

typedef struct node
{
    void *data;
    struct node *next;
} node_t;

typedef struct queue_list
{
    int size_of_queue;
    node_t *head;
    node_t *tail;
} queue_list_t;

queue_list_t *queue_init();

int queue_push(queue_list_t *q, void *data);

void *pop_front(queue_list_t *q);

void queue_clear(queue_list_t *q);

void queue_destroy(queue_list_t *q);

int get_queue_size(queue_list_t *q);

#endif //ASYNC_GENERIC_QUEUE_H
