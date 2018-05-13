#ifndef __FS_INODE_H
#define __FS_INODE_H
#include "list.h"
#include "stdint.h"

// inode结构
struct inode {
    uint32_t i_no; // inode编号

    // 当此inode是文件时,i_size是指文件大小,
    // 若此inode是目录,i_size是指该目录下所有目录项大小之和
    // 以字节为单位而不是块
    uint32_t i_size;

    uint32_t i_open_cnts; // 记录此文件被打开的次数
    bool write_deny; // 写文件不能并行,进程写文件前检查此标识

    // i_sectors[0-11]是直接块, i_sectors[12]用来存储一级间接块指针
    uint32_t i_sectors[13];
    //暂时只支持一级间接块索引表
    // 12 + sizeof(sector) / sizeof(uint32_t) == 12 + 512/4 == 140
    struct list_elem inode_tag; //已打开的inode队列
};
#endif
