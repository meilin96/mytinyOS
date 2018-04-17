#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "memory.h"
#include "global.h"
#include "debug.h"
#define PG_SIZE 4096

static void kernel_thread(thread_func* function, void* func_arg){
    function(func_arg);
}

//初始化线程栈,把待执行的函数和参数放到栈中对应的位置
void init_thread_stack(struct task_struct* pcb, thread_func function, void* func_arg){
    //预留出中断栈的空间
    pcb->self_kstack -= sizeof(struct intr_stack);
    //预留出线程栈的空间
    pcb->self_kstack -= sizeof(struct thread_stack);
    //WTF?一个内核栈一个线程栈？
    struct thread_stack* kthread_stack = (struct thread_stack*)pcb->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;

}

//初始化线程的基本信息
void init_thread(struct task_struct* pcb, char* tname, int prio){
    memset(pcb, 0, sizeof(*pcb));
    strcpy(pcb->name, tname);
    pcb->status = TASK_RUNNING;     //to be continued
    pcb->priority = prio;
    pcb->self_kstack = (uint32_t*)((uint32_t)pcb + PG_SIZE);
    pcb->stack_magic = STACK_MAGIC;
}

//创建一个优先级为prio的线程，线程名字为tname
struct task_struct* thread_start(char* tname, int prio, thread_func function, void* func_arg){
    struct task_struct* pcb = get_kernel_pages(1);
    init_thread(pcb, tname, prio);
    init_thread_stack(pcb, function, func_arg);

    //asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" : : "g" (pcb->self_kstack) : "memory");
    asm volatile ("movl %0, %%esp;\
                   pop %%ebp; \
                   pop %%ebx;\
                   pop %%edi; \
                   pop %%esi;\
                   ret":: "g"(pcb->self_kstack) : "memory");
    return pcb;
}
