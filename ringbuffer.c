#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define THREAD_NUM 20
#define ITER_NUMS 50000
int UNLOCKED=0;
int LOCKED=1;
typedef struct data {
    int open;
    int high;
    int low;
    int close;
} Data;
typedef struct ringbuffer {
    int lock;
    int idx;
    int size;
    Data *data;
} RingBuffer;

RingBuffer *CreateRingBuffer(int size) {
    size_t cap = sizeof(Data) * size + offsetof(RingBuffer, data);
    RingBuffer *buffer = (RingBuffer *) malloc(cap);
    memset(buffer, 0, cap);
    buffer->size = size - 1;
    buffer->data = (Data *) ((void *) buffer + sizeof(RingBuffer));
    return buffer;
}

void lock(RingBuffer *buffer) {
    int target = 0;
    int time = 1;
    while (1) {
        if (__atomic_compare_exchange_n(&buffer->lock, &UNLOCKED, 1, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            return;
        }
        printf("thread %d: try to get locker %d times \n", pthread_self(), time);
        time++;
    };
}

void unlock(RingBuffer *buffer) {
    int target = 1;
    while (1) {
        if (__atomic_compare_exchange_n(&buffer->lock, &LOCKED, 0, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            return;
        }

    };
}

Data *get(RingBuffer *buffer) {
    lock(buffer);
    Data *data = buffer->data + buffer->idx;
    buffer->idx = (buffer->idx + 1) & buffer->size;
    unlock(buffer);
    return data;
};

void *test(void *buffer) {
    Data *data;
    int i;
    for (; i < ITER_NUMS; i++) {
        data = get((RingBuffer *) buffer);
        data->open += 1;
    }
};


int main() {
    pthread_t thread[THREAD_NUM];
    RingBuffer *buffer = CreateRingBuffer(512);
    for (int j = 0; j < THREAD_NUM; j++) {
        pthread_create(&(thread[j]), NULL, &test, buffer);
    }
    for (int t = 0; t < THREAD_NUM; t++) {
        pthread_join(thread[t], NULL);
    };
    int total;
    for (int i = 0; i <= buffer->size; i++) {
        total += (buffer->data + i)->open;
        printf("%d ", (buffer->data + i)->open);
    }
    printf("\n expected: %d result:%d ", ITER_NUMS * THREAD_NUM, total);
    return 0;
};
