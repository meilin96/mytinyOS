#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "bitmap.h"
#include "debug.h"
#include "string.h"
#include "sync.h"
#include "interrupt.h"
#define PG_SIZE 4096

#define MEM_BITMAP_BASE 0xc009a000 //P384

#define K_HEAP_START 0xc0100000

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)
//物理内存内存池
struct pool {
    Bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
    Lock lock;
};

struct arena {
    struct mem_block_desc* desc;
    //空闲mem_block数,当large为true时表示本arena占用的页框数
    uint32_t free_block_cnt;
    bool large;                     
};

struct mem_block_desc k_block_desc[DESC_CNT];
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
        struct task_struct* cur = running_thread();
        bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1) return NULL;
        while(cnt < pg_cnt){
            bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt, 1);
            cnt++;
        }
        vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));

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
    lock_acquire(&kernel_pool.lock);
    void* vaddr = malloc_pages(PF_KERNEL, pg_cnt);
    if(vaddr != NULL){
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    }
    lock_release(&kernel_pool.lock);
    return vaddr;
}

void* get_user_pages(uint32_t pg_cnt){
    lock_acquire(&user_pool.lock);
    void* vaddr = malloc_pages(PF_USER, pg_cnt);
    if(vaddr != NULL){
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    }
    lock_release(&user_pool.lock);
    return vaddr;
}

//分配一个指定的物理页给vaddr
void* get_a_page(enum pool_flags pf, uint32_t vaddr){
    struct pool* mem_pool = pf == PF_KERNEL?&kernel_pool:&user_pool;
    lock_acquire(&mem_pool->lock);
    
    struct task_struct* cur = running_thread();
    int32_t bit_idx = -1;

    if(cur->pgdir != NULL && pf == PF_USER){
        bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);
    }else if(cur->pgdir == NULL && pf == PF_KERNEL){
        bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);
    }else{
        PANIC("get_a_page error!");
    }

    void* page_phyaddr = phy_page_alloc(mem_pool);
    page_table_add((void*) vaddr, page_phyaddr);
    lock_release(&mem_pool->lock);
    return (void*)vaddr;
}

//将虚拟地址映射到物理地址
uint32_t addr_v2p(uint32_t vaddr){
    uint32_t* pte = pte_ptr(vaddr);
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

//初始化物理内存池
static void mem_pool_init(uint32_t all_mem){
    put_str(" mem_pool_init start!\n");
    uint32_t page_table_size = PG_SIZE * 256;

    uint32_t used_mem = 0x100000 + page_table_size; //页表和低端1M

    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;

    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;
    //位图中一位代表一页，所有除以一byte的位数，同时忽略余数
    //好处是不用做内存越界检查，坏处是可用内存略少于实际内存（最多7 * PG_SIZE
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

    bitmap_bzero(&kernel_pool.pool_bitmap);
    bitmap_bzero(&user_pool.pool_bitmap);

    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_bzero(&kernel_vaddr.vaddr_bitmap);
    lock_init(&kernel_pool.lock);
    lock_init(&user_pool.lock);
    put_str("mem_pool_init done\n");    
}

//为7种desc做初始化
void block_desc_init(struct mem_block_desc *desc_array) {
    uint16_t desc_idx, block_size = 16;
    for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
        desc_array[desc_idx].block_size = block_size;
        desc_array[desc_idx].blocks_per_arena =
            (PG_SIZE - sizeof(struct arena)) / block_size;
        list_init(&desc_array[desc_idx].free_list);
        block_size *= 2;
    }
}

void mem_init(){
    put_str("mem_init start\n");
    //在loader中把机器的内存容量保存在了地址(0xb00)中
    uint32_t mem_bytes_total = *(uint32_t*)(0xb00);
    mem_pool_init(mem_bytes_total);
    block_desc_init(k_block_desc);
    put_str("mem_init done!\n");
}

//返回arena中第idx个内存块的地址
static struct mem_block* arena2block(struct arena* a, uint32_t idx){
    return (struct mem_block*)((uint32_t)a + sizeof(struct arena) + idx * a->desc->block_size);
}

//返回block所在arena的地址
static struct arena* block2arena(struct mem_block* b){
    return (struct arena*)((uint32_t)b & 0xfffff000);
}

void* sys_malloc(uint32_t size){
    enum pool_flags PF;
    struct pool* mem_pool;
    uint32_t pool_size;
    struct mem_block_desc* descs;
    struct task_struct* cur_thread = running_thread();

    if(cur_thread->pgdir == NULL){
        PF = PF_KERNEL;
        pool_size = kernel_pool.pool_size;
        mem_pool = &kernel_pool;
        descs = k_block_desc;
    }else{
        PF = PF_USER;
        pool_size = kernel_pool.pool_size;
        mem_pool = &kernel_pool;
        descs = cur_thread->u_block_desc;
    }

    if(!(size > 0 && size < pool_size)){
        return NULL;
    }
    struct arena* a;
    struct mem_block* b;
    lock_acquire(&mem_pool->lock);
    //申请的内存大于1024，直接分配一页
    if(size > 1024){
        uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PG_SIZE);
        a = malloc_pages(PF, page_cnt);

        if(a != NULL){
            bzero(a, page_cnt * PG_SIZE);
            a->desc = NULL;
            a->free_block_cnt = page_cnt;
            a->large = true;
            lock_release(&mem_pool->lock);
            return (void*)(a + 1);
        }else{
            lock_release(&mem_pool->lock);
            return NULL;
        }
    }else{
        uint8_t desc_idx;
        for(desc_idx = 0;desc_idx < DESC_CNT;desc_idx++){
            if(size <= descs[desc_idx].block_size)
                break;
        }
        //mem_desc中已经没有可用的block，创建新arena提供block
        if( list_empty(&descs[desc_idx].free_list)){
            a = malloc_pages(PF, 1);
            if (a == NULL){
                lock_release(&mem_pool->lock);
                return NULL;
            }
            bzero(a, PG_SIZE);
            a->desc = &descs[desc_idx];
            a->large = false;
            a->free_block_cnt = descs[desc_idx].blocks_per_arena;
            uint32_t block_idx;

            enum intr_status old_status = intr_disable();
            for(block_idx = 0;block_idx < descs[desc_idx].blocks_per_arena; block_idx++){
                b = arena2block(a, block_idx);
                ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
                list_push_back(&a->desc->free_list, &b->free_elem);
            }
            intr_set_status(old_status);
        }
        b = elem2entry(struct mem_block, free_elem, list_pop_front(&(descs[desc_idx].free_list)));
        bzero(b, descs[desc_idx].block_size);

        a = block2arena(b);
        a->free_block_cnt--;
        lock_release(&mem_pool->lock);
        return (void*) b;
    }
}

//在物理内存池中回收pg_phy_addr所在的页
void pfree(uint32_t pg_phy_addr){
    struct pool* mem_pool;
    uint32_t bit_idx = 0;
    //根据地址所在的位置判断是哪个物理内存池详见mem_pool_init
    if(pg_phy_addr >= user_pool.phy_addr_start)
        mem_pool = &user_pool;
    else
        mem_pool = &kernel_pool;

    bit_idx = (pg_phy_addr - mem_pool->phy_addr_start) / PG_SIZE;
    bitmap_set(&mem_pool->pool_bitmap, bit_idx, 0);
}

//将vaddr所在页表的pte的P位置0, 达到在页表中去掉虚拟地址映射的目地
void page_table_pte_remove(uint32_t vaddr){
    uint32_t* pte = pte_ptr(vaddr);
    *pte &= ~PG_P_1;
    //跟新快表
    asm volatile("invlpg %0"::"m"(vaddr):"memory");
}

//在虚拟地址池中释放以vaddr为起始地址的连续pg_cnt个虚拟页地址
void vaddr_remove(enum pool_flags PF, void* _vaddr, uint32_t pg_cnt){
    uint32_t bit_idx_start = 0, vaddr = (uint32_t)_vaddr, cnt = 0;
    if(PF == PF_KERNEL){
        bit_idx_start = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        while(cnt < pg_cnt){
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }else{
        struct task_struct* cur_thread = running_thread();
        bit_idx_start = (vaddr - cur_thread->userprog_vaddr.vaddr_start) / PG_SIZE;
        while(cnt < pg_cnt){
            bitmap_set(&cur_thread->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }
}

//释放以vaddr起始的 连续pgcnt个内存页
void mfree_page(enum pool_flags PF, void *_vaddr, uint32_t pg_cnt) {
    uint32_t pg_phy_addr;
    uint32_t vaddr = (int32_t)_vaddr, page_cnt = 0;
    ASSERT(pg_cnt >= 1 && vaddr % PG_SIZE == 0);
    pg_phy_addr = addr_v2p(vaddr);

    //确认释放的空间在低1M + pdt之外
    ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= 0x102000);
    if (pg_phy_addr >= user_pool.phy_addr_start) {
        vaddr -= PG_SIZE;
        while (page_cnt < pg_cnt) {
            vaddr += PG_SIZE;
            pg_phy_addr = addr_v2p(vaddr);

            ASSERT((pg_phy_addr % PG_SIZE) == 0 &&
                   pg_phy_addr >= user_pool.phy_addr_start);

            pfree(pg_phy_addr);
            page_table_pte_remove(vaddr);
            page_cnt++;
        }
        vaddr_remove(PF, _vaddr, pg_cnt);
    } else {
        vaddr -= PG_SIZE;
        while (page_cnt < pg_cnt) {
            vaddr += PG_SIZE;
            pg_phy_addr = addr_v2p(vaddr);

            ASSERT((pg_phy_addr % PG_SIZE) == 0 &&
                   pg_phy_addr >= kernel_pool.phy_addr_start &&
                   pg_phy_addr < user_pool.phy_addr_start);

            pfree(pg_phy_addr);
            page_table_pte_remove(vaddr);
            page_cnt++;
        }
        vaddr_remove(PF, _vaddr, pg_cnt);
    }
}

void sys_free(void *ptr) {
    ASSERT(ptr != NULL);
    if (ptr != NULL) {
        enum pool_flags PF;
        struct pool *mem_pool;
        if (running_thread()->pgdir == NULL) {
            ASSERT((uint32_t)ptr >= K_HEAP_START);
            PF = PF_KERNEL;
            mem_pool = &kernel_pool;
        } else {
            PF = PF_USER;
            mem_pool = &user_pool;
        }

        lock_acquire(&mem_pool->lock);
        struct mem_block *b = ptr;
        struct arena *a = block2arena(b);

        ASSERT(a->large == 0 || a->large == 1);
        if (a->desc == NULL && a->large == true) {
            mfree_page(PF, a, a->free_block_cnt);
        } else {
            list_push_back(&a->desc->free_list, &b->free_elem);
            a->free_block_cnt += 1;
            //如果arena中的所有block都是空闲状态，就释放整个页
            if (a->free_block_cnt == a->desc->blocks_per_arena) {
                uint32_t block_idx;
                for (block_idx = 0; block_idx < a->desc->blocks_per_arena;
                     block_idx++) {
                    struct mem_block *b = arena2block(a, block_idx);
                    ASSERT(elem_find(&a->desc->free_list, &b->free_elem));
                    list_remove(&b->free_elem);
                }
                mfree_page(PF, a, 1);
            }
        }
        lock_release(&mem_pool->lock);
    }
}