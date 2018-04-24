#include "sync.h"
#include "list.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"

void sema_init(struct semaphore* sema, uint8_t value){
    sema->value = value;
    list_init(&sema->waiters);
}

void lock_init(Lock* lock){
    lock->holder = NULL;
    lock->holder_repeat_nr = 0;
    sema_init(&lock->semaphore, 1);
}

void sema_down(struct semaphore* sema){
    enum intr_status old_status = intr_disable();
    while(sema->value == 0){
        ASSERT(!elem_find(&sema->waiters, &running_thread()->general_tag));
        if(elem_find(&sema->waiters, &running_thread()->general_tag)){
            PANIC("sema_down: thread blocked has been in waiters_list");
        }
        list_push_back(&sema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);
    }
    sema->value--;
    ASSERT(sema->value == 0);
    intr_set_status(old_status);
}

void sema_up(struct semaphore* sema){
    enum intr_status old_status = intr_disable();
    ASSERT(sema->value == 0);
    if(!list_empty(&sema->waiters)){
        struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop_front(&sema->waiters));
        thread_unblock(thread_blocked);
    }
    sema->value++;
    ASSERT(sema->value == 1);
    intr_set_status(old_status);
}

void lock_acquire(Lock* lock){
    if(lock->holder != running_thread()){
        sema_down(&lock->semaphore);
        lock->holder = running_thread();
        ASSERT(lock->holder_repeat_nr == 0);
        lock->holder_repeat_nr = 1;
    }else{
        lock->holder_repeat_nr++;
    }
}

void lock_release(Lock* lock){
    ASSERT(lock->holder == running_thread());
    if(lock->holder_repeat_nr > 1){
        lock->holder_repeat_nr--;
        return;
    }
    ASSERT(lock->holder_repeat_nr == 1);
    lock->holder = NULL;
    lock->holder_repeat_nr = 0;
    sema_up(&lock->semaphore);
}
