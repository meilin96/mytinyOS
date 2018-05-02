#include "process.h"
#include "global.h"
#include "debug.h"
#include "memory.h"
#include "list.h"
#include "thread.h"
#include "list.h"
#include "tss.h"
#include "interrupt.h"
#include "string.h"
#include "console.h"

extern  void intr_exit(void);
//构建进程初始上下文信息
void start_process(void* filename_){
    void* function = filename_;
    struct task_struct* cur = running_thread();
    cur->self_kstack += sizeof(struct thread_stack);
    //1.任务被中断时用来保护上下文. 2.用来填充用户进程的上下文
    //proc_stack指向cur->self_stack 也就是intr_stack的最低处
    //完全可以用局部变量代替，因为只有一次，即通过调用intr_exit伪造从中断退出的假象(高特权级—>低特权级的唯一途径
    struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;
    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
    proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
    proc_stack->gs = 0; //用户态不能直接操作显存。
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip = function;
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    //指向用户3特权级下的栈 P511
    proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USR_STACK3_VADDR) + PG_SIZE);
    proc_stack->ss = SELECTOR_U_STACK;
    asm volatile ("movl %0, %%esp; jmp intr_exit" :: "g" (proc_stack) : "memory");
}
//激活页表
void page_dir_activate(struct task_struct* p_thread){
    //默认为内核页目录表的地址,因为线程没有自己的地址空间, 不应使用进程的页表
    uint32_t pagedir_phy_addr = 0x100000; 
    if (p_thread->pgdir != NULL){
        pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir);

        asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
    }
}

