#include "future.h"
#include <stdlib.h>

typedef void *(*function_t)(void *);

struct data {
    callable_t *callable;
    future_t *future;
};

void function(void *arg, size_t size) {
    future_t *future = (future_t *)arg;
    future->result = future->callable.function(future->callable.arg, future->callable.argsz, &future->result_size);
    sem_post(&future->result_ready);
}

void map_function(void *arg, size_t size) {
    int32_t err;
    future_t *future = (future_t *)arg;
    future_t *from = (future_t *)(future->callable.arg);
    if((err = sem_wait(&from->result_ready)) != 0) {
        exit(-1);
    }
    future->result = future->callable.function(from->result, future->callable.argsz, &future->result_size);
    sem_post(&future->result_ready);
}

int async(thread_pool_t *pool, future_t *future, callable_t callable) {
    int32_t err;
    if((err = sem_init(&future->result_ready, 0, 0)) != 0) {
        return err;
    }
    future->callable = callable;
    runnable_t runnable;
    runnable.arg = future;
    runnable.argsz = sizeof(future_t);
    runnable.function = function;
    if((err = defer(pool, runnable)) != 0) {
        return err;
    }
    return 0;
}

int map(thread_pool_t *pool, future_t *future, future_t *from,
        void *(*function)(void *, size_t, size_t *)) {
    int32_t err;
    if((err = sem_init(&future->result_ready, 0, 0)) != 0) {
        return err;
    }
    callable_t callable;
    callable.function = function;
    callable.arg = from;
    callable.argsz = sizeof(future_t);
    future->callable = callable;
    runnable_t runnable;
    runnable.arg = future;
    runnable.argsz = sizeof(future_t);
    runnable.function = map_function;
    if((err = defer(pool, runnable)) != 0) {
        return err;
    }
    return 0;
}

void *await(future_t *future) {
    int32_t err;
    if((err = sem_wait(&future->result_ready)) != 0) {
        NULL;
    }
    return future->result;
}
