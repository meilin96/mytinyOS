#include "console.h"
#include "debug.h"
#include "dir.h"
#include "fs.h"
#include "init.h"
#include "interrupt.h"
#include "memory.h"
#include "print.h"
#include "process.h"
#include "stdio.h"
#include "syscall-init.h"
#include "syscall.h"
#include "thread.h"
#include "shell.h"
void k_thread_a(void *);
void k_thread_b(void *);
void u_prog_a(void);
void u_prog_b(void);
void init();

int main(void) {
    put_str("I am kernel\n");
    init_all();
    cls_screen();
    
    /********  测试代码  ********/
    // char cwd_buf[32] = {0};
    // sys_getcwd(cwd_buf, 32);
    // printf("cwd: %s\n", cwd_buf);
    // sys_chdir("/dir1");
    // printf("change cwd now\n");
    // sys_getcwd(cwd_buf, 32);
    // printf("cwd: %s\n", cwd_buf);

    // struct stat obj_stat;
    // sys_stat("/", &obj_stat);
    // printf("/`s info \n i_no: %d\n   size:%d\n   filetype:%s\n",
    //        obj_stat.st_ino, obj_stat.st_size,
    //        obj_stat.st_filetype == 2 ? "directory" : "regular");
    // sys_stat("/dir1", &obj_stat);
    // printf("/`s info \n i_no: %d\n   size:%d\n   filetype:%s\n",
    //        obj_stat.st_ino, obj_stat.st_size,
    //        obj_stat.st_filetype == 2 ? "directory" : "regular");
    /********  测试代码  ********/
    while (1)
        ;
    return 0;
}

void init() {
    uint32_t ret_pid = fork();
    if (ret_pid) {
        // printf("I am father, my pid is %d, child pid is %d\n", getpid(),
        //    ret_pid);
        while (1)
            ;
    } else {
        my_shell();
    }
}

/* 在线程中运行的函数 */
void k_thread_a(void *arg) {
    void *addr1 = sys_malloc(256);
    void *addr2 = sys_malloc(255);
    void *addr3 = sys_malloc(254);
    console_put_str(" thread_a malloc addr:0x");
    console_put_int((int)addr1);
    console_put_char(',');
    console_put_int((int)addr2);
    console_put_char(',');
    console_put_int((int)addr3);
    console_put_char('\n');

    int cpu_delay = 100000;
    while (cpu_delay-- > 0)
        ;
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    while (1)
        ;
}

/* 在线程中运行的函数 */
void k_thread_b(void *arg) {
    void *addr1 = sys_malloc(256);
    void *addr2 = sys_malloc(255);
    void *addr3 = sys_malloc(254);
    console_put_str(" thread_b malloc addr:0x");
    console_put_int((int)addr1);
    console_put_char(',');
    console_put_int((int)addr2);
    console_put_char(',');
    console_put_int((int)addr3);
    console_put_char('\n');

    int cpu_delay = 100000;
    while (cpu_delay-- > 0)
        ;
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    while (1)
        ;
}

/* 测试用户进程 */
void u_prog_a(void) {
    void *addr1 = malloc(256);
    void *addr2 = malloc(255);
    void *addr3 = malloc(254);
    printf(" prog_a malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2,
           (int)addr3);

    int cpu_delay = 100000;
    while (cpu_delay-- > 0)
        ;
    free(addr1);
    free(addr2);
    free(addr3);
    while (1)
        ;
}

/* 测试用户进程 */
void u_prog_b(void) {
    void *addr1 = malloc(256);
    void *addr2 = malloc(255);
    void *addr3 = malloc(254);
    printf(" prog_b malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2,
           (int)addr3);

    int cpu_delay = 100000;
    while (cpu_delay-- > 0)
        ;
    free(addr1);
    free(addr2);
    free(addr3);
    while (1)
        ;
}
