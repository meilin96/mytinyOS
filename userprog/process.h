#ifndef __USERPROG_PROCESS_H
#define __USERPROG_PROCESS_H
#include "thread.h"
#include "stdint.h"
#define USER_STACK3_VADDR (0xc0000000 - 0x1000)
#define USER_VADDR_START 0x8048000
void process_activate(struct task_struct *p_thread);
void process_execute(void* filename, char* name);
void page_dir_activate(struct task_struct *p_thread);
uint32_t *create_page_dir();
#endif