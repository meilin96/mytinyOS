#include "stdio.h"
#include "global.h"
#include "interrupt.h"
#include "print.h"
#include "stdint.h"
#include "string.h"
#include "syscall.h"
//初始化指针ap(Arguments Pointer),把ap指向栈中可变参数的第一个参数v
#define va_start(ap, v) ap = (va_list)&v
//使ap指向下一个参数，32位的存储单元是4字节
#define va_arg(ap, t) *((t*)(ap += 4))
//清空ap
#define va_end(ap) ap = NULL
//将整形转换成字符ASCII(integer to ascii)
void itoa(uint32_t value, char **buf_ptr_addr, uint8_t base) {
    uint32_t m = value % base;
    uint32_t i = value / base;
    if (i) {
        itoa(i, buf_ptr_addr, base);
    }
    if (m < 10) {
        *((*buf_ptr_addr)++) = m + '0';
    } else {
        *((*buf_ptr_addr)++) = m - 10 + 'A';
    }
}

uint32_t vsprintf(char *str, const char *format, va_list ap) {
    char *buf_ptr = str;
    const char *index_ptr = format;
    char index_char = *index_ptr;
    char* arg_str;
    int32_t arg_int;
    while (index_char) {
        if (index_char != '%') {
            *buf_ptr++ = index_char;
            index_char = *(++index_ptr);
            continue;
        }
        index_char = *(++index_ptr);
        switch (index_char) {
        case 'x': //暂时只支持16进制的输出
            arg_int = va_arg(ap, int);
            itoa(arg_int, &buf_ptr, 16);
            //此时index_ptr指向 %x 中的x 因此要跳过
            index_char = *(++index_ptr);
            break;
        case 'd':
            arg_int = va_arg(ap, int);
            if(arg_int < 0){
                arg_int = 0 - arg_int;
                *buf_ptr++ = '-';
            }
            itoa(arg_int, &buf_ptr, 10);
            index_char = *(++index_ptr);
            break;
        case 'c':
            *(buf_ptr++) = va_arg(ap, char);
            index_char = *(++index_ptr);
            break;
        case 's':
            arg_str = va_arg(ap , char*);
            strcpy(buf_ptr, arg_str);
            buf_ptr += strlen(arg_str);
            index_char = *(++index_ptr);
            break;
        }
    }
    return strlen(str);
}

uint32_t printf(const char* format, ...){
    va_list args;
    va_start(args, format);
    char buf[1024] = {'0'};
    vsprintf(buf, format, args);
    va_end(args);
    return write(1, buf, strlen(buf));
}

uint32_t sprintf(char* buf, const char* format, ...){
    va_list args;
    uint32_t res;
    va_start(args, format);
    res = vsprintf(buf, format, args);
    va_end(args);
    return res;
}