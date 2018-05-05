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
#include "print.h"
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
    proc_stack->esp = ((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE); //to be continued (void*)
    proc_stack->ss = SELECTOR_U_STACK;
    // console_put_str("\nbitmap_pg_cnt: "); Page Fault to be continued
    asm volatile ("movl %0, %%esp; jmp intr_exit" :: "g" (proc_stack) : "memory");
}
//激活页表
void page_dir_activate(struct task_struct* p_thread){
    //默认为内核页目录表的地址,因为线程没有自己的地址空间, 不应使用进程的页表
    uint32_t pagedir_phy_addr = 0x100000; 
    if (p_thread->pgdir != NULL){
        pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir);
    }
    asm volatile("movl %0, %%cr3" : : "r"(pagedir_phy_addr) : "memory");
}

//激活进程或线程的页表，更新tss中esp0为进程的0特权级栈
void process_activate(struct task_struct* p_thread){
    ASSERT(p_thread != NULL);
    page_dir_activate(p_thread);
    if(p_thread->pgdir){
        update_tss_esp(p_thread);
    }
}

//为用户创建页目录表
uint32_t* create_page_dir(){
    //为什么要把用户进程的页目录表放在内核空间中？to be continued
    uint32_t* page_dir_vaddr = get_kernel_pages(1);
    if(page_dir_vaddr == NULL){
        console_put_str("create_page_dir: get_kernel_pages fail");
        return NULL;
    }
    //把内核的页目录项(768-1023)复制到用户进程的页目录表中
    // 0x300 * 4 == 768 * PDE_SIZE
    // 0xfffff000 -> 0x100000 内核页目录表的基址 1024为256个页目录项的大小
    
    memcpy((uint32_t *)((uint32_t)page_dir_vaddr + 0x300 * 4),
           (uint32_t *)(0xfffff000 + 0x300 * 4), 1024);
    //把用户进程PGD最后一个PDE写入用户进程自己PGD的物理地址，方便内核为其创建页表
    uint32_t page_dir_paddr = addr_v2p((uint32_t)page_dir_vaddr);
    page_dir_vaddr[1023] = page_dir_paddr | PG_US_U | PG_RW_W | PG_P_1;
    return page_dir_vaddr;
}

//为用户进程创建虚拟地址位图
void create_user_vaddr_bitmap(struct task_struct* user_prog){
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) /PG_SIZE / 8, PG_SIZE);
    //在内核空间中分配其位图所需要的页框
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);  //同上 to be continued
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitmap_bzero(&user_prog->userprog_vaddr.vaddr_bitmap);
}

void process_execute(void* filename, char* name){
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, DEFAULT_PRIO);
    create_user_vaddr_bitmap(thread);
    init_thread_stack(thread, start_process, filename);
    thread->pgdir = create_page_dir();  
    enum intr_status old_status = intr_disable();
    
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_push_back(&thread_ready_list, &thread->general_tag);

    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_push_back(&thread_all_list, &thread->all_list_tag);
    intr_set_status(old_status);
}