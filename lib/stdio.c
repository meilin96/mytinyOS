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
    const char *index_str = format;
    char index_char = *index_str;
    int32_t arg_int;
    while (index_char) {
        if (index_char != '%') {
            *buf_ptr++ = index_char;
            index_char = *(++index_str);
            continue;
        }
        index_char = *(++index_str);
        switch (index_char) {
        case 'x': //暂时只支持16进制的输出
            arg_int = va_arg(ap, int);
            itoa(arg_int, &buf_ptr, 16);
            //此时index_str指向 %x 中的x 因此要跳过
            index_char = *(++index_str);
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
    return write(buf);
}