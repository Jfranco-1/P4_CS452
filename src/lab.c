#include "lab.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Struct that represents a queue
 */

struct queue {
    void **buffer;           // Array to store data pointers
    int capacity;            // Maximum capacity of the queue
    int size;                // Current number of elements
    int front;               // Index of the front element
    int rear;                // Index of the rear element
    bool is_shutdown_flag;   // Flag to indicate if queue is shut down
    pthread_mutex_t lock;    // Mutex
    pthread_cond_t not_full; // Condition variable for when queue is not full
    pthread_cond_t not_empty; // Condition variable for when queue is not empty
};

queue_t queue_init(int capacity) {
    queue_t q = (queue_t)malloc(sizeof(struct queue));
    if (!q) {
        return NULL;
    }
    
    q->buffer = (void **)malloc(capacity * sizeof(void *));
    if (!q->buffer) {
        free(q);
        return NULL;
    }
    
    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = 0;
    q->is_shutdown_flag = false;
    
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    
    return q;
}

void queue_destroy(queue_t q) {
    if (!q) {
        return;
    }
    
    pthread_mutex_lock(&q->lock);
    q->is_shutdown_flag = true;
    
    // Signal all waiting threads to wake up
    pthread_cond_broadcast(&q->not_full);
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
    
    // Clean up resources
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
    
    free(q->buffer);
    free(q);
}

void enqueue(queue_t q, void *data) {
    if (!q) {
        return;
    }
    
    pthread_mutex_lock(&q->lock);
    
    // Wait until the queue is not full or shutdown
    while (q->size == q->capacity && !q->is_shutdown_flag) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    
    // If queue is shutdown, don't add more items
    if (q->is_shutdown_flag) {
        pthread_mutex_unlock(&q->lock);
        return;
    }
    
    // Add item to the queue
    q->buffer[q->rear] = data;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
    
    // Signal that the queue is not empty anymore
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

void *dequeue(queue_t q) {
    if (!q) {
        return NULL;
    }
    
    pthread_mutex_lock(&q->lock);
    
    // Wait until the queue is not empty
    while (q->size == 0 && !q->is_shutdown_flag) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    
    // Return NULL if queue is empty
    if (q->size == 0) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }
    
    // Remove item from the queue
    void *data = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    
    // Signal that the queue is not full anymore
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    
    return data;
}

void queue_shutdown(queue_t q) {
    if (!q) {
        return;
    }
    
    pthread_mutex_lock(&q->lock);
    q->is_shutdown_flag = true;
    
    // Signal to check the shutdown flag
    pthread_cond_broadcast(&q->not_full);
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

bool is_empty(queue_t q) {
    if (!q) {
        return true;
    }
    
    pthread_mutex_lock(&q->lock);
    bool empty = (q->size == 0);
    pthread_mutex_unlock(&q->lock);
    
    return empty;
}

bool is_shutdown(queue_t q) {
    if (!q) {
        return true;
    }
    
    pthread_mutex_lock(&q->lock);
    bool shutdown = q->is_shutdown_flag;
    pthread_mutex_unlock(&q->lock);
    
    return shutdown;
}