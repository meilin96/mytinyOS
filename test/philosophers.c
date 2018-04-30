#include "stdint.h"
#include "sync.h"
#include "thread.h"
#include "console.h"
#include "interrupt.h"
#include "init.h"

#define N 5
#define THINKING 0
#define HUNGRY 1
#define EATING 2
#define LEFT (i-1+N)%N
#define RIGHT (i+1)%N
int32_t cnt[N];
struct semaphore sema_phr[N];
int32_t status[N];
int32_t idx[N];
Lock lock;
// Lock check_lock;
void take_forks(int32_t);
void put_forks(int32_t);
void test(int32_t);
void think(int32_t);
void eat(int32_t);
void check();
bool is_all_starve();
bool is_finished();
void philosophers(void *arg);

int main()
{
    int32_t i = 0;
    for(;i < N;i++){
        sema_init(&sema_phr[i], 0);
        idx[i] = i;
        status[i] = THINKING;
    }
    init_all();
    lock_init(&lock);
    intr_disable();
   for(int i = 0;i < N; i++){
        thread_start("philosophers", 31, philosophers, (void*)(idx+i));
        // thread_start("philosophers_a", 31, philosophers, "Phor_A ");
        // thread_start("philosophers_b", 31, philosophers, "Phor_B ");
    }

    thread_start("check_thread", 31, check, "Check");
    intr_enable();
    while (1){
        // console_put_str("Main ");
    }    
    return 0;
}

void philosophers(void* arg){
    int i = *((int32_t*) arg);
    while(1){
        think(i);
        take_forks(i);
        eat(i);
        put_forks(i);
        // console_put_str(arg);console_put_str(" ");
    }
}

// lock_acquire(Lock* lock);
// void lock_release

void take_forks(int32_t i){
    lock_acquire(&lock);
    status[i] = HUNGRY;
    test(i);
    lock_release(&lock);
    sema_down(&sema_phr[i]); 
}

void put_forks(int32_t i){
    lock_acquire(&lock);
    status[i] = THINKING;
    test(LEFT);
    test(RIGHT);
    lock_release(&lock);
}

void test(int32_t i){
    if(status[i] == HUNGRY && status[LEFT] != EATING && status[RIGHT] != EATING){
        status[i] = EATING;
        sema_up(&sema_phr[i]);
    }
}

void think(int32_t i){
    if(i + 1 == 2){
    }else{
    }
}

void eat(int32_t i){
    console_put_int(i);
    console_put_str("_eat  ");
    cnt[i]++;
}

void check(void* arg){
    while(1){
        intr_disable();
        if (is_all_starve() || is_finished())
        {
            int i;
            console_put_str("\nStatus: ");
            for (i = 0; i < N; i++){
                console_put_int(status[i]);
                console_put_char(' ');
            }
            console_put_str("\nCnt: ");
            for (i = 0; i < N; i++){
                console_put_int(cnt[i]);
                console_put_char(' ');
            }
            while (1)
                ;
        }
        intr_enable();
    }
    
}

bool is_all_starve(){
    bool res = true;
    int i = 0;
    for(;i < N; i++){
        if(status[i] != HUNGRY){
            res = false;
        } 
    }
    return res;
}
bool is_finished(){
    bool res = true;
    int i = 0;
    for (; i < N; i++)
    {
        if (cnt[i] < 10)
        {
            res = false;
        }
    }
    return res;
}