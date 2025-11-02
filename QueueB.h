//
// Created by jamestbest on 10/20/25.
//

#ifndef ANURA_QUEUEB_H
#define ANURA_QUEUEB_H

#include <stddef.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define QUEUE_B_MIN_SIZE 10

typedef struct QueueB {
    void** data_arr;

    size_t pos;
    size_t max_size;

    pthread_mutex_t lock;
    sem_t sem;
} QueueB;

typedef struct QueueBRes {
    bool succ;
    void* data;
} QueueBRes;

typedef struct QueueBAll {
    void** data;
    size_t len;
} QueueBAll;

extern const QueueB QUEUEB_ERR;
extern const QueueBRes QUEUEBRES_ERR;

QueueB queueb_create();
void queueb_destroy(QueueB* queue);

bool queueb_resize(QueueB* queue, size_t new_size);

void queueb_push_blocking(QueueB* queue, void* data);
void* queueb_pop_blocking(QueueB* queue);
QueueBAll queueb_pop_all(QueueB* queue);

QueueBRes queueb_pop(QueueB* queue);

size_t queueb_len(QueueB* queue);
bool queueb_empty(QueueB* queue);

#endif //ANURA_QUEUEB_H
