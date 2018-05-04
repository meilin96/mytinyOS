#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H
#include "global.h"
#define BITMAP_MASK 1
struct bitmap{
    uint32_t btmp_bytes_len;    //位图长度，以bytes为单位
    uint8_t* bits;              //位图地址
};

typedef struct bitmap Bitmap;

void bitmap_bzero(Bitmap* btmp);
bool bitmap_scan_test(Bitmap* btmp, uint32_t bit_idx);
int bitmap_scan(Bitmap* btmp, uint32_t cnt);
void bitmap_set(Bitmap* btmp, uint32_t bit_idx, int8_t value);
void bitmap_init(Bitmap *btmp);
#endif
