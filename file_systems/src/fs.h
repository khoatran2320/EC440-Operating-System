#ifndef INCLUDE_FS_H
#define INCLUDE_FS_H
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_NUM_FILES 64            //maximum number of files supported 
#define MAX_NUM_FD 32               //maximum number of supported file descriptors per file
#define MAX_FILENAME_LEN 15         //maximum supported length of file name
#define MAX_FILE_SIZE 1<<20         //maximum supported file size in bytes (1MiB = 2^20 bytes)
#define NUM_DIRECT_BLOCKS 25        //number of direct blocks in an inode
/* Disk Layout */
//|-------------------------------------------------------------------------------------------------|
//|             |           |                       |                       |           |           |
//| SUPERBLOCK  |   DENTRY  |   FREE INODES BITMAP  |   FREE BLOCKS BITMAP  |   INODES  |   DATA    |
//|             |           |                       |                       |           |           |
//|-------------------------------------------------------------------------------------------------|


/* data structures */
struct superblock {
    uint16_t data_block_offset;
    uint16_t block_bitmap_size;
    uint16_t block_bitmap_offset;
    uint16_t inode_bitmap_size;
    uint16_t inode_bitmap_offset;
    uint16_t inode_size;
    uint16_t inode_offset;
    uint16_t dir_entry_offset;
    uint16_t dir_entry_size;
};

struct inode {
    uint16_t inode_number;
    uint8_t file_type;              // 0 for regular file, 1 for directory
    uint32_t file_size;             // file size in bytes
    uint16_t direct_block[NUM_DIRECT_BLOCKS];
    uint16_t single_indirect_block;
};

struct dir_entry {
    uint8_t is_used;
    uint8_t name[MAX_FILENAME_LEN+1];
    uint16_t inode_number;
};

struct f_desc {
    bool is_used;
    uint16_t inode_number;
    int offset;
};

// bool get_bit(unsigned short offset);
// void set_bit(unsigned short offset);
// void clear_bit(unsigned short offset);

/* file system APIs */
int make_fs(const char *disk_name);
int mount_fs(const char *disk_name);
int umount_fs(const char *disk_name);
int fs_open(const char *name);
int fs_close(int fildes);
int fs_create(const char *name);
int fs_delete(const char *name);
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);
int fs_get_filesize(int fildes);
int fs_listfiles(char ***files);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);
#endif /* INCLUDE_FS_H */
