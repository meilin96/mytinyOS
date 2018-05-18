#include "inode.h"
#include "debug.h"
#include "fs.h"
#include "ide.h"
#include "interrupt.h"
#include "memory.h"
#include "stdint.h"
#include "stdio-kernel.h"
#include "string.h"
#include "super_block.h"

struct inode_position {
    bool two_sec; //跨扇区标识，inode在扇区末创建，一部分在下一个扇区
    uint32_t sec_lba;  //所在扇区地址
    uint32_t off_size; //在扇区内的便宜量
};

//根据inode_no获取inode在扇区内的位置并将其写入inode_pos
static void inode_locate(struct partition *part, uint32_t inode_no,
                         struct inode_position *inode_pos) {
    ASSERT(inode_no < 4096);
    uint32_t inode_table_lba = part->sb->inode_table_lba;
    uint32_t off_size = inode_no * sizeof(struct inode);
    uint32_t off_sec = off_size / SECTOR_SIZE;
    uint32_t off_size_in_sec = off_size % SECTOR_SIZE;

    //判断是否跨扇区
    uint32_t left_in_sec = 512 - off_size_in_sec;
    if (left_in_sec < sizeof(struct inode)) {
        inode_pos->two_sec = true;
    } else {
        inode_pos->two_sec = false;
    }

    inode_pos->sec_lba = inode_table_lba + off_sec;
    inode_pos->off_size = off_size_in_sec;
}

//将inode写入磁盘，通常发生在文件被修改后，即同步(sync)
void inode_sync(struct partition *part, struct inode *inode, void *io_buf) {
    uint8_t inode_no = inode->i_no;
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));

    struct inode pure_inode;
    memcpy(&pure_inode, inode, sizeof(struct inode));
    //一下成员只在内存中有用，为避免下次加载时发生错误，先将其请0
    pure_inode.i_open_cnts = 0;
    pure_inode.write_deny = false;
    pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;

    char *inode_buf = (char *)io_buf;
    if (inode_pos.two_sec) {
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
        memcpy((inode_buf + inode_pos.off_size), &pure_inode,
               sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    } else {
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
        memcpy((inode_buf + inode_pos.off_size), &pure_inode,
               sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }
}

//在part->open_inodes中寻找inode_no,如没找到则在磁盘中寻找并加入open_inodes
struct inode* inode_open(struct partition* part, uint32_t inode_no){
    struct list_elem* elem = part->open_inodes.head.next;

    struct inode *inode_found;
    while (elem != &part->open_inodes.tail) {
        inode_found = elem2entry(struct inode, inode_tag, elem);
        if (inode_found->i_no == inode_no) {
            inode_found->i_open_cnts++;
            return inode_found;
        }
        elem = elem->next;
    }

    /*由于open_inodes链表中找不到,下面从硬盘上读入此inode并加入到此链表 */
    struct inode_position inode_pos;
    /* inode位置信息会存入inode_pos, 包括inode所在扇区地址和扇区内的字节偏移量
     */
    inode_locate(part, inode_no, &inode_pos);

    /* 为使通过sys_malloc创建的新inode被所有任务共享,
     * 需要将inode置于内核空间,故需要临时
     * 将cur_pbc->pgdir置为NULL */
    struct task_struct *cur = running_thread();
    uint32_t *cur_pagedir_bak = cur->pgdir;
    cur->pgdir = NULL;
    /* 以上三行代码完成后下面分配的内存将位于内核区 */
    inode_found = (struct inode *)sys_malloc(sizeof(struct inode));
    /* 恢复pgdir */
    cur->pgdir = cur_pagedir_bak;

    char *inode_buf;
    if (inode_pos.two_sec) { // 考虑跨扇区的情况
        inode_buf = (char *)sys_malloc(1024);

        /* i结点表是被partition_format函数连续写入扇区的,
         * 所以下面可以连续读出来 */
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    } else { // 否则,所查找的inode未跨扇区,一个扇区大小的缓冲区足够
        inode_buf = (char *)sys_malloc(512);
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }
    memcpy(inode_found, inode_buf + inode_pos.off_size, sizeof(struct inode));

    /* 因为一会很可能要用到此inode,故将其插入到队首便于提前检索到 */
    list_push_front(&part->open_inodes, &inode_found->inode_tag);
    inode_found->i_open_cnts = 1;

    sys_free(inode_buf);
    return inode_found;
}

//将inode->i_open_cnt减1,若已经为0则从open_inodes_list中去掉
void inode_close(struct inode* inode){
    /* 若没有进程再打开此文件,将此inode去掉并释放空间 */
    enum intr_status old_status = intr_disable();
    if (inode->i_open_cnts == 0) {
        list_remove(&inode->inode_tag); // 将I结点从part->open_inodes中去掉
        /* inode_open时为实现inode被所有进程共享,
         * 已经在sys_malloc为inode分配了内核空间,
         * 释放inode时也要确保释放的是内核内存池 */
        struct task_struct *cur = running_thread();
        uint32_t *cur_pagedir_bak = cur->pgdir;
        cur->pgdir = NULL;
        sys_free(inode);
        cur->pgdir = cur_pagedir_bak;
    }else{
        inode->i_open_cnts--;
    }
    intr_set_status(old_status);
}

//使用inode_no初始化一个inode
void inode_init(uint32_t inode_no, struct inode* new_inode){
    new_inode->i_no = inode_no;
    new_inode->i_size = 0;
    new_inode->i_open_cnts = 0;
    new_inode->write_deny = false;

    /* 初始化块索引数组i_sector */
    uint8_t sec_idx = 0;
    while (sec_idx < 13) {
        /* i_sectors[12]为一级间接块地址 */
        new_inode->i_sectors[sec_idx] = 0;
        sec_idx++;
    }
}