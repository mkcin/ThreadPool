#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include "threadpool.h"

typedef struct data {
    int64_t *row_sum;
    sem_t *row_semaphore;
    int64_t value;
    uint64_t sleep_time;
} data_t;

void add_to_sum(void *arg, size_t size) {
    data_t *data = (data_t *)arg;
    if(!data) {
        exit(-1);
    }
    int32_t err;
    usleep(1000 * data->sleep_time);
    if((err = sem_wait(data->row_semaphore)) != 0) {
        exit(err);
    }
    *(data->row_sum) += data->value;
    if((err = sem_post(data->row_semaphore)) != 0) {
        exit(err);
    }
    free(data);
}

void destroy_result_and_semaphores(size_t n, int64_t *result, sem_t *semaphores) {
    int32_t err;
    if(semaphores) {
        for(size_t i=0; i<n; i++) {
            if((err = sem_destroy(&semaphores[i])) != 0) {
                exit(err);
            }
        }
        free(semaphores);
    }
    if(result) {
        free(result);
    }
}

int main() {
    uint32_t k, n;
    int32_t err;
    scanf("%d %d", &k, &n);
    int64_t *result = malloc(k * sizeof(int64_t));
    if(!result) {
        return -1;
    }
    sem_t *sum_semaphores = malloc(k * sizeof(sem_t));
    if(!sum_semaphores) {
        destroy_result_and_semaphores(k, result, sum_semaphores);
    }
    for(size_t i = 0; i < k; i++) {
        result[i] = 0;
        if((err = sem_init(&sum_semaphores[i], 0, 1)) != 0) {
            destroy_result_and_semaphores(k, result, sum_semaphores);
            return err;
        }
    }
    thread_pool_t pool;
    if((err = thread_pool_init(&pool, 4)) != 0) {
        destroy_result_and_semaphores(n, result, sum_semaphores);
        return err;
    }
    for(uint32_t i = 0; i < k * n; i++) {
        int64_t value;
        uint64_t time;
        scanf("%ld %d", &value, &time);
        data_t *data = malloc(sizeof(data_t));
        if(!data) {
            thread_pool_destroy(&pool);
            destroy_result_and_semaphores(n, result, sum_semaphores);
            return -1;
        }
        data->value = value;
        data->sleep_time = time;
        data->row_sum = &result[i/n];
        data->row_semaphore = &sum_semaphores[i/n];
        runnable_t runnable;
        runnable.arg = data;
        runnable.function = add_to_sum;
        runnable.argsz = sizeof(data_t);
        if((err = defer(&pool, runnable)) != 0) {
            thread_pool_destroy(&pool);
            destroy_result_and_semaphores(n, result, sum_semaphores);
            return err;
        }
    }
    thread_pool_destroy(&pool);
    for(size_t i=0; i < k; i++) {
        printf("%ld\n", result[i]);
    }
    for(size_t i=0; i < k; i++) {
        if((err = sem_destroy(&sum_semaphores[i])) != 0) {
            return -1;
        }
    }
    free(result);
    free(sum_semaphores);
    return 0;
}