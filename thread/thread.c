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
//队列中的节点都是PCB中的tag，需要完成从tag-->PCB的转换(elem2entry（定义这个变量非必要,可以用局部变量代替
struct list_elem *thread_tag;

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
    list_push_back(&thread_all_list, &pcb->all_list_tag);
    
    return pcb;
}

//将kernel的中main函数完善为主线程
static void make_main_thread(){
   main_thread = running_thread(); 
   init_thread(main_thread, "main", 31);

   ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
   list_push_back(&thread_all_list, &main_thread->all_list_tag);
}

void schedule() {

   ASSERT(intr_get_status() == INTR_OFF);

   struct task_struct* cur = running_thread(); 
   if (cur->status == TASK_RUNNING) { // 若此线程只是cpu时间片到了,将其加入到就绪队列尾
      ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
      list_push_back(&thread_ready_list, &cur->general_tag);
      cur->ticks = cur->priority;     // 重新将当前线程的ticks再重置为其priority;
      cur->status = TASK_READY;
   } else { 
      /* 若此线程需要某事件发生后才能继续上cpu运行,
      不需要将其加入队列,因为当前线程不在就绪队列中。*/
   }

   ASSERT(!list_empty(&thread_ready_list));
   thread_tag = NULL;	  // thread_tag清空
/* 将thread_ready_list队列中的第一个就绪线程弹出,准备将其调度上cpu. */
   thread_tag = list_pop_front(&thread_ready_list);   
   struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
   next->status = TASK_RUNNING;
   switch_to(cur, next);
}

void thread_init(){
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    make_main_thread();
    put_str("thread_init done\n");
}

//当前线程将自己阻塞，并标记状态为stat
void thread_block(enum task_status stat){
    ASSERT((stat == TASK_BLOCKED) || (stat == TASK_HANGING) || (stat == TASK_WAITING));
    enum intr_status old_intr_status = intr_disable();
    struct task_struct* cur_thread = running_thread();
    cur_thread->status = stat;
    schedule();
    //线程阻塞被解除后才运行下面的
    intr_set_status(old_intr_status);
}
//解除线程thread的阻塞
void thread_unblock(struct task_struct* thread){
    enum intr_status intr_old_status = intr_disable();
    enum task_status task_old_status = thread->status;
    ASSERT((task_old_status == TASK_BLOCKED) || (task_old_status == TASK_WAITING) || (task_old_status == TASK_BLOCKED)); 
    if(task_old_status != TASK_READY){
        if(elem_find(&thread_ready_list, &thread->general_tag)){
            PANIC("thread_unblock:blocked thread in ready list!");
        }
        list_push_back(&thread_ready_list, &thread->general_tag);
        thread->status = TASK_READY;
    }
    intr_set_status(intr_old_status);
}
