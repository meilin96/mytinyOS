#include "file.h"
#include "debug.h"
#include "fs.h"
#include "global.h"
#include "ide.h"
#include "inode.h"
#include "interrupt.h"
#include "list.h"
#include "stdio-kernel.h"
#include "string.h"
#include "thread.h"
#include "super_block.h"

#define DEFAULT_SECS 1 //

//文件表(文件结构数组)
struct file file_table[MAX_FILE_OPEN];

//从file_table中获取一个空闲位返回下标，失败返回-1
int32_t get_free_slot_in_global(void) {
    uint32_t fd_idx = 3; // 0 1 2为标准io
    for (; fd_idx < MAX_FILE_OPEN; fd_idx++) {
        if (file_table[fd_idx].fd_inode == NULL) {
            break;
        }
    }
    if (fd_idx == MAX_FILE_OPEN) {
        printk("exceed max open files\n");
        return -1;
    }
    return fd_idx;
}

//在进程或线程的文件描述符中安装file_table[globa_fd_idx],返回其下标，失败返回-1
int32_t pcb_fd_install(int32_t globa_fd_idx) {
    struct task_struct *cur = running_thread();
    uint32_t local_fd_idx = 3;
    for (; local_fd_idx < MAX_FILES_OPEN_PER_PROC; local_fd_idx++) {
        if (cur->fd_table[local_fd_idx] == -1) {
            cur->fd_table[local_fd_idx] = globa_fd_idx;
            break;
        }
    }

    if (local_fd_idx == MAX_FILES_OPEN_PER_PROC) {
        printk("exceed max open files_per_proc\n");
        return -1;
    }
    return local_fd_idx;
}

//分配一个inode
int32_t inode_bitmap_alloc(struct partition *part) {
    int32_t bit_idx = bitmap_scan(&part->inode_bitmap, 1);
    if (bit_idx == -1)
        return -1;
    bitmap_set(&part->inode_bitmap, bit_idx, 1);
    return bit_idx;
}

//分配一个扇区
int32_t block_bitmap_alloc(struct partition *part) {
    int32_t bit_idx = bitmap_scan(&part->block_bitmap, 1);
    if (bit_idx == -1) {
        return -1;
    }
    bitmap_set(&part->block_bitmap, bit_idx, 1);
    return (part->sb->data_start_lba + bit_idx);
}

//将bitmap同步到硬盘
void bitmap_sync(struct partition *part, uint32_t bit_idx, uint8_t btmp) {
    uint32_t off_sec = bit_idx / BITS_PER_SECTOR; // 本i结点索引相对于位图的扇区偏移量
    uint32_t off_size =
        bit_idx / 8; // 本i结点索引相对于位图的字节偏移量
    uint32_t sec_lba;
    uint8_t *bitmap_off;

    /* 需要被同步到硬盘的位图只有inode_bitmap和block_bitmap */
    switch (btmp) {
    case INODE_BITMAP:
        sec_lba = part->sb->inode_bitmap_lba + off_sec;
        bitmap_off = part->inode_bitmap.bits + off_size;
        break;

    case BLOCK_BITMAP:
        sec_lba = part->sb->block_bitmap_lba + off_sec;
        bitmap_off = part->block_bitmap.bits + off_size;
        break;
    }
    ide_write(part->my_disk, sec_lba, bitmap_off, 1);
}