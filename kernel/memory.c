#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "bitmap.h"
#include "debug.h"
#include "string.h"
#define PG_SIZE 4096

#define MEM_BITMAP_BASE 0xc009a000 //P384

#define K_HEAP_START 0xc0100000

//物理内存内存池
struct pool {
    Bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
};

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)


struct virtual_addr kernel_vaddr;
typedef struct pool Pool; 
Pool kernel_pool, user_pool;

//在虚拟内存池中申请连续的pg_cnt个虚拟页
//成功返回虚拟页的起始地址，否则返回NULL
static void* vaddr_pool_apply(enum pool_flags pf, uint32_t pg_cnt){
    int vaddr_start = 0, bit_idx_start = -1;
    uint32_t cnt = 0;
    if(pf == PF_KERNEL){
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if(bit_idx_start == -1) return NULL;
        while(cnt < pg_cnt){
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt, 1);
            cnt++;
        }
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }else{
    //用户内存池 to be continued
    }
    return (void*)vaddr_start;
}

//***************用于修改页表和页目录表
//根据虚拟地址返回对应pte的指针(虚拟地址)
uint32_t* pte_ptr(uint32_t vaddr){
    uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}
//根据虚拟地址返回对应pde的指针(虚拟地址)
uint32_t* pde_ptr(uint32_t vaddr){
    uint32_t* pde = (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
    return pde;
}
//**************

//在m_pool指向的物理内存池中分配一个物理页，成功发回页的物理地址，否则返回NULL
static void* phy_page_alloc(Pool* m_pool){
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if(bit_idx == -1) return NULL;
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = (bit_idx * PG_SIZE) + m_pool->phy_addr_start;
    return (void*)page_phyaddr;
}
//在页表中添加与物理地址对应的pte
static void page_table_add(void* _vaddr, void* _page_phyaddr){
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);

    //先判断对应的pde是否存在
    if (*pde & 0x00000001){
        put_int((uint32_t)_vaddr);put_str("\n");put_int((uint32_t) _page_phyaddr);
        ASSERT(!(*pte & 0x00000001));//如果对应pte已经存在则报错
        if(!(*pte & 0x00000001)){
            *pte = page_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
        }else{
            //PANIC("pte repeat");
            //*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }else{//如果对应的pde不存在，则先分配一个物理页作为页表
        uint32_t pde_phyaddr = (uint32_t)phy_page_alloc(&kernel_pool);
        *pde = pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
        //然后再把页表内容清零;
        memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);

        ASSERT(!(*pte & 0x00000001));//pte已存在报错 
        *pte = page_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
    }
}

//在pf指定的内存池中分配pg_cnt个页表,成功返回起始虚拟地址
void* malloc_pages(enum pool_flags pf, uint32_t pg_cnt){
    //根据内存池限制申请的内存页数大小(15MB / 4k = 3840)
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    /*
    1.vaddr_pool_apply 在虚拟内存池中申请分配内存页。
    2.phy_page_alloc 分配物理内存
    3.page_table_add 在添加页表中添加映射
    */
    void* vaddr_start = vaddr_pool_apply(pf, pg_cnt);
    if (vaddr_start == NULL)
        return NULL;

    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

    while(cnt--){
        void* page_phyaddr = phy_page_alloc(mem_pool);
        if (page_phyaddr == NULL){
            //失败要把所有申请的虚拟页和物理页回滚，to be continued
            ASSERT(page_phyaddr);
            return NULL;
        }
        page_table_add((void*)vaddr, page_phyaddr);
        vaddr += PG_SIZE;
    }

    return vaddr_start;
}

//在内核物理内存池中申请pg_cnt页内存，成功则返回虚拟地址，失败则返回NULL
void* get_kernel_pages(uint32_t pg_cnt){
    void* vaddr = malloc_pages(PF_KERNEL, pg_cnt);
    if(vaddr != NULL){
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    }
    return vaddr;
}

//初始化物理内存池
static void mem_pool_init(uint32_t all_mem){
    put_str(" mem_pool_init start!\n");
    uint32_t page_table_size = PG_SIZE * 256;

    uint32_t used_mem = page_table_size + 0x100000;

    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;

    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    uint32_t kbm_length = kernel_free_pages / 8;
    uint32_t ubm_length = user_free_pages / 8;

    uint32_t kp_start = used_mem;
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;
    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

    put_str("   kernel_pool_bitmap_start: ");put_int((int)kernel_pool.pool_bitmap.bits);
    put_str("  kernel_pool_phy_addr_start: ");put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("   user_pool_bitmap_start: "); put_int((int)user_pool.pool_bitmap.bits);
    put_str("  user_pool_phy_addr_start: "); put_int(user_pool.phy_addr_start);
    put_str("\n");

    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("mem_pool_init done\n");    
}

void mem_init(){
    put_str("mem_init start\n");
    uint32_t mem_bytes_total = *(uint32_t*)(0xb00);
    mem_pool_init(mem_bytes_total);
    put_str("mem_init done!\n");
}
