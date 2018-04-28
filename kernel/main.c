#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
void k_thread_a(void*);

int main(void) {
    put_str("I am kernel\n");
    init_all();
    
    thread_start("consumer_a", 31, k_thread_a, "A ");
    thread_start("consumer_b", 31, k_thread_a, "B ");
    intr_enable();

    while(1);
//        console_put_str("Main ");
   
   return 0;
}

void k_thread_a(void* arg){
    while(1){
        intr_disable();
        if(!ioq_empty(&kbd_buf)){
            console_put_str(arg);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
        }
        intr_enable();
    }
}
