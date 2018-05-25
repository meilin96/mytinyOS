#include "shell.h"
#include "debug.h"
#include "file.h"
#include "fs.h"
#include "global.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "interrupt.h"
#define CMD_LEN 128   // 最大支持键入128个字符的命令行输入
#define MAX_ARG_NR 16 // 加上命令名外,最多支持15个参数

static char cmd_line[CMD_LEN] = {0};
//当前目录路径，每次切换目录刷新
char cwd_cache[64] = {0};

//打印提示符
void print_prompt() { printf("merin@localhost:%s$ ", cwd_cache); }

//从键盘缓冲区中最多读入count个字符到buf
static void readline(char *buf, int32_t count) {
    ASSERT(buf != NULL && count > 0);
    char *pos = buf;
    while (read(stdin_no, pos, 1) != -1 && (pos - buf) < count) {
        switch (*pos) {
        case '\n':
        case '\r':
            *pos = 0;
            putchar('\n');
            return;
        case '\b':
            if (buf[0] != '\b') {
                --pos;
                putchar('\b');
            }
            break;
        default:
            putchar(*pos);
            pos++;
        }
    }
    printf("readline: can't find enter_key in the cmd_line, \
    max num of char is 128\n");
}

void my_shell(){
    cwd_cache[0] = '/';
    while(1){
        print_prompt();
        memset(cmd_line, 0, CMD_LEN);
        readline(cmd_line, CMD_LEN);
        if(cmd_line[0] == 0){
            continue;
        }
    }
    PANIC("my_shell: should not be here");
}