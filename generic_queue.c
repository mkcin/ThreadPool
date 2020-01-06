//
// Created by mkcin on 28.12.2019.
//

//#include "generic_queue.h"
#include <stdlib.h>
#include <string.h>
#include "generic_queue.h"

#include <stdio.h>

queue_list_t *queue_init() {
    queue_list_t *q = malloc(sizeof(queue_list_t));
    if (!q) return NULL;
    q->size_of_queue = 0;
    q->head = q->tail = NULL;
    return q;
}

int queue_push(queue_list_t *q, void *data) {
    node_t *new_node = malloc(sizeof(node_t));

    if (new_node == NULL) {
        return -1;
    }

    new_node->data = data;

    new_node->next = NULL;

    if (q->size_of_queue == 0) {
        q->head = q->tail = new_node;
    } else {
        q->tail->next = new_node;
        q->tail = new_node;
    }

    q->size_of_queue++;
    return 0;
}

void *pop_front(queue_list_t *q) {
    if (q->size_of_queue > 0) {
        node_t *temp = q->head;

        if (q->size_of_queue > 1) {
            q->head = q->head->next;
        } else {
            q->head = NULL;
            q->tail = NULL;
        }

        q->size_of_queue--;
        void *data = temp->data;

        free(temp);

        return data;
    }
    return NULL;
}


void queue_clear(queue_list_t *q) {
    node_t *temp;

    while (q->size_of_queue > 0) {
        temp = q->head;
        q->head = temp->next;
        free(temp);
        q->size_of_queue--;
    }

    q->head = q->tail = NULL;
}

void queue_destroy(queue_list_t *q) {
    if(q) {
        queue_clear(q);
        free(q);
    }
}

int get_queue_size(queue_list_t *q) {
    return q->size_of_queue;
}

void queue_delete(queue_list_t *q, void *data) {
    if(get_queue_size(q) == 0) {
        return;
    }
    if(data == q->head->data) {
        pop_front(q);
        return;
    }
    node_t *before = q->head;
    node_t *node = before->next;
    while(node) {
        if(node->data == data) {
            before->next = node->next;
            q->size_of_queue--;
            if(q->tail == node) {
                q->tail = before;
            }
            free(node);
            return;
        }
        before = node;
        node = node->next;
    }
}