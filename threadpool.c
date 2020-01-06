#include "threadpool.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>

static queue_list_t *working_pools = NULL;

static sem_t working_pools_mutex;

static void lib_init (void) __attribute__ ((constructor));

static void lib_destroy (void) __attribute__ ((destructor));

void catch(int sig) {
    if (sem_wait(&working_pools_mutex)) {
        exit(-1);
    }
    while (get_queue_size(working_pools) > 0) {
        thread_pool_t *pool = pop_front(working_pools);
        if (sem_post(&working_pools_mutex)) {
            exit(-1);
        }
        thread_pool_destroy(pool);
        if (sem_wait(&working_pools_mutex)) {
            exit(-1);
        }
        printf("sigint -> pool destroyed\n");
        strsignal(sig);
    }
    if (sem_post(&working_pools_mutex)) {
        exit(-1);
    }
}

void lib_init (void) {
    working_pools = queue_init();

    if (sem_init(&working_pools_mutex, 0, 1) != 0) {
        exit(-1);
    }

    struct sigaction action;
    sigset_t block_mask;

    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);

    action.sa_handler = catch;
    action.sa_mask = block_mask;
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, 0) == -1) {
        exit(-1);
    }
}

void lib_destroy (void) {
    queue_destroy(working_pools);
    if (sem_destroy(&working_pools_mutex)) {
        exit(-1);
    }
}

void *worker_function(void *data) {
    assert(data);
    thread_pool_t *pool = (thread_pool_t *)data;
    int32_t err;
    while (true) {
        if((err = sem_wait(&pool->queue_edit_semaphore)) != 0) {
            exit(err);
        }
        while(get_queue_size(pool->tasks) == 0) {
            if((err = sem_post(&pool->queue_edit_semaphore)) != 0) {
                exit(err);
            }
            if((err = sem_wait(&pool->empty_queue)) != 0) {
                exit(err);
            }
            if((err = sem_wait(&pool->queue_edit_semaphore)) != 0) {
                exit(err);
            }
            if(pool->finishing && get_queue_size(pool->tasks) == 0) {
                if((err = sem_post(&pool->queue_edit_semaphore)) != 0) {
                    exit(err);
                }
                break;
            }
        }
        if(pool->finishing && get_queue_size(pool->tasks) == 0) {
            if((err = sem_post(&pool->queue_edit_semaphore)) != 0) {
                exit(err);
            }
            break;
        }
        runnable_t *task = pop_front(pool->tasks);
        if((err = sem_post(&pool->queue_edit_semaphore)) != 0) {
            exit(err);
        }
        (task->function)(task->arg, task->argsz);
        free(task);
    }

    return 0;
}

int thread_pool_init(thread_pool_t *pool, size_t num_threads) {
    int32_t err;

    pool->size = num_threads;
    pool->threads = malloc(num_threads * sizeof(pthread_t));
    if(!pool->threads) {
        thread_pool_destroy(pool);
        return -1;
    }
    if ((err = pthread_attr_init (&pool->attr)) != 0) {
        thread_pool_destroy(pool);
        return err;
    }
    if ((err = pthread_attr_setdetachstate(&pool->attr,PTHREAD_CREATE_JOINABLE)) != 0) {
        thread_pool_destroy(pool);
        return err;
    }
    pool->tasks = queue_init();
    if(!pool->tasks) {
        thread_pool_destroy(pool);
        return -1;
    }
    if((err = sem_init(&pool->queue_edit_semaphore, 0, 1)) != 0) {
        thread_pool_destroy(pool);
        return err;
    }
    if((err = sem_init(&pool->empty_queue, 0, 0)) != 0) {
        thread_pool_destroy(pool);
        return err;
    }
    pool->finishing = false;
    for(size_t i=0; i<num_threads; i++) {
        pool->threads[i] = malloc(sizeof(pthread_t));
        if(!pool->threads[i]) {
            thread_pool_destroy(pool);
            return -1;
        }
        if((err = pthread_create(pool->threads[i], &pool->attr, worker_function, pool)) != 0) {
            thread_pool_destroy(pool);
            return err;
        }
    }

    if((err = sem_wait(&working_pools_mutex)) != 0) {
        thread_pool_destroy(pool);
        return err;
    }
    queue_push(working_pools, pool);
    if((err = sem_post(&working_pools_mutex)) != 0) {
        thread_pool_destroy(pool);
        return err;
    }
    return 0;
}

void thread_pool_destroy(struct thread_pool *pool) {
    if(!pool)
        return;
    int32_t err;
    pool->finishing = true;
    if(pool->threads) {
        for(size_t i = 0; i < pool->size; i++) {
            if((err = sem_post(&pool->empty_queue)) != 0) {
                exit(-1);
            }
        }
        for(size_t i = 0; i < pool->size; i++) {
            if(pool->threads[i]) {
                if(pthread_join(*(pool->threads[i]), NULL) != 0) {
                    exit(-1);
                }
                free(pool->threads[i]);
            }
        }
    }
    if((err = pthread_attr_destroy(&pool->attr)) != 0) {
        exit(err);
    }
    if((err = sem_destroy(&pool->queue_edit_semaphore)) != 0) {
        exit(err);
    }
    if((err = sem_destroy(&pool->empty_queue)) != 0) {
        exit(err);
    }
    queue_destroy(pool->tasks);
    if(pool->threads) {
        free(pool->threads);
    }

    if((err = sem_wait(&working_pools_mutex)) != 0) {
        exit(err);
    }
    queue_delete(working_pools, pool);
    if((err = sem_post(&working_pools_mutex)) != 0) {
        exit(err);
    }
}

int defer(struct thread_pool *pool, runnable_t runnable) {
    if(!pool->finishing) {
        int32_t err;
        if((err = sem_wait(&pool->queue_edit_semaphore)) != 0) {
            return err;
        }
        runnable_t *runnable_copy = malloc(sizeof(runnable_t));
        if(!runnable_copy) {
            return -1;
        }
        runnable_copy->function = runnable.function;
        runnable_copy->arg = runnable.arg;
        runnable_copy->argsz = runnable.argsz;
        queue_push(pool->tasks, runnable_copy);
        if((err = sem_post(&pool->queue_edit_semaphore)) != 0) {
            return err;
        }
        if((err = sem_post(&pool->empty_queue)) != 0) {
            return err;
        }
        return 0;
    }
    return -1;
}
