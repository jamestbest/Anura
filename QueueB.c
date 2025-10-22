//
// Created by jamestbest on 10/20/25.
//

#include <malloc.h>
#include <assert.h>
#include "QueueB.h"

const QueueB QUEUEB_ERR= {
    .pos= -1,
    .max_size= -1,
    .data_arr= NULL
};

const QueueBRes QUEUEBRES_ERR= {
    .data= NULL,
    .succ= false
};

QueueB queueb_create() {
    QueueB q= (QueueB) {
        .data_arr= malloc(sizeof (void*) * QUEUE_B_MIN_SIZE),
        .max_size= QUEUE_B_MIN_SIZE,
        .pos= 0
    };

    pthread_mutex_init(&q.lock, NULL);
    sem_init(&q.sem, 0, 0);

    return q;
}

void queueb_destroy(QueueB* queue) {
    pthread_mutex_destroy(&queue->lock);
    sem_destroy(&queue->sem);

    *queue= QUEUEB_ERR;
}

void queueb_push_blocking(QueueB* queue, void* data) {
    pthread_mutex_lock(&queue->lock);

    if (queue->pos == queue->max_size) {
        if (!queueb_resize(queue, queue->pos + QUEUE_B_MIN_SIZE)) assert(false);
    }

    queue->data_arr[queue->pos++]= data;

    sem_post(&queue->sem);
    pthread_mutex_unlock(&queue->lock);
}

void* queueb_pop_blocking(QueueB* queue) {
    sem_wait(&queue->sem);

    pthread_mutex_lock(&queue->lock);

    void* res= queue->data_arr[--queue->pos];

    pthread_mutex_unlock(&queue->lock);

    return res;
}

QueueBAll queueb_pop_all(QueueB* queue) {
    pthread_mutex_unlock(&queue->lock);

    const QueueBAll res= (QueueBAll){
        .data= queue->data_arr,
        .len= queue->pos
    };

    *queue= queueb_create();

    return res;
}

QueueBRes queueb_pop(QueueB* queue) {
    pthread_mutex_unlock(&queue->lock);

    if (queue->pos == 0) {
        pthread_mutex_unlock(&queue->lock);
        return QUEUEBRES_ERR;
    }

    // consume a sem, this should be instant return as we know it's not empty
    sem_wait(&queue->sem);
    void* res= queue->data_arr[--queue->pos];

    pthread_mutex_unlock(&queue->lock);
    return (QueueBRes) {
        .data= res,
        .succ= true
    };
}

size_t queueb_len(QueueB* queue) {
    return queue->pos;
}

bool queueb_empty(QueueB* queue) {
    return queue->pos == 0;
}
