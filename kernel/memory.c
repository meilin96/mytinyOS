#include "memory.h"
#include "stdint.h"
#include "print.h"

#define PG_SIZE 4096

#define MEM_BITMAP_BASE 0xc009a000 //P384

#define K_HEAP_START 0xc0100000

//物理内存内存池
struct pool {
    Bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
};


Pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;

static void mem_pool_init(uint32_t all_mem){
   put_str(" mem_pool_init start!\n");
   uint32_t page_table_size = PG_SIZE * 256;
}
