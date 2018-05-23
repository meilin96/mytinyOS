#include "fs.h"
#include "debug.h"
#include "dir.h"
#include "global.h"
#include "ide.h"
#include "inode.h"
#include "list.h"
#include "memory.h"
#include "stdint.h"
#include "stdio-kernel.h"
#include "string.h"
#include "super_block.h"
#include "file.h"
#include "console.h"
static char* path_parse(char* pathname, char* name_store);
struct partition *cur_part; //默认情况下操作的分区

//挂载分区，也就是把磁盘中的元信息读入到内存
static bool mount_partition(ListElem *pelem, int arg) {
    char *part_name = (char *)arg;
    struct partition *part = elem2entry(struct partition, part_tag, pelem);
    if (!strcmp(part->name, part_name)) {
        cur_part = part;
        struct disk *hd = cur_part->my_disk;
        struct super_block *sb_buf =
            (struct super_block *)sys_malloc(SECTOR_SIZE);

        cur_part->sb =
            (struct super_block *)sys_malloc(sizeof(struct super_block));

        if (cur_part->sb == NULL) {
            PANIC("alloc memory failed!");  //???
        }
        //读入超级块
        memset(sb_buf, 0, SECTOR_SIZE);
        //先读入到缓冲区中
        ide_read(hd, cur_part->start_lba + 1, sb_buf, 1);
        memcpy(cur_part->sb, sb_buf, sizeof(struct super_block));
        //读入块位图
        cur_part->block_bitmap.bits =
            (uint8_t *)sys_malloc(sb_buf->block_bitmap_sects * SECTOR_SIZE);
        if (cur_part->block_bitmap.bits == NULL) {
            PANIC("alloc memory failed!");  //???
        }
        cur_part->block_bitmap.btmp_bytes_len =
            sb_buf->block_bitmap_sects * SECTOR_SIZE;
        ide_read(hd, sb_buf->block_bitmap_lba, cur_part->block_bitmap.bits,
                 sb_buf->block_bitmap_sects);
        //读入inode位图
        cur_part->inode_bitmap.bits =
            (uint8_t *)sys_malloc(sb_buf->inode_bitmap_sects * SECTOR_SIZE);
        if (cur_part->inode_bitmap.bits == NULL) {
            PANIC("alloc memory failed!");  //???
        }
        
        cur_part->inode_bitmap.btmp_bytes_len =
            sb_buf->inode_bitmap_sects * SECTOR_SIZE;

        ide_read(hd, sb_buf->inode_bitmap_lba, cur_part->inode_bitmap.bits,
                 sb_buf->inode_bitmap_sects);
        
        list_init(&cur_part->open_inodes);
        printk("mount %s done!\n", part->name);
        sys_free(sb_buf);
        return true;
    }else{
        //???? to be continued
        // char* p = part->name;
        // for(;*p != '/0';p++){
        //     printk("%c : %d\n", *p, (uint32_t)*p);
        // }
        // printk("part->name :   %s\n", part->name);
    }
    return false;
}

// 格式化分区,也就是初始化分区的元信息,创建文件系统
// (1)根据分区 part 大小,计算分区文件系统各元信息需要的扇区数及位置。
// (2)在内存中创建超级块,将以上步骤计算的元信息写入超级块。
// (3)将超级块写入磁盘。
// (4)将元信息写入磁盘上各自的位置。
// (5)将根目录写入磁盘。
static void partition_format(struct partition *part) {
    /* 为方便实现,一个块大小是一扇区 */
    uint32_t boot_sector_sects = 1;
    uint32_t super_block_sects = 1;
    uint32_t inode_bitmap_sects = DIV_ROUND_UP(
        MAX_FILES_PER_PART,
        BITS_PER_SECTOR); // I结点位图占用的扇区数.最多支持4096个文件
    uint32_t inode_table_sects = DIV_ROUND_UP(
        ((sizeof(struct inode) * MAX_FILES_PER_PART)), SECTOR_SIZE);
    uint32_t used_sects = boot_sector_sects + super_block_sects +
                          inode_bitmap_sects + inode_table_sects;
    uint32_t free_sects = part->sec_cnt - used_sects;

    /************** 简单处理块位图占据的扇区数 ***************/
    uint32_t block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
    /* block_bitmap_bit_len是位图中位的长度,也是可用块的数量 */
    uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR);
    /*********************************************************/

    /* 超级块初始化 */
    struct super_block sb;
    sb.magic = 0x05211996;
    sb.sec_cnt = part->sec_cnt;
    sb.inode_cnt = MAX_FILES_PER_PART;
    sb.part_lba_base = part->start_lba;

    sb.block_bitmap_lba = sb.part_lba_base + 2; // 第0块是引导块,第1块是超级块
    sb.block_bitmap_sects = block_bitmap_sects;

    sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
    sb.inode_bitmap_sects = inode_bitmap_sects;

    sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
    sb.inode_table_sects = inode_table_sects;

    sb.data_start_lba = sb.inode_table_lba + sb.inode_table_sects;
    //根目录
    sb.root_inode_no = 0;
    sb.dir_entry_size = sizeof(struct dir_entry);

    printk("%s info:\n", part->name);
    printk("   magic:0x%x\n   part_lba_base:0x%x\n   all_sectors:0x%x\n   "
           "inode_cnt:0x%x\n   block_bitmap_lba:0x%x\n   "
           "block_bitmap_sectors:0x%x\n   inode_bitmap_lba:0x%x\n   "
           "inode_bitmap_sectors:0x%x\n   inode_table_lba:0x%x\n   "
           "inode_table_sectors:0x%x\n   data_start_lba:0x%x\n",
           sb.magic, sb.part_lba_base, sb.sec_cnt, sb.inode_cnt,
           sb.block_bitmap_lba, sb.block_bitmap_sects, sb.inode_bitmap_lba,
           sb.inode_bitmap_sects, sb.inode_table_lba, sb.inode_table_sects,
           sb.data_start_lba);

    struct disk *hd = part->my_disk;
    /*******************************
     * 1 将超级块写入本分区的1扇区 *
     ******************************/
    ide_write(hd, part->start_lba + 1, &sb, 1);
    printk("   super_block_lba:0x%x\n", part->start_lba + 1);

    // 找出数据量最大的元信息,用其尺寸做存储缓冲区
    // 一个扇区的块位图能代表的容量也仅为512*8*512 = 2Mb
    // 通过malloc分配缓冲区，将起赋值后再写入硬盘
    uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects
                             ? sb.block_bitmap_sects
                             : sb.inode_bitmap_sects);
    buf_size =
        (buf_size >= sb.inode_table_sects ? buf_size : sb.inode_table_sects) *
        SECTOR_SIZE;
    uint8_t *buf =
        (uint8_t *)sys_malloc(buf_size); // 申请的内存由内存管理系统清0后返回

    /**************************************
     * 2 将块位图初始化并写入sb.block_bitmap_lba *
     *************************************/
    /* 初始化块位图block_bitmap */
    buf[0] |= 0x01; // 第0个块预留给根目录,位图中先占位
    uint32_t block_bitmap_last_byte = block_bitmap_bit_len / 8;
    uint8_t block_bitmap_last_bit = block_bitmap_bit_len % 8;
    uint32_t last_size =
        SECTOR_SIZE -
        (block_bitmap_last_byte %
         SECTOR_SIZE); // last_size是位图所在最后一个扇区中，不足一扇区的其余部分

    /* 1
     * 先将位图最后一字节到其所在的扇区的结束全置为1,即超出实际块数的部分直接置为已占用*/
    memset(&buf[block_bitmap_last_byte], 0xff, last_size);

    /* 2 再将上一步中覆盖的最后一字节内的有效位重新置0 */
    uint8_t bit_idx = 0;
    while (bit_idx <= block_bitmap_last_bit) {
        buf[block_bitmap_last_byte] &= ~(1 << bit_idx++);
    }
    ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

    /***************************************
     * 3 将inode位图初始化并写入sb.inode_bitmap_lba *
     ***************************************/
    /* 先清空缓冲区*/
    memset(buf, 0, buf_size);
    buf[0] |= 0x1; // 第0个inode分给了根目录
    /* 由于inode_table中共4096个inode,位图inode_bitmap正好占用1扇区,
     * 即inode_bitmap_sects等于1, 所以位图中的位全都代表inode_table中的inode,
     * 无须再像block_bitmap那样单独处理最后一扇区的剩余部分,
     * inode_bitmap所在的扇区中没有多余的无效位 */
    ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);

    /***************************************
     * 4 将inode数组初始化并写入sb.inode_table_lba *
     ***************************************/
    /* 准备写inode_table中的第0项,即根目录所在的inode */
    memset(buf, 0, buf_size); // 先清空缓冲区buf
    struct inode *i = (struct inode *)buf;
    i->i_size = sb.dir_entry_size * 2; // .和..
    i->i_no = 0;                       // 根目录占inode数组中第0个inode
    i->i_sectors[0] =
        sb.data_start_lba; // 由于上面的memset,i_sectors数组的其它元素都初始化为0
    ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);

    /***************************************
     * 5 将根目录初始化并写入sb.data_start_lba
     ***************************************/
    /* 写入根目录的两个目录项.和.. */
    memset(buf, 0, buf_size);
    struct dir_entry *p_de = (struct dir_entry *)buf;

    /* 初始化当前目录"." */
    memcpy(p_de->filename, ".", 1);
    p_de->i_no = 0;
    p_de->f_type = FT_DIRECTORY;
    p_de++;

    /* 初始化当前目录父目录".." */
    memcpy(p_de->filename, "..", 2);
    p_de->i_no = 0; // 根目录的父目录依然是根目录自己
    p_de->f_type = FT_DIRECTORY;

    /* sb.data_start_lba已经分配给了根目录,里面是根目录的目录项 */
    ide_write(hd, sb.data_start_lba, buf, 1);

    printk("   root_dir_lba:0x%x\n", sb.data_start_lba);
    printk("%s format done\n", part->name);
    // if(!strcmp(part->name, "sdb1")){
    //     while(1);
    // }
    sys_free(buf);
}

/* 搜索文件pathname,若找到则返回其inode号,否则返回-1 */
static int search_file(const char *pathname,
                       struct path_search_record *searched_record) {
    /* 如果待查找的是根目录,为避免下面无用的查找,直接返回已知根目录信息 */
    if (!strcmp(pathname, "/") || !strcmp(pathname, "/.") ||
        !strcmp(pathname, "/..")) {
        searched_record->parent_dir = &root_dir;
        searched_record->file_type = FT_DIRECTORY;
        searched_record->searched_path[0] = 0; // 搜索路径置空
        return 0;
    }

    uint32_t path_len = strlen(pathname);
    /* 保证pathname至少是这样的路径/x且小于最大长度 */
    ASSERT(pathname[0] == '/' && path_len > 1 && path_len < MAX_PATH_LEN);
    char *sub_path = (char *)pathname;
    struct dir *parent_dir = &root_dir;
    struct dir_entry dir_e;

    /* 记录路径解析出来的各级名称,如路径"/a/b/c",
     * 数组name每次的值分别是"a","b","c" */
    char name[MAX_FILE_NAME_LEN] = {0};

    searched_record->parent_dir = parent_dir;
    searched_record->file_type = FT_UNKNOWN;
    uint32_t parent_inode_no = 0; // 父目录的inode号

    sub_path = path_parse(sub_path, name);
    // printk("in search_file, name: %s", name);
    while (name[0]) { // 若第一个字符就是结束符,结束循环
        /* 记录查找过的路径,但不能超过searched_path的长度512字节 */
        ASSERT(strlen(searched_record->searched_path) < 512);

        /* 记录已存在的父目录 */
        strcat(searched_record->searched_path, "/");
        strcat(searched_record->searched_path, name);
        // printk("in search_file searched_path: %s", searched_record->searched_path);
        /* 在所给的目录中查找文件 */
        if (search_dir_entry(cur_part, parent_dir, name, &dir_e)) {
            memset(name, 0, MAX_FILE_NAME_LEN);
            /* 若sub_path不等于NULL,也就是未结束时继续拆分路径 */
            if (sub_path) {
                sub_path = path_parse(sub_path, name);
            }

            if (FT_DIRECTORY == dir_e.f_type) { // 如果被打开的是目录
                parent_inode_no = parent_dir->inode->i_no;
                dir_close(parent_dir);
                parent_dir = dir_open(cur_part, dir_e.i_no); // 更新父目录
                searched_record->parent_dir = parent_dir;
                continue;
            } else if (FT_REGULAR == dir_e.f_type) { // 若是普通文件
                searched_record->file_type = FT_REGULAR;
                return dir_e.i_no;
            }
        } else { //若找不到,则返回-1
            /* 找不到目录项时,要留着parent_dir不要关闭,
             * 若是创建新文件的话需要在parent_dir中创建 */
            return -1;
        }
    }

    /* 执行到此,必然是遍历了完整路径并且查找的文件或目录只有同名目录存在 */
    dir_close(searched_record->parent_dir);

    /* 保存被查找目录的直接父目录 */
    searched_record->parent_dir = dir_open(cur_part, parent_inode_no);
    searched_record->file_type = FT_DIRECTORY;
    return dir_e.i_no;
}

/* 在磁盘上搜索文件系统,若没有则格式化分区创建文件系统 */
void filesys_init() {
    uint8_t channel_no = 0, dev_no, part_idx = 0;

    /* sb_buf用来存储从硬盘上读入的超级块 */
    struct super_block *sb_buf = (struct super_block *)sys_malloc(SECTOR_SIZE);

    if (sb_buf == NULL) {
        PANIC("alloc memory failed!");
    }
    printk("searching filesystem......\n");
    while (channel_no < channel_cnt) {
        dev_no = 0;
        while (dev_no < 2) {
            if (dev_no == 0) { // 跨过裸盘hd60M.img
                dev_no++;
                continue;
            }
            struct disk *hd = &channels[channel_no].devices[dev_no];
            struct partition *part = hd->prim_parts;
            while (part_idx < 12) {  // 4个主分区+8个逻辑
                if (part_idx == 4) { // 开始处理逻辑分区
                    part = hd->logic_parts;
                }

                /* channels数组是全局变量,默认值为0,disk属于其嵌套结构,
                 * partition又为disk的嵌套结构,因此partition中的成员默认也为0.
                 * 若partition未初始化,则partition中的成员仍为0.
                 * 下面处理存在的分区. */
                if (part->sec_cnt != 0) { // 如果分区存在
                    memset(sb_buf, 0, SECTOR_SIZE);

                    /* 读出分区的超级块,根据魔数是否正确来判断是否存在文件系统
                     */
                    ide_read(hd, part->start_lba + 1, sb_buf, 1);

                    /* 只支持自己的文件系统.若磁盘上已经有文件系统就不再格式化了
                     */
                    if (sb_buf->magic == 0x05211996) {
                        printk("%s has filesystem\n", part->name);
                    } else { // 其它文件系统不支持,一律按无文件系统处理
                        printk("formatting %s`s partition %s......\n", hd->name,
                               part->name);
                        partition_format(part);
                    }
                }
                part_idx++;
                part++; // 下一分区
            }
            dev_no++; // 下一磁盘
        }
        channel_no++; // 下一通道
    }
    sys_free(sb_buf);
    char default_part[8] = "sdb1";
    list_traversal(&partition_list, mount_partition, (int)default_part);

    open_root_dir(cur_part);

    uint32_t fd_idx = 0;
    while(fd_idx <  MAX_FILE_OPEN){
        file_table[fd_idx++].fd_inode = NULL;
    }
}

//解析路径最上层的路径名,如/a/b/c -> a, return /b/c
static char* path_parse(char* pathname, char* name_store){
    if(pathname[0] == '/'){
        while(*(++pathname) == '/');
    }
    while(*pathname != '/' && *pathname != 0){
        *name_store++ = *pathname++;
    }
    if(pathname[0] == 0)
        return NULL;
    return pathname;
}

int32_t path_depth_cnt(char* pathname){
    ASSERT(pathname != NULL);
    char* p = pathname;
    char name[MAX_FILE_NAME_LEN];
    uint32_t depth = 0;

    p = path_parse(p, name);
    while(name[0]){
        depth++;
        memset(name, 0, MAX_FILE_NAME_LEN);
        if(p){
            p = path_parse(p, name);
        }
    }
    return depth;
}

/* 打开或创建文件成功后,返回文件描述符,否则返回-1 */
int32_t sys_open(const char *pathname, uint8_t flags) {
    /* 对目录要用dir_open,这里只有open文件 */
    if (pathname[strlen(pathname) - 1] == '/') {
        printk("can`t open a directory %s\n", pathname);
        return -1;
    }
    ASSERT(flags <= 7);
    int32_t fd = -1; // 默认为找不到

    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));

    /* 记录目录深度.帮助判断中间某个目录不存在的情况 */
    uint32_t pathname_depth = path_depth_cnt((char *)pathname);

    /* 先检查文件是否存在 */
    int inode_no = search_file(pathname, &searched_record);
    // printk("\n searched_path: %s\n parent_dir_inode: %d\n file_type: %d\n",
    //        searched_record.searched_path,
    //        searched_record.parent_dir->inode->i_no, searched_record.file_type);
    bool found = inode_no != -1 ? true : false;

    if (searched_record.file_type == FT_DIRECTORY) {
        printk(
            "can`t open a direcotry with open(), use opendir() to instead\n");
        dir_close(searched_record.parent_dir);
        return -1;
    }

    uint32_t path_searched_depth =
        path_depth_cnt(searched_record.searched_path);
    // printk("searched_path: %s\n", searched_record.searched_path);
    /* 先判断是否把pathname的各层目录都访问到了,即是否在某个中间目录就失败了 */
    // printk("pathname_depth = %d, path_searched_depth = %d\n", pathname_depth, path_searched_depth);
    if (pathname_depth !=
        path_searched_depth) { // 说明并没有访问到全部的路径,某个中间目录是不存在的
        printk("cannot access %s: Not a directory, subpath %s is`t exist\n",
               pathname, searched_record.searched_path);
        dir_close(searched_record.parent_dir);
        return -1;
    }

    /* 若是在最后一个路径上没找到,并且并不是要创建文件,直接返回-1 */
    if (!found && !(flags & O_CREAT)) {
        printk("in path %s, file %s is`t exist\n",
               searched_record.searched_path,
               (strrchr(searched_record.searched_path, '/') + 1));
        dir_close(searched_record.parent_dir);
        return -1;
    } else if (found && flags & O_CREAT) { // 若要创建的文件已存在
        printk("%s has already exist!\n", pathname);
        dir_close(searched_record.parent_dir);
        return -1;
    }

    switch (flags & O_CREAT) {
    case O_CREAT:
        printk("creating file\n");
        fd = file_create(searched_record.parent_dir,
                         (strrchr(pathname, '/') + 1), flags);
        dir_close(searched_record.parent_dir);
        break;
        // 其余为打开已存在的文件
    default:
        fd = file_open(inode_no, flags);
    }

    /* 此fd是指任务pcb->fd_table数组中的元素下标,
     * 并不是指全局file_table中的下标 */
    return fd;
}

//把文件描述符转为文件表的下标
static uint32_t fd_loacl2global(uint32_t local_fd) {
    struct task_struct *cur_thread = running_thread();
    int32_t global_fd = cur_thread->fd_table[local_fd];
    ASSERT(global_fd >= 0 && global_fd < MAX_FILE_OPEN);
    return (uint32_t)global_fd;
}

int32_t sys_close(int32_t fd) {
    int32_t ret = -1;
    if (fd > 2) {
        uint32_t file_table_idx = fd_loacl2global(fd);
        ret = file_close(&file_table[file_table_idx]);
        running_thread()->fd_table[fd] = -1;
    }
    return ret;
}

int32_t sys_write(int32_t fd, const void *buf, uint32_t count) {
    if (fd < 0) {
        printk("sys_write: fd error\n");
        return -1;
    }
    if (fd == stdout_no) {
        char tmp_buf[1024] = {0};
        memcpy(tmp_buf, buf, count);
        console_put_str(tmp_buf);
        return count;
    }
    uint32_t _fd = fd_loacl2global(fd);
    struct file *wr_file = &file_table[_fd];
    if (wr_file->fd_flag & O_WRONLY || wr_file->fd_flag & O_RDWR) {
        uint32_t bytes_written = file_write(wr_file, buf, count);
        return bytes_written;
    } else {
        console_put_str("sys_write: not allowed to write file without flag "
                        "O_RDWR or O_WRONLY\n");
        return -1;
    }
}

int32_t sys_read(int32_t fd, void* buf, uint32_t count){
    if(fd < 0){
        printk("sys_read: fd error\n");
    }
    ASSERT(buf != NULL);
    uint32_t _fd = fd_loacl2global(fd);
    return file_read(&file_table[_fd], buf, count);
}

int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence){
    if(fd < 0){
        printk("sys_lseek: fd error\n");
        return -1;
    }
    ASSERT(whence > 0 && whence < 4);
    uint32_t _fd = fd_loacl2global(fd);
    struct file* pf = &file_table[_fd];
    int32_t new_pos = 0;
    int32_t file_size = (int32_t)pf->fd_inode->i_size;
    switch(whence){
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = (int32_t)pf->fd_pos + offset;
            break;
        case SEEK_END:
            new_pos = file_size + offset;   //此时offset应该为负值
            break;
    }
    if (new_pos < 0 || new_pos > file_size - 1){
        return -1;
    }
    pf->fd_pos = new_pos;
    return pf->fd_pos;
}

// (1)确认待创建的新目录在文件系统上不存在。
// (2)为新目录创建 inode。
// (3)为新目录分配 1 个块存储该目录中的目录项。
// (4)在新目录中创建两个目录项“.”和“..”,这是每个目录都必须存在的两个目录项。
// (5)在新目录的父目录中添加新目录的目录项。
// (6)将以上资源的变更同步到硬盘。

/* 删除文件(非目录),成功返回0,失败返回-1 */
int32_t sys_unlink(const char *pathname) {
    ASSERT(strlen(pathname) < MAX_PATH_LEN);

    /* 先检查待删除的文件是否存在 */
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = search_file(pathname, &searched_record);
    ASSERT(inode_no != 0);
    if (inode_no == -1) {
        printk("file %s not found!\n", pathname);
        dir_close(searched_record.parent_dir);
        return -1;
    }
    if (searched_record.file_type == FT_DIRECTORY) {
        printk(
            "can`t delete a direcotry with unlink(), use rmdir() to instead\n");
        dir_close(searched_record.parent_dir);
        return -1;
    }

    /* 检查是否在已打开文件列表(文件表)中 */
    uint32_t file_idx = 0;
    while (file_idx < MAX_FILE_OPEN) {
        if (file_table[file_idx].fd_inode != NULL &&
            (uint32_t)inode_no == file_table[file_idx].fd_inode->i_no) {
            break;
        }
        file_idx++;
    }
    if (file_idx < MAX_FILE_OPEN) {
        dir_close(searched_record.parent_dir);
        printk("file %s is in use, not allow to delete!\n", pathname);
        return -1;
    }
    ASSERT(file_idx == MAX_FILE_OPEN);

    /* 为delete_dir_entry申请缓冲区 */
    void *io_buf = sys_malloc(SECTOR_SIZE + SECTOR_SIZE);
    if (io_buf == NULL) {
        dir_close(searched_record.parent_dir);
        printk("sys_unlink: malloc for io_buf failed\n");
        return -1;
    }

    struct dir *parent_dir = searched_record.parent_dir;
    delete_dir_entry(cur_part, parent_dir, inode_no, io_buf);
    inode_release(cur_part, inode_no);
    sys_free(io_buf);
    dir_close(searched_record.parent_dir);
    return 0; // 成功删除文件
}

/* 创建目录pathname,成功返回0,失败返回-1 */
int32_t sys_mkdir(const char *pathname) {
    uint8_t rollback_step = 0; // 用于操作失败时回滚各资源状态
    void *io_buf = sys_malloc(SECTOR_SIZE * 2);
    if (io_buf == NULL) {
        printk("sys_mkdir: sys_malloc for io_buf failed\n");
        return -1;
    }

    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = -1;
    inode_no = search_file(pathname, &searched_record);
    if (inode_no != -1) { // 如果找到了同名目录或文件,失败返回
        printk("sys_mkdir: file or directory %s exist!\n", pathname);
        rollback_step = 1;
        goto rollback;
    } else { // 若未找到,也要判断是在最终目录没找到还是某个中间目录不存在
        uint32_t pathname_depth = path_depth_cnt((char *)pathname);
        uint32_t path_searched_depth =
            path_depth_cnt(searched_record.searched_path);
        /* 先判断是否把pathname的各层目录都访问到了,即是否在某个中间目录就失败了
         */
        if (pathname_depth !=
            path_searched_depth) { // 说明并没有访问到全部的路径,某个中间目录是不存在的
            printk("sys_mkdir: can`t access %s, subpath %s is`t exist\n",
                   pathname, searched_record.searched_path);
            rollback_step = 1;
            goto rollback;
        }
    }

    struct dir *parent_dir = searched_record.parent_dir;
    /* 目录名称后可能会有字符'/',所以最好直接用searched_record.searched_path,无'/'
     */
    char *dirname = strrchr(searched_record.searched_path, '/') + 1;

    inode_no = inode_bitmap_alloc(cur_part);
    if (inode_no == -1) {
        printk("sys_mkdir: allocate inode failed\n");
        rollback_step = 1;
        goto rollback;
    }

    struct inode new_dir_inode;
    inode_init(inode_no, &new_dir_inode); // 初始化i结点

    uint32_t block_bitmap_idx = 0; // 用来记录block对应于block_bitmap中的索引
    int32_t block_lba = -1;
    /* 为目录分配一个块,用来写入目录.和.. */
    block_lba = block_bitmap_alloc(cur_part);
    if (block_lba == -1) {
        printk("sys_mkdir: block_bitmap_alloc for create directory failed\n");
        rollback_step = 2;
        goto rollback;
    }
    new_dir_inode.i_sectors[0] = block_lba;
    /* 每分配一个块就将位图同步到硬盘 */
    block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
    ASSERT(block_bitmap_idx != 0);
    bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);

    /* 将当前目录的目录项'.'和'..'写入目录 */
    memset(io_buf, 0, SECTOR_SIZE * 2); // 清空io_buf
    struct dir_entry *p_de = (struct dir_entry *)io_buf;

    /* 初始化当前目录"." */
    memcpy(p_de->filename, ".", 1);
    p_de->i_no = inode_no;
    p_de->f_type = FT_DIRECTORY;

    p_de++;
    /* 初始化当前目录".." */
    memcpy(p_de->filename, "..", 2);
    p_de->i_no = parent_dir->inode->i_no;
    p_de->f_type = FT_DIRECTORY;
    ide_write(cur_part->my_disk, new_dir_inode.i_sectors[0], io_buf, 1);

    new_dir_inode.i_size = 2 * cur_part->sb->dir_entry_size;

    /* 在父目录中添加自己的目录项 */
    struct dir_entry new_dir_entry;
    memset(&new_dir_entry, 0, sizeof(struct dir_entry));
    create_dir_entry(dirname, inode_no, FT_DIRECTORY, &new_dir_entry);
    memset(io_buf, 0, SECTOR_SIZE * 2); // 清空io_buf
    if (!sync_dir_entry(
            parent_dir, &new_dir_entry,
            io_buf)) { // sync_dir_entry中将block_bitmap通过bitmap_sync同步到硬盘
        printk("sys_mkdir: sync_dir_entry to disk failed!\n");
        rollback_step = 2;
        goto rollback;
    }

    /* 父目录的inode同步到硬盘 */
    memset(io_buf, 0, SECTOR_SIZE * 2);
    inode_sync(cur_part, parent_dir->inode, io_buf);

    /* 将新创建目录的inode同步到硬盘 */
    memset(io_buf, 0, SECTOR_SIZE * 2);
    inode_sync(cur_part, &new_dir_inode, io_buf);

    /* 将inode位图同步到硬盘 */
    bitmap_sync(cur_part, inode_no, INODE_BITMAP);

    sys_free(io_buf);

    /* 关闭所创建目录的父目录 */
    dir_close(searched_record.parent_dir);
    return 0;

/*创建文件或目录需要创建相关的多个资源,若某步失败则会执行到下面的回滚步骤 */
rollback: // 因为某步骤操作失败而回滚
    switch (rollback_step) {
    case 2:
        bitmap_set(
            &cur_part->inode_bitmap, inode_no,
            0); // 如果新文件的inode创建失败,之前位图中分配的inode_no也要恢复
    case 1:
        /* 关闭所创建目录的父目录 */
        dir_close(searched_record.parent_dir);
        break;
    }
    sys_free(io_buf);
    return -1;
}

/* 目录打开成功后返回目录指针,失败返回NULL */
struct dir *sys_opendir(const char *name) {
    ASSERT(strlen(name) < MAX_PATH_LEN);
    /* 如果是根目录'/',直接返回&root_dir */
    if (name[0] == '/' && (name[1] == 0 || name[0] == '.')) {
        return &root_dir;
    }

    /* 先检查待打开的目录是否存在 */
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = search_file(name, &searched_record);
    struct dir *ret = NULL;
    if (inode_no == -1) { // 如果找不到目录,提示不存在的路径
        printk("In %s, sub path %s not exist\n", name,
               searched_record.searched_path);
    } else {
        if (searched_record.file_type == FT_REGULAR) {
            printk("%s is regular file!\n", name);
        } else if (searched_record.file_type == FT_DIRECTORY) {
            ret = dir_open(cur_part, inode_no);
        }
    }
    dir_close(searched_record.parent_dir);
    return ret;
}

/* 成功关闭目录dir返回0,失败返回-1 */
int32_t sys_closedir(struct dir *dir) {
    int32_t ret = -1;
    if (dir != NULL) {
        dir_close(dir);
        ret = 0;
    }
    return ret;
}

//读取目录的一个目录项，返回目录项地址，失败返回null
struct dir_entry *sys_readdir(struct dir *dir) {
    ASSERT(dir != NULL);
    return dir_read(dir);
}

//把dir_pos置为0
void sys_rewinddir(struct dir *dir) { dir->dir_pos = 0; }

//删除空目录，返回0,失败返回-1
int32_t sys_rmdir(const char* pathname) {
   /* 先检查待删除的文件是否存在 */
   struct path_search_record searched_record;
   memset(&searched_record, 0, sizeof(struct path_search_record));
   int inode_no = search_file(pathname, &searched_record);
   ASSERT(inode_no != 0);
   int retval = -1;	// 默认返回值
   printk("pathname: %s , inode_no: %d\n", pathname, inode_no);
   if (inode_no == -1) {
      printk("In %s, sub path %s not exist\n", pathname, searched_record.searched_path); 
   } else {
      if (searched_record.file_type == FT_REGULAR) {
	 printk("%s is regular file!\n", pathname);
      } else { 
	 struct dir* dir = dir_open(cur_part, inode_no);
	 if (!dir_is_empty(dir)) {	 // 非空目录不可删除
	    printk("dir %s is not empty, it is not allowed to delete a nonempty directory!\n", pathname);
	 } else {
	    if (!dir_remove(searched_record.parent_dir, dir)) {
	       retval = 0;
	    }
	 }
	 dir_close(dir);
      }
   }
   dir_close(searched_record.parent_dir);
   return retval;
}