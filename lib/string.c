#include "string.h"
#include "debug.h"
#include "global.h"
void memset(void* dst, uint8_t value, uint32_t size){
    ASSERT(dst != NULL);
    while(size--)
        *((uint8_t*)dst++) = value;
}

void memcpy(void* dst, const void* src, uint32_t size){
    ASSERT(dst != NULL && src != NULL && size > 0);
    uint8_t* d = dst;
    const uint8_t *s = src;
    while(size-- > 0){
        *d++ = *s++; 
    }
}

int memcmp(const void* p1, const void* p2, uint32_t size){
    ASSERT(p1 != NULL && p2 != NULL)
    const uint8_t* a = (const uint8_t*)p1;
    const uint8_t* b = (const uint8_t*)p2;
    while(size--){
        if(*a != *b){
            return *a > *b?1:-1; 
        }
        a++;b++;
    }
    return 0;
}

char* strcpy(char* dst, const char* src){
    ASSERT(dst != NULL && src != NULL);
    char* d = dst;
    while( (*(dst++) = *(src++)) != '\0');
    
    return d;
}

uint32_t strlen(const char *a){
    ASSERT(a != NULL)
    const char* p = a;
    while(*(p++) != '\0');
    return (p - a - 1);
}

int8_t strcmp(const char *a, const char *b){
    ASSERT(a != NULL && b != NULL);
    while(*a != '\0' && *a == *b){
        a++;b++;
    }

    return *a < *b?-1:*a > *b;
}

char* strchr(const char* string, const uint8_t ch){
    ASSERT(string != NULL);
    while(string != '\0'){
        if(*string == ch){
            return (char *)string;
        }
        string++;
    }
    return NULL;
}

uint32_t strchrs(const char* str, uint8_t ch){
    ASSERT(str != NULL);
    uint32_t cnt = 0;
    while(str != '\0'){
        if(*str == ch){
            cnt++;
        }
        str++;
    }
    return cnt;
}

char* strrchr(const char* string, const uint8_t ch){
    ASSERT(string != NULL);
    const char* last_char = NULL;
    while(string != '\0'){
        if(*string == ch){
            last_char = string;
        }
        string++;
    }
    return (char*)last_char;
}

//将字符串src拼接到dst后，返回拼接的地址
char* strcat(char* dst, const char* src){
    ASSERT(dst != NULL && src != NULL);
    char* str = dst;
    while(*str != '\0')
        str++;
    while(*(str)++ != *(src)++ );

    return dst;
}
