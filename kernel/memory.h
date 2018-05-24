#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#include "list.h"
#define DESC_CNT 7  //16 32 64 128 256 512 1024
//虚拟地址池,用于虚拟地址管理
struct virtual_addr {
   Bitmap vaddr_bitmap; 
   uint32_t vaddr_start;
};

enum pool_flags{
    PF_KERNEL = 1,
    PF_USER = 2
};

struct mem_block{
    struct list_elem free_elem;
};

struct mem_block_desc{
    uint32_t block_size;
    uint32_t blocks_per_arena;
    List free_list;
};

#define PG_P_1 1//存在位
#define PG_P_0 0
#define PG_RW_R 0
#define PG_RW_W 2
#define PG_US_S 0
#define PG_US_U 4

extern struct pool kernel_pool, user_pool;
void mem_init(void);
void* get_kernel_pages(uint32_t pg_cnt);
void* get_user_pages(uint32_t pg_cnt);
void* malloc_pages(enum pool_flags pf, uint32_t pg_cnt);
void malloc_init();
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
void *get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);
void block_desc_init(struct mem_block_desc *desc_array);
void *sys_malloc(uint32_t size);
void sys_free(void *ptr);
void *get_a_page_without_opvaddrbitmap(enum pool_flags pf, uint32_t vaddr);
void mfree_page(enum pool_flags PF, void *_vaddr, uint32_t pg_cnt);
#endif
