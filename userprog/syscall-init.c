#include "syscall-init.h"
#include "print.h"
#include "thread.h"
#include "stdint.h"
#include "syscall.h"

#define syscall_nr 32 //最大支持系统调用子功能数
typedef void* syscall;
syscall syscall_table[syscall_nr];

uint32_t sys_getpid(void){
    return running_thread()->pid;
}

void syscall_init(){
    put_str("syscall_init start\n");
    syscall_table[SYS_GETPID] = sys_getpid;
    put_str("syscalll_init done\n");
}
