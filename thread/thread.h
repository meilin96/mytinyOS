#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "memory.h"
#include "fs.h"
#define STACK_MAGIC 0x19960521 
#define DEFAULT_PRIO 31
typedef void thread_func(void*);
typedef int16_t pid_t;
enum task_status{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

//中断栈，用于中断发生时保护上下文环境
struct intr_stack{
    uint32_t vec_no;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t err_code;
    void (*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
};

//线程自己的栈，用于存储线程中待执行的函数
struct thread_stack{
    //被调用者(switch-to())保存
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    //线程第一次执行时，eip指向待调用的函数，to be continued
    void (*eip)(thread_func* func, void* func_arg);

    void (*unused_retaddr);//占位
    thread_func* function;
    void* func_arg;
};

//PCB 在内存中占用一页
struct task_struct{
    uint32_t* self_kstack;   //内核栈
    pid_t pid;
    enum task_status status;
    uint8_t priority;        //优先级
    uint8_t ticks;           //每次在cpu上执行的时间片
    uint32_t elapsed_ticks;  //在cpu上执行的总时间片
    //ready or blocked or ...
    int32_t fd_table[MAX_FILES_OPEN_PER_PROC];
    ListElem general_tag;
    //用于线程队列thread_all_list
    ListElem all_list_tag;
    uint32_t* pgdir;         //进程页表的虚拟地址，线程为NULL
    struct virtual_addr userprog_vaddr; //进程的虚拟地址池
    struct mem_block_desc u_block_desc[DESC_CNT];
    char name[16];
    uint32_t stack_magic;    //栈边界标记，用于检测栈溢出
};

extern List thread_all_list;
extern List thread_ready_list;

void init_thread_stack(struct task_struct* pcb, thread_func func, void* func_arg);
void init_thread(struct task_struct* pcb, char* name, int prio);
struct task_struct* thread_start(char* tname, int prio, thread_func func, void* func_arg);
struct task_struct* running_thread();
void schedule();
void thread_init();
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* thread);
void thread_yield();
#endif
