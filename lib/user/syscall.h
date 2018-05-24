#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H
#include "stdint.h"
#include "thread.h"
enum SYSCALL_NR{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
    SYS_FORK
};

uint32_t getpid();
uint32_t write(int32_t fd, const void* buf, uint32_t count);  //简易的write
void* malloc(uint32_t size);
void free(void* ptr);
pid_t fork();
#endif