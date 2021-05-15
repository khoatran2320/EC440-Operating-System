# File Systems
#### By Khoa Tran 
## EC440: Introduction to Operating Systems Spring 2021

## Introduction
This library provides a file system that runs on a virtual disk. The virtual disk is a single file provided by the native Linux file system. The file system in this library uses an inode mechanism similar to that implemented in Linux. The disk is limited to 8192 blocks of 4096 bytes each. The specifications are:
-   There is only one directory
-   File names are limited to 15 characters
-   The maximum number of concurrently-existing files is 64
-   The maximum number of concurrently-existing file descriptors is 32
-   The maximum file size is 1MiB

The design choices are:
-   Each inode has 25 direct blocks
-   Each inode has a single indirect block
-   An inode's ID is its index in the inode table
-   Bitmaps are maintained to keep track of available inodes and blocks

### Disk Layout
|-------------------------------------------------------------------------------------------|<br />
| SUPERBLOCK | DENTRY | FREE INODES BITMAP | FREE BLOCKS BITMAP | INODES | DATA |<br />
|-------------------------------------------------------------------------------------------|<br />


## Supported Functions
-   int make_fs(const char *disk_name): make a file system for use. 
-   int mount_fs(const char *disk_name): mounts a file system. Loads everything from disk and readies the file system for use. 
-   int umount_fs(const char *disk_name): unmounts a file system. Write everything back to disk for persistent storage. 
-   int fs_open(const char *name): open a file. 
-   int fs_close(int fildes): close a file.
-   int fs_create(const char *name): create a file.
-   int fs_delete(const char *name): delete a file. 
-   int fs_read(int fildes, void *buf, size_t nbyte): read from a file nbyte bytes starting from the file offset. 
-   int fs_write(int fildes, void *buf, size_t nbyte): write from a file nbyte bytes starting from the file offset.
-   int fs_get_filesize(int fildes): returns the file size.
-   int fs_listfiles(char ***files): list all the current file names. 
-   int fs_lseek(int fildes, off_t offset): moves the file offset.
-   int fs_truncate(int fildes, off_t length): reduces the size of the file. 

## Credits

-   Linux man pages for general memory-related functions references. 
-   Lecture notes (especially the homework discussion lecture).
-   Class Piazza posts. 
-   Bit manipulation: https://stackoverflow.com/questions/2249731/how-do-i-get-bit-by-bit-data-from-an-integer-value-in-c

