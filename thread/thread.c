#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "memory.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "interrupt.h"
#define PG_SIZE 4096

struct task_struct* main_thread;    //主线程pcb
struct list thread_ready_list;      //就绪队列
struct list thread_all_list;        //所有task队列
struct list_elem* thread_tag;       //队列中的节点都是PCB中的tag，需要完成从tag-->PCB的转换（定义这个变量非必要

extern void switch_to(struct task_struct* cur, struct task_struct* next);

struct task_struct* running_thread(){
    uint32_t esp;
    asm("mov %%esp, %0":"=g"(esp));
    //各线程的栈都在pcb中，而一个pcb占一个页，取pcb的高20位就是指向这个pcb的指针
    return (struct task_struct*)(esp & 0xfffff000);
}

static void kernel_thread(thread_func* function, void* func_arg){
    intr_enable();
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
    if(pcb == main_thread){
        pcb->status = TASK_RUNNING;
    }else{
        pcb->status = TASK_READY;
    }
    pcb->priority = prio;
    pcb->ticks = prio;
    pcb->elapsed_ticks = 0;
    pcb->pgdir = NULL;  //how about process? to be continued
    pcb->self_kstack = (uint32_t*)((uint32_t)pcb + PG_SIZE);
    pcb->stack_magic = STACK_MAGIC;
}

//创建一个优先级为prio的线程，线程名字为tname
struct task_struct* thread_start(char* tname, int prio, thread_func function, void* func_arg){
    struct task_struct* pcb = get_kernel_pages(1);
    init_thread(pcb, tname, prio);
    init_thread_stack(pcb, function, func_arg);

    //加入队列
    ASSERT(!elem_find(&thread_ready_list, &pcb->general_tag));
    list_push_back(&thread_ready_list, &pcb->general_tag);
    ASSERT(!elem_find(&thread_all_list, &pcb->general_tag));
    list_push_back(&thread_all_list, &pcb->general_tag);
    
    //asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" : : "g" (pcb->self_kstack) : "memory");
    asm volatile ("movl %0, %%esp;\
                   pop %%ebp; \
                   pop %%ebx;\
                   pop %%edi; \
                   pop %%esi;\
                   ret":: "g"(pcb->self_kstack) : "memory");
    return pcb;
}

//将kernel的中main函数完善为主线程
static void make_main_thread(){
   main_thread = running_thread(); 
   init_thread(main_thread, "main", 31);

   ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
   list_push_back(&thread_all_list, &main_thread->all_list_tag);
}

void schedule(){
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct* cur = running_thread();
    if(cur->status == TASK_RUNNING){
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_push_back(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }else{
        //cur->status == Blocked to be continued
    }

    ASSERT(!list_empty(&thread_ready_list));
    
    ListElem* next_thread_tag = list_pop_front(&thread_list_ready);
    struct task_struct* next_thread = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(cur, next);
    
}
