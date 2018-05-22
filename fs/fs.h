#ifndef __FS_FS_H
#define __FS_FS_H
#include "stdint.h"
#include "ide.h"

#define MAX_FILES_PER_PART 4096	    // 每个分区所支持最大创建的文件数
#define BITS_PER_SECTOR 4096	    // 每扇区的位数
#define SECTOR_SIZE 512		    // 扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE	    // 块字节大小
#define MAX_PATH_LEN 512
#define FILE_FLAGS_CHEACK 7
/* 文件类型 */
enum file_types {
   FT_UNKNOWN,	  // 不支持的文件类型
   FT_REGULAR,	  // 普通文件
   FT_DIRECTORY	  // 目录
};

enum oflags{
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT = 4
};

enum whence {
    SEEK_SET = 1, //offset的参照物是文件开始处
    SEEK_CUR,      //当前位置
    SEEK_END        //文件结束处的下一字节
};

struct path_search_record{
    char searched_path[MAX_PATH_LEN]; //查找过的路径
    struct dir* parent_dir;     //
    enum file_types file_type;  //文件or目录
};

void filesys_init(void);
int32_t path_depth_cnt(char* pathname);
int32_t sys_open(const char *pathname, uint8_t flags);
int32_t sys_close(int32_t fd);
uint32_t sys_write(int32_t fd, const void *buf, uint32_t count);
int32_t sys_read(int32_t fd, void* buf, uint32_t count);
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t sys_unlink(const char *pathname);
int32_t sys_mkdir(const char *pathname);
struct dir *sys_opendir(const char *name);
extern struct partition *cur_part;
#endif
