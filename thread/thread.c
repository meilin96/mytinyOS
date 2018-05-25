#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "memory.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "interrupt.h"
#include "process.h"
#include "sync.h"
#include "file.h"
#include "stdio.h"
#define PG_SIZE 4096

struct task_struct* main_thread;    //主线程pcb
struct list thread_ready_list;      //就绪队列
struct list thread_all_list;        //所有task队列
struct task_struct* idle_thread;
static void idle(void *arg);
    //队列中的节点都是PCB中的tag，需要完成从tag-->PCB的转换(elem2entry（定义这个变量非必要,可以用局部变量代替
    struct list_elem *thread_tag;
Lock pid_lock;
extern void switch_to(struct task_struct* cur, struct task_struct* next);
extern void init();
struct task_struct* running_thread(){
    uint32_t esp;
    asm("mov %%esp, %0":"=g"(esp));
    //各线程的栈都在pcb中，而一个pcb占一个页，取pcb的高20位就是指向这个pcb的指针
    return (struct task_struct*)(esp & 0xfffff000);
}

static void kernel_thread(thread_func* function, void* func_arg){
    intr_enable();
    function(func_arg);
    //what if thread finished? how to schedule now ? to be continued
    // struct task_struct* cur = running_thread();
    // ASSERT(elem_find(&thread_ready_list, &cur->general_tag));
    // list_remove(&cur->general_tag);
    // while(1);
    // schedule();
}

//初始化线程栈,把待执行的函数和参数放到栈中对应的位置
// 详情 ${workspace}/img/thread_stack_status_before_first_schedule.png OR P509
void init_thread_stack(struct task_struct* pcb, thread_func function, void* func_arg){
    //预留出中断栈的空间
    pcb->self_kstack -= sizeof(struct intr_stack);
    //预留出线程栈的空间
    pcb->self_kstack -= sizeof(struct thread_stack);
    //(划掉)(WTF?一个内核栈一个线程栈？) 那是内核栈包含了一个intr_stack 和 thread_stack, idiot
    struct thread_stack* kthread_stack = (struct thread_stack*)pcb->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;

}

static pid_t allocate_pid(){
    static pid_t next_pid = 0;
    lock_acquire(&pid_lock);
    next_pid++;
    lock_release(&pid_lock);
    return next_pid;
}

//初始化线程的基本信息
void init_thread(struct task_struct* pcb, char* tname, int prio){
    memset(pcb, 0, sizeof(*pcb));
    pcb->pid = allocate_pid();
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
    //预留标准输入输出标准错误
    pcb->fd_table[0] = 0;
    pcb->fd_table[1] = 1;
    pcb->fd_table[2] = 2;
    uint8_t fd_idx = 3;
    for(;fd_idx < MAX_FILES_OPEN_PER_PROC; fd_idx++){
        pcb->fd_table[fd_idx] = -1;
    }
    pcb->cwd_inode_nr = 0;  //根目录为默认工作路径
    pcb->parent_pid = -1;    
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

    if(list_empty(&thread_ready_list)){
        thread_unblock(idle_thread);
    }
   ASSERT(!list_empty(&thread_ready_list));
   thread_tag = NULL;	  // thread_tag清空
/* 将thread_ready_list队列中的第一个就绪线程弹出,准备将其调度上cpu. */
   thread_tag = list_pop_front(&thread_ready_list);   
   struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
   next->status = TASK_RUNNING;
   process_activate(next);
   switch_to(cur, next);
}

void thread_init(){
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    lock_init(&pid_lock);
    process_execute(init, "init");
    make_main_thread();
    idle_thread = thread_start("idle", 10, idle, NULL);
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
        list_push_front(&thread_ready_list, &thread->general_tag);
        thread->status = TASK_READY;
    }
    intr_set_status(intr_old_status);
}

//系统空闲时运行的线程
static void idle(void* arg){
    while(1){
        thread_block(TASK_BLOCKED);
        asm volatile("sti; hlt" :::"memory");
    }
}

//主动把当前进程的cpu使用权让出来
void thread_yield(){
    struct task_struct* cur = running_thread();
    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
    list_push_back(&thread_ready_list, &cur->general_tag);
    cur->status = TASK_READY;
    schedule();
    intr_set_status(old_status);
}

pid_t fork_pid(){
    return allocate_pid();
}

/* 以填充空格的方式输出buf */
static void pad_print(char *buf, int32_t buf_len, void *ptr, char format) {
    memset(buf, 0, buf_len);
    uint8_t out_pad_0idx = 0;
    switch (format) {
    case 's':
        out_pad_0idx = sprintf(buf, "%s", ptr);
        break;
    case 'd':
        out_pad_0idx = sprintf(buf, "%d", *((int16_t *)ptr));
    case 'x':
        out_pad_0idx = sprintf(buf, "%x", *((uint32_t *)ptr));
    }
    while (out_pad_0idx < buf_len) { // 以空格填充
        buf[out_pad_0idx] = ' ';
        out_pad_0idx++;
    }
    sys_write(stdout_no, buf, buf_len - 1);
}

/* 用于在list_traversal函数中的回调函数,用于针对线程队列的处理 */
static bool elem2thread_info(struct list_elem *pelem, int arg) {
    struct task_struct *pthread =
        elem2entry(struct task_struct, all_list_tag, pelem);
    char out_pad[16] = {0};

    pad_print(out_pad, 16, &pthread->pid, 'd');

    if (pthread->parent_pid == -1) {
        pad_print(out_pad, 16, "NULL", 's');
    } else {
        pad_print(out_pad, 16, &pthread->parent_pid, 'd');
    }

    switch (pthread->status) {
    case 0:
        pad_print(out_pad, 16, "RUNNING", 's');
        break;
    case 1:
        pad_print(out_pad, 16, "READY", 's');
        break;
    case 2:
        pad_print(out_pad, 16, "BLOCKED", 's');
        break;
    case 3:
        pad_print(out_pad, 16, "WAITING", 's');
        break;
    case 4:
        pad_print(out_pad, 16, "HANGING", 's');
        break;
    case 5:
        pad_print(out_pad, 16, "DIED", 's');
    }
    pad_print(out_pad, 16, &pthread->elapsed_ticks, 'x');

    memset(out_pad, 0, 16);
    ASSERT(strlen(pthread->name) < 17);
    memcpy(out_pad, pthread->name, strlen(pthread->name));
    strcat(out_pad, "\n");
    sys_write(stdout_no, out_pad, strlen(out_pad));
    return false; // 此处返回false是为了迎合主调函数list_traversal,只有回调函数返回false时才会继续调用此函数
}

/* 打印任务列表 */
void sys_ps(void) {
    char *ps_title =
        "PID            PPID           STAT           TICKS          COMMAND\n";
    sys_write(stdout_no, ps_title, strlen(ps_title));
    list_traversal(&thread_all_list, elem2thread_info, 0);
}