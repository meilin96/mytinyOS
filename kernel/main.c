#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
void main(void) {
    put_str("I am kernel\n");
    init_all();
//   asm volatile("sti");	     // 为演示中断处理,在此临时开中断
    void* vaddr = get_kernel_pages(1);
    put_int((uint32_t)vaddr);
    put_str("\n");
    while(1);
}
