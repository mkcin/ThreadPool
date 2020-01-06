#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stddef.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "generic_queue.h"

typedef struct runnable {
  void (*function)(void *, size_t);
  void *arg;
  size_t argsz;
} runnable_t;

typedef struct thread_pool {
    size_t size;
    queue_list_t *tasks;
    pthread_t **threads;
    pthread_attr_t attr;
    sem_t queue_edit_semaphore;
    sem_t empty_queue;
    bool finishing;
} thread_pool_t;

// Wywołanie thread_pool_init inicjuje argument wskazywany przez pool jako nową pulę, w której będzie funkcjonować
// pool_size wątków obsługujących zgłoszone do wykonania zadania. Za gospodarkę pamięcią wskazywaną przez pool
// odpowiada użytkownik biblioteki. Poprawność działania biblioteki jest gwarantowana tylko, jeśli każda pula
// stworzona przez thread_pool_init jest niszczona przez wywołanie thread_pool_destroy z argumentem reprezentującym
// tę pulę.
int thread_pool_init(thread_pool_t *pool, size_t pool_size);

void thread_pool_destroy(thread_pool_t *pool);

// Wywołanie defer(pool, runnable) zleca puli wątków pool wykonanie zadania opisanego przez argument runnable,
// argumenty function są przekazywane przez wskaźnik args, w polu argsz znajduje się długość dostępnego do pisania
// i czytania buforu znajdującego się pod tym wskaźnikiem. Za zarządzanie pamięcią wskazywaną przez args odpowiada
// klient biblioteki.
int defer(thread_pool_t *pool, runnable_t runnable);

// Usuwa element na który wskazuje wskacnik data. Jeśli element nie istnieje w kolejce, nie robi nic
void queue_delete(queue_list_t *q, void *data);

#endif
