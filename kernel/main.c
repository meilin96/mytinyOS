#include "console.h"
#include "debug.h"
#include "init.h"
#include "interrupt.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "memory.h"
#include "print.h"
#include "process.h"
#include "thread.h"
void k_thread_a(void *);
void k_thread_b(void *);
void u_prog_a();
void u_prog_b();

int var_a = 0, var_b = 0;

int main(void) {
    put_str("I am kernel\n");
    init_all();

    thread_start("consumer_a", 31, k_thread_a, "argA ");
    thread_start("consumer_b", 31, k_thread_b, "argB ");
    process_execute(u_prog_a, "user_prog_a");
    process_execute(u_prog_b, "user_prog_b");
    intr_enable();

    while (1)
        ;
        //    console_put_str("Main ");

    return 0;
}

void k_thread_a(void *arg) {
    char *p = arg;
    while (1) {
        console_put_str("v_a:0x");
        console_put_int(var_a);
    }
}

void k_thread_b(void *arg) {
    char *p = arg;
    while (1) {
        console_put_str("v_b:0x");
        console_put_int(var_b);
    }
}

void u_prog_a() {
    while (1) {
        var_a++;
    }
}
void u_prog_b() {
    while (1) {
        var_b++;
    }
}