#include "string.h"
#include "debug.h"
#include "global.h"
void memset(void* dst, uint8_t value, uint32_t size){
    ASSERT(dst != NULL);
    while(size--)
        *((uint8_t*)dst++) = value;
}

void memcpy(void* dst, const void* src, uint32_t size){
    ASSERT(dst != NULL && src != NULL);
    while(size--){
       *((uint8_t*)dst++) = *((const uint8_t*) src++); 
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
