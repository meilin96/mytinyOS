#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"

void k_thread_a(void*);

void main(void) {
    put_str("I am kernel\n");
    init_all();
    
    thread_start("k_thread_a", 31, k_thread_a, "argA");

    while(1);
}

void k_thread_a(void* arg){
    char* para = arg;
    put_str(para);
}
