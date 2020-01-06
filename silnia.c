//
// Created by mkcin on 30.12.2019.
//

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "future.h"
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <string.h>

typedef struct factorial {
    uint64_t result, n;
} factorial_t;

void *next_factorial(void *arg, size_t size, size_t *size_pointer) {
    factorial_t *factorial = (factorial_t *)arg;
    factorial_t *result = malloc(sizeof(factorial_t));
    if(!result) {
        exit(-1);
    }
    result->n = factorial->n + 1;
    result->result = factorial->result * result->n;
    *size_pointer = sizeof(factorial_t);
    free(factorial);
    return result;
}

void destroy_pool_and_futures(size_t n, future_t **futures, thread_pool_t *pool) {
    for(size_t i=0; i < n; i++) {
        if(futures[i]) {
            free(futures[i]);
        }
        else {
            break;
        }
    }
    free(futures);
    thread_pool_destroy(pool);
}

int32_t main() {
    thread_pool_t pool;
    int32_t err;
    if((err = thread_pool_init(&pool, 3)) != 0) {
        return err;
    }
    uint32_t n;
    scanf("%d", &n);
    if(n < 2) {
        printf("1\n");
        thread_pool_destroy(&pool);
        return 0;
    }
    factorial_t *factorial = malloc(sizeof(factorial_t));
    future_t **futures = malloc(n * sizeof(future_t*));
    if(!futures || !factorial) {
        thread_pool_destroy(&pool);
        return -1;
    }
    for(int i=0; i < n; i++) {
        futures[i] = malloc(sizeof(future_t));
        if(!futures[i]) {
            destroy_pool_and_futures(n, futures, &pool);
            return -1;
        }
    }
    callable_t callable;
    callable.function = next_factorial;
    factorial->result = 1;
    factorial->n = 0;
    callable.arg = factorial;
    callable.argsz = sizeof(factorial_t);
    if((err = async(&pool, futures[0], callable)) != 0) {
        destroy_pool_and_futures(n, futures, &pool);
        return err;
    }
    for(size_t i=1; i < n; i++) {
        ;
        if((err = map(&pool, futures[i], futures[i-1], next_factorial)) != 0) {
            destroy_pool_and_futures(n, futures, &pool);
            return err;
        }
        usleep(1000000000);
    }
    factorial_t *result;
    if((result = await(futures[n-1])) == NULL) {
        destroy_pool_and_futures(n, futures, &pool);
        return -1;
    }
    printf("%ld\n", result->result);

    destroy_pool_and_futures(n, futures, &pool);
    free(result);
}
