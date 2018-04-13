#include "bitmap.h"
#include "string.h"
#include "print.h"
#include "stdint.h"
#include "debug.h"
#include "interrupt.h"

void bitmap_init(Bitmap* btmp){
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

bool bitmap_scan_test(Bitmap* btmp, uint32_t bit_idx){
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;
    return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

//在位图中找到连续的cnt个可用位，返回起始空位的下标，若没有则返回-1
int bitmap_scan(Bitmap* btmp, uint32_t cnt){
    uint32_t idx_byte = 0;
    while((0xff == btmp->bits[idx_byte] && (idx_byte < btmp->btmp_bytes_len))){
    idx_byte++;
   }

    ASSERT(idx_byte < btmp->btmp_bytes_len);
    if (idx_byte == btmp->btmp_bytes_len){
        return -1;
    }

    int idx_bit = 0;

}

void bitmap_set(Bitmap* btmp, uint32_t bit_idx, int8_t value){
    ASSERT(value == 0 || value == 1);
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;

    if (value){
        btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
    }else{
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
    }
}
