#include "init.h"
#include "../device/timer.h" // 用相对路径演示头文件包含
#include "console.h"
#include "fs.h"
#include "ide.h"
#include "interrupt.h"
#include "keyboard.h"
#include "memory.h"
#include "print.h"
#include "syscall-init.h"
#include "thread.h"
#include "tss.h"
/*负责初始化所有模块 */
void init_all() {
    put_str("init_all\n");
    idt_init(); // 初始化中断
    mem_init();
    thread_init();
    timer_init(); // 初始化PIT
    console_init();
    keyboard_init();
    tss_init();
    syscall_init();
    intr_enable();
    ide_init();
    filesys_init();
}
