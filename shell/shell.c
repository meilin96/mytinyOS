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

static int32_t cmd_parse(char *cmd_str, char **argv, char token);
char final_path[MAX_PATH_LEN] = {0}; // 用于洗路径时的缓冲
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

char *argv[MAX_ARG_NR]; // argv必须为全局变量，为了以后exec的程序可访问参数
int32_t argc = -1;

void my_shell(){
    cwd_cache[0] = '/';
    while(1){
        print_prompt();
        memset(cmd_line, 0, CMD_LEN);
        memset(final_path, 0, MAX_PATH_LEN);
        readline(cmd_line, CMD_LEN);
        if(cmd_line[0] == 0){
            continue;
        }
        argc = -1;
        argc = cmd_parse(cmd_line, argv, ' ');
        if(argc == -1){
            printf("num of arguments exceed %d\n", MAX_ARG_NR);
            continue;
        }
        int32_t arg_idx = 0;
        while(arg_idx < argc){
            printf("%s ", argv[arg_idx]);
            arg_idx++;
        }
        printf("\n");
    }
    PANIC("my_shell: should not be here");
}

/* 分析字符串cmd_str中以token为分隔符的单词,将各单词的指针存入argv数组 */
static int32_t cmd_parse(char *cmd_str, char **argv, char token) {
    ASSERT(cmd_str != NULL);
    int32_t arg_idx = 0;
    while (arg_idx < MAX_ARG_NR) {
        argv[arg_idx] = NULL;
        arg_idx++;
    }
    char *next = cmd_str;
    int32_t argc = 0;
    /* 外层循环处理整个命令行 */
    while (*next) {
        /* 去除命令字或参数之间的空格 */
        while (*next == token) {
            next++;
        }
        /* 处理最后一个参数后接空格的情况,如"ls dir2 " */
        if (*next == 0) {
            break;
        }
        argv[argc] = next;

        /* 内层循环处理命令行中的每个命令字及参数 */
        while (*next && *next != token) { // 在字符串结束前找单词分隔符
            next++;
        }

        /* 如果未结束(是token字符),使tocken变成0 */
        if (*next) {
            *next++ =
                0; // 将token字符替换为字符串结束符0,做为一个单词的结束,并将字符指针next指向下一个字符
        }

        /* 避免argv数组访问越界,参数过多则返回0 */
        if (argc > MAX_ARG_NR) {
            return -1;
        }
        argc++;
    }
    return argc;
}
