#include "print.h"
#include "init.h"
#include "debug.h"
void main(void) {
    put_str("I am kernel\n");
    init_all();
//   asm volatile("sti");	     // 为演示中断处理,在此临时开中断
    put_int(*(uint32_t*)(0xb00));
    ASSERT(1==2);
    while(1);
}
