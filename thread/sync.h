#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "list.h"
#include "stdint.h"
#include "thread.h"

struct semaphore{
    uint8_t value;
    List waiters;
};

struct lock{
    struct task_struct* holder;
    struct semaphore semaphore;
    uint32_t holder_repeat_nr;
};

typedef struct lock Lock;

void lock_init(Lock* lock);
void lock_acquire(Lock* lock);
void lock_release(Lock* lock);
#endif
