#include "syscall.h"
#include "stdint.h"
#define _syscall0(NUMBER)                                                      \
    ({                                                                         \
        int res;                                                               \
        asm volatile("int $0x80" : "=a"(res) : "a"(NUMBER) : "memory");        \
        res;                                                                   \
    })

#define _syscall3(NUMBER, ARG1, ARG2, ARG3)                                    \
    ({                                                                         \
        int res;                                                               \
        asm volatile("int $0x80"                                               \
                     : "=a"(res)                                               \
                     : "a"(NUMBER), "b"(ARG1), "c"(ARG2), "d"(ARG3)            \
                     : "memory");                                              \
        res;                                                                   \
    })

#define _syscall1(NUMBER, ARG1)                                                \
    ({                                                                         \
        int res;                                                               \
        asm volatile("int $0x80"                                               \
                     : "=a"(res)                                               \
                     : "a"(NUMBER), "b"(ARG1)                                  \
                     : "memory");                                              \
        res;                                                                   \
    })

uint32_t getpid() { return _syscall0(SYS_GETPID); }

uint32_t write(int32_t fd, const void* buf, uint32_t count) { return _syscall3(SYS_WRITE, fd, buf, count); }

void *malloc(uint32_t size) { return (void *)_syscall1(SYS_MALLOC, size); }

void free(void *ptr) { _syscall1(SYS_FREE, ptr); }