#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define BUFSIZE 10

struct ioqueue{
    Lock lock;
    struct task_struct* producer;
    struct task_struct* consumer;
    char buf[BUFSIZE];
    int32_t head;
    int32_t tail;
};

void ioqueue_init(struct ioqueue *ioq);
bool ioq_full(struct ioqueue* kbd_buf);
void ioq_putchar(struct ioqueue* , char);
char oq_getchar(struct ioqueue *);
bool ioq_empty(struct ioqueue *);
#endif
