#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H
#include "stdint.h"
typedef char* va_list;
uint32_t printf(const char* str, ...);
uint32_t vsprintf(char* str, const char* format, va_list ap);
void itoa(uint32_t value, char **buf_ptr_addr, uint8_t base);
#endif