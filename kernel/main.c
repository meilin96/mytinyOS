#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"
#include "interrupt.h"

void k_thread_a(void*);

void main(void) {
    put_str("I am kernel\n");
    init_all();
    
    thread_start("k_thread_a", 31, k_thread_a, "A ");
    thread_start("k_thread_a", 31, k_thread_a, "B ");
    intr_enable();

    while(1)
        put_str("Main ");
    
}

void k_thread_a(void* arg){
    char* para = arg;
    while(1)
        put_str(para);put_str("\n");
}
