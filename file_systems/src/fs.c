#include "fs.h"
#include "disk.h"

/* global variables */
struct superblock sb;
struct f_desc fd_table[MAX_NUM_FD];
struct inode inode_table[MAX_NUM_FILES];
struct dir_entry dir_table[MAX_NUM_FILES];
uint8_t blocks_bitmap[1024] = {0};
uint8_t inodes_bitmap[8] = {0};
static bool is_fs_mounted = false;

static uint8_t get_bit(unsigned short offset, uint8_t *bitmap)
{
    unsigned short bitmap_offset = offset / 8;
    unsigned char char_offset = offset % 8;
    unsigned char char_in_bitmap = bitmap[bitmap_offset];
    return (char_in_bitmap >> char_offset) & 1U;
}

static void set_bit(unsigned short offset, uint8_t *bitmap)
{
    unsigned short bitmap_offset = offset / 8;
    unsigned char char_offset = offset % 8;
    unsigned char mask = 1<<char_offset;
    bitmap[bitmap_offset] |= mask;
    return;
}

static void clear_bit(unsigned short offset, uint8_t *bitmap)
{
    unsigned short bitmap_offset = offset / 8;
    unsigned char char_offset = offset % 8;
    unsigned char mask = ~(1<<char_offset);
    bitmap[bitmap_offset] &= mask;
    return;
}
static unsigned short find_free_bit(uint8_t *bitmap){
    for(unsigned short i = 0; i < DISK_BLOCKS; ++i){
        if(!get_bit(i, bitmap)){
            return i;
        }
    }
    return 0;
}

int make_fs(const char *disk_name)
{   
    if(make_disk(disk_name)){
        perror("make_fs: error calling make_disk()");
        return -1;
    }
    if(open_disk(disk_name)){
        perror("make_fs: error calling open_disk()");
        return -1;
    }
    
    /* Disk Layout */
    //|-------------------------------------------------------------------------------------------------|
    //|             |           |                       |                       |           |           /
    //| SUPERBLOCK  |   DENTRY  |   FREE INODES BITMAP  |   FREE BLOCKS BITMAP  |   INODES  |   DATA    /
    //|             |           |                       |                       |           |           /
    //|-------------------------------------------------------------------------------------------------|

    /* set up superblock */
    struct superblock super_b;
    super_b.dir_entry_offset    =   1;                                                                  // directory entries are right after the superblock
    super_b.dir_entry_size      =   (sizeof(struct dir_entry) * MAX_NUM_FILES) % BLOCK_SIZE == 0 ? \
                                    (sizeof(struct dir_entry) * MAX_NUM_FILES) / BLOCK_SIZE : \
                                    (sizeof(struct dir_entry) * MAX_NUM_FILES) / BLOCK_SIZE + 1;        // 20 * 64 / 4096 + 1 = 1
    
    super_b.inode_bitmap_offset =   super_b.dir_entry_offset + super_b.dir_entry_size;                  // 1 + 1 = 2
   
    super_b.inode_bitmap_size   =   sizeof(inodes_bitmap) % BLOCK_SIZE == 0 ? \
                                    sizeof(inodes_bitmap) / BLOCK_SIZE : \
                                    sizeof(inodes_bitmap) / BLOCK_SIZE + 1;                             // 16 / 4096 + 1 = 1
    
    super_b.block_bitmap_offset =   super_b.inode_bitmap_offset + super_b.inode_bitmap_size;            // 2 + 1 = 3
    
    super_b.block_bitmap_size   =   sizeof(blocks_bitmap) % BLOCK_SIZE == 0 ? \
                                    sizeof(blocks_bitmap) / BLOCK_SIZE : \
                                    sizeof(blocks_bitmap) / BLOCK_SIZE + 1;                             // 1024 / 4096 + 1 = 1
    
    super_b.inode_offset        =   super_b.block_bitmap_offset + super_b.block_bitmap_size;            // 3 + 1 = 4
    
    super_b.inode_size          =   (sizeof(struct inode) * MAX_NUM_FILES) % BLOCK_SIZE == 0 ? \
                                    (sizeof(struct inode) * MAX_NUM_FILES) / BLOCK_SIZE : \
                                    (sizeof(struct inode) * MAX_NUM_FILES) / BLOCK_SIZE + 1;            // 64 * 64 / 4096  = 1
    
    super_b.data_block_offset = super_b.inode_offset+ super_b.inode_size;                               // 4 + 1 = 5

    /* write superblock to disk */
    uint8_t *block = calloc(BLOCK_SIZE, 1);
    memcpy(block, &super_b, sizeof(super_b));
    if (block_write(0, block)){
        perror("make_fs: unable to write superblock to disk");
        return -1;
    }

    /* write emptry directory entries to disk */
    struct dir_entry empty_dentry = {.is_used = 0, .inode_number = 0, .name[0] = '\0'};
    memset(block, 0, sizeof(BLOCK_SIZE));
    for(int i = 0; i < MAX_NUM_FILES; ++i){
        memcpy(block + i*sizeof(struct dir_entry), &empty_dentry, sizeof(struct dir_entry));
    }
    if(block_write(super_b.dir_entry_offset, block)){
        perror("make_fs: unable to write empty dentries to disk");
        return -1;
    }

    /* write inodes bitmap to disk */
    memset(block, 0, sizeof(BLOCK_SIZE));
    if(block_write(super_b.inode_bitmap_offset, block)){
        perror("make_fs: unable to write inode bitmap to disk");
        return -1;
    }

    /* write blocks bitmap to disk */
    memset(block, 0, sizeof(BLOCK_SIZE));
    for(unsigned short i = 0; i < super_b.data_block_offset; ++i){
        set_bit(i, blocks_bitmap);
    }
    memcpy(block, blocks_bitmap, sizeof(blocks_bitmap));
    if(block_write(super_b.block_bitmap_offset, block)){
        perror("make_fs: unable to write block bitmap to disk");
        return -1;
    }

    /* write empty inodes to disk */
    memset(block, 0, sizeof(BLOCK_SIZE));
    for(int i = 0; i < MAX_NUM_FILES; ++i){
        struct inode in = {.direct_block = {0}, .file_size = 0, .file_type = 0, .inode_number = i, .single_indirect_block = 0};
        memcpy(block + i*sizeof(struct inode), &in, sizeof(struct inode));
    }
    if(block_write(super_b.inode_offset, block)){
        perror("make_fs: unable to write empty inodes to disk");
        return -1;
    }
    free(block);
    
    return close_disk();
}
int mount_fs(const char *disk_name)
{
    if(is_fs_mounted){
        perror("mount_fs: fs already mounted");
        return -1;
    }
    if(open_disk(disk_name)){
        perror("mount_fs: error calling open_disk()");
        return -1;
    }

    /* mount superblock */
    char *block = calloc(BLOCK_SIZE, 1);
    if(block_read(0, block)){
        perror("mount_fs: unable to read superblock");
        return -1;
    }
    memcpy(&sb, block, sizeof(struct superblock));

    /* mount directory entries */
    memset(block, 0, BLOCK_SIZE);
    if(block_read(sb.dir_entry_offset, block)){
        perror("mount_fs: unable to read directory entries");
        return -1;
    }
    memcpy(dir_table, block, sizeof(dir_table));

    /* mount inodes bitmap */
    memset(block, 0, BLOCK_SIZE);
    if(block_read(sb.inode_bitmap_offset, block)){
        perror("mount_fs: unable to read inode bitmap");
        return -1;
    }
    memcpy(inodes_bitmap, block, sizeof(inodes_bitmap));

    /* mount blocks bitmap */
    memset(block, 0, BLOCK_SIZE);
    if(block_read(sb.block_bitmap_offset, block)){
        perror("mount_fs: unable to read blocks bitmap");
        return -1;
    }
    memcpy(blocks_bitmap, block, sizeof(blocks_bitmap));

    /* mount inodes */
    memset(block, 0, BLOCK_SIZE);
    if(block_read(sb.inode_offset, block)){
        perror("mount_fs: unable to read inodes");
        return -1;
    }
    memcpy(inode_table, block, sizeof(inode_table));

    /* initialize fd table */
    for(int i = 0; i < MAX_NUM_FD; ++i){
        fd_table[i] = (struct f_desc){.is_used = false, .inode_number = 0, .offset = 0};
    }

    is_fs_mounted = true;
    free(block);
    return 0;
}
int umount_fs(const char *disk_name)
{
    /* write superblock to disk */
    uint8_t *block = calloc(BLOCK_SIZE, 1);
    memcpy(block, &sb, sizeof(sb));
    if (block_write(0, block)){
        perror("unmount_fs: unable to write superblock to disk");
        return -1;
    }

    /* write directory entries to disk */
    memset(block, 0, sizeof(BLOCK_SIZE));
    for(int i = 0; i < MAX_NUM_FILES; ++i){
        memcpy(block + i*sizeof(struct dir_entry), &dir_table[i], sizeof(struct dir_entry));
    }
    if(block_write(sb.dir_entry_offset, block)){
        perror("unmount_fs: unable to write dentries to disk");
        return -1;
    }

    /* write inodes bitmap to disk */
    memset(block, 0, sizeof(BLOCK_SIZE));
    memcpy(block, inodes_bitmap, sizeof(inodes_bitmap));
    if(block_write(sb.inode_bitmap_offset, block)){
        perror("unmount_fs: unable to write inode bitmap to disk");
        return -1;
    }

    /* write blocks bitmap to disk */
    memset(block, 0, sizeof(BLOCK_SIZE));
    memcpy(block, blocks_bitmap, sizeof(blocks_bitmap));
    if(block_write(sb.block_bitmap_offset, block)){
        perror("unmount_fs: unable to write block bitmap to disk");
        return -1;
    }

    /* write inodes to disk */
    memset(block, 0, sizeof(BLOCK_SIZE));
    for(int i = 0; i < MAX_NUM_FILES; ++i){
        memcpy(block + i*sizeof(struct inode), &inode_table[i], sizeof(struct inode));
    }
    if(block_write(sb.inode_offset, block)){
        perror("unmount_fs: unable to write inodes to disk");
        return -1;
    }

    if(close_disk()){
        perror("unmount_fs: unable to close disk");
        return -1;
    }
    free(block);
    return 0;
}
int fs_open(const char *name)
{   
    int fd_index, i;
    bool is_name_found = false;

    /* check for valid name */
    for(i = 0; i < MAX_NUM_FILES; ++i){
        if(strcmp((const char *)dir_table[i].name, name) == 0){
            is_name_found = true;
            break;
        }
    }
    if(!is_name_found){
        perror("fs_open: name not found");
        return -1;
    }

    /* find free fd */
    for(fd_index = 0; fd_index < MAX_NUM_FD; ++fd_index){
        if(fd_table[fd_index].is_used == 0){
            break;
        }
    }

    /* check for fd limit */
    if(fd_index == MAX_NUM_FD){
        perror("fs_open: maximum number of fds in use");
        return -1;
    }

    /* initialize fd */
    fd_table[fd_index] = (struct f_desc){.inode_number = dir_table[i].inode_number, .is_used = true, .offset = 0};

    return fd_index;
}
int fs_close(int fildes)
{
    /* check for valid fd */
    if(fildes < 0 || fildes > MAX_NUM_FD - 1){
        perror("fs_close: invalid file descriptor");
        return -1;
    }

    /* check if fd is in use */
    if(!fd_table[fildes].is_used){
        perror("fs_close: file descriptor not in use");
        return -1;
    }

    /* reset fd entry */
    fd_table[fildes] = (struct f_desc){.inode_number = 0, .is_used = false, .offset = 0};
    return 0;
}
int fs_create(const char *name)
{
    /* check for name size */
    if(sizeof(name) > MAX_FILENAME_LEN){
        perror("fs_create: name is longer than 16 characters");
        return -1;
    }

    /* check for name duplicates and directory capacity */
    bool is_full = true;
    for(int i = 0; i < MAX_NUM_FILES; ++i){
        if(strcmp(name, (const char *)dir_table[i].name) == 0){
            perror("fs_create: file name already exists");
            return -1;
        }
        if(dir_table[i].is_used == 0){
            is_full = false;
        }
    }
    if(is_full){
        perror("fs_create: directory is full");
        return -1;
    }

    /* create directory entry and map to free inode*/
    for(unsigned short i = 0; i < MAX_NUM_FILES; ++i){
        if(!get_bit(i, inodes_bitmap)){
            dir_table[i] = (struct dir_entry){.inode_number = inode_table[i].inode_number, .is_used = 1U};
            strcpy((char *)dir_table[i].name, name);
            set_bit(i, inodes_bitmap);
            break;
        }
    }
    return 0;
}
int fs_delete(const char *name)
{
    //file name must exist
    //file must not be open
    //delete directory entry
    //unmap directory entry to inode
    //unset inode map use
    //unset data blocks used

    /* find name in directory entries table */
    int dentry_idx;
    bool is_name_found = false;
    for(dentry_idx = 0; dentry_idx < MAX_NUM_FILES; ++dentry_idx){
        if(strcmp((const char *)dir_table[dentry_idx].name, name) == 0){
            is_name_found = true;
            break;
        }
    }
    if(!is_name_found){
        perror("fs_delete: file name does not exist");
        return -1;
    }

    /* check if file is in use */
    for(int i = 0; i < MAX_NUM_FD; ++i){
        if(fd_table[i].is_used){
            perror("fs_delete: file is in use");
            return -1;
        }
    }

    /* check valid inode number */
    uint16_t inode_idx = dir_table[dentry_idx].inode_number;
    if(inode_idx < 0 || inode_idx > MAX_NUM_FILES - 1){
        perror("fs_delete: invalid inode number");
        return -1;
    }

    /* clear the direct blocks (clean the blocks bitmap for those blocks) */
    for(unsigned short i = 0; i < NUM_DIRECT_BLOCKS; ++i){
        if(inode_table[inode_idx].direct_block[i]){
            clear_bit(inode_table[inode_idx].direct_block[i], blocks_bitmap);
        }
    }

    /* clear the blocks referenced by the single indirect block */
    if(inode_table[inode_idx].single_indirect_block && inode_table[inode_idx].single_indirect_block < DISK_BLOCKS){
        uint16_t * indirect_block = calloc(BLOCK_SIZE, 1);
        if(block_read(inode_table[inode_idx].single_indirect_block, indirect_block)){
            perror("fs_delete: unable to read indirect block");
            return -1;
        }
        int num_block_entries = 2048;
        for (int i = 0; i < num_block_entries; ++i){
            if(*(indirect_block + i)){
                clear_bit(*(indirect_block + i), blocks_bitmap);
            }
        }
        free(indirect_block);
    }

    /* clear the inode entry */
    for(int i = 0; i < NUM_DIRECT_BLOCKS; ++i){
        inode_table[inode_idx].direct_block[i] = 0;
    }
    inode_table[inode_idx].single_indirect_block = 0;
    // inode_table[inode_idx].double_indirect_block = 0;
    inode_table[inode_idx].file_size = 0;
    inode_table[inode_idx].file_type = 0;

    /* clear the inodes bitmap */
    clear_bit(inode_idx, inodes_bitmap);

    /* clear the directory entry and unmap from inode */
    dir_table[dentry_idx] = (struct dir_entry){.inode_number = 0, .is_used = 0, .name[0] = '\0'};
    
    return 0;
}

int fs_read(int fildes, void *buf, size_t nbyte)
{
    // printf("read %ld bytes at cursor %d\n", nbyte, fd_table[fildes].offset);
    if(fildes < 0 || fildes > MAX_NUM_FD - 1){
        perror("fs_read: invalid fd");
        return -1;
    }
    if(!fd_table[fildes].is_used){
        perror("fs_read: fd not in use");
        return -1;
    }

    uint16_t inode_idx = fd_table[fildes].inode_number;
    nbyte = nbyte > (MAX_FILE_SIZE) ? (MAX_FILE_SIZE) : nbyte;
    int cursor = fd_table[fildes].offset;
    size_t bytes_read = 0;

    int starting_block = cursor / BLOCK_SIZE;
    
    bool first_block = true;

    size_t file_size = inode_table[inode_idx].file_size;

    if(starting_block < NUM_DIRECT_BLOCKS){
        for(int i = starting_block; i < NUM_DIRECT_BLOCKS && nbyte - bytes_read > 0 && file_size > bytes_read + cursor; ++i){
            if(inode_table[inode_idx].direct_block[i]){
                uint8_t *read_block = (uint8_t *)calloc(BLOCK_SIZE, 1);
                if(block_read(inode_table[inode_idx].direct_block[i], read_block)){
                    perror("fs_read: unable to read direct block");
                    return -1;
                }
                if(first_block){
                    first_block = false;
                    size_t read_size = nbyte > BLOCK_SIZE - cursor % BLOCK_SIZE ? BLOCK_SIZE - cursor % BLOCK_SIZE : nbyte;
                    read_size = file_size - cursor > read_size ? read_size : file_size - cursor;
                    memcpy((uint8_t*)buf, read_block + cursor % BLOCK_SIZE, read_size);
                    bytes_read += read_size;
                    continue;
                }
                if(nbyte - bytes_read < BLOCK_SIZE){
                    memcpy((uint8_t*)buf+bytes_read, read_block, nbyte-bytes_read);
                    bytes_read = nbyte;
                }
                else{
                    memcpy((uint8_t*)buf + bytes_read, read_block, BLOCK_SIZE);
                    bytes_read += BLOCK_SIZE;
                }
                free(read_block);
            }
        }
    }

    if(nbyte - bytes_read > 0 && inode_table[inode_idx].single_indirect_block && file_size > bytes_read + cursor){
        uint16_t * ind_block = calloc(BLOCK_SIZE, 1);
        if(block_read(inode_table[inode_idx].single_indirect_block, ind_block)){
            perror("fs_read: unable to read indirect block");
            return -1;
        }
        unsigned short blocks_needed = (nbyte - bytes_read) % BLOCK_SIZE == 0 ? (nbyte - bytes_read) / BLOCK_SIZE : (nbyte - bytes_read) / BLOCK_SIZE + 1;
        for(int i = starting_block < NUM_DIRECT_BLOCKS ? 0 : starting_block ; i < blocks_needed; ++i){
            if(*(ind_block+i)){
                uint8_t *read_block = calloc(BLOCK_SIZE, 1);
                if(block_read(*(ind_block+i), read_block)){
                    perror("fs_read: unable to read direct block");
                    return -1;
                }
                if(first_block){
                    first_block = false;
                    size_t read_size = nbyte > BLOCK_SIZE - cursor % BLOCK_SIZE ? BLOCK_SIZE - cursor % BLOCK_SIZE : nbyte;
                    read_size = file_size - cursor > read_size ? read_size : file_size - cursor;
                    memcpy((uint8_t*)buf, read_block+cursor % BLOCK_SIZE, read_size);
                    bytes_read += read_size;
                    continue;
                }
                if(nbyte - bytes_read < BLOCK_SIZE){
                    memcpy((uint8_t*)buf+bytes_read, read_block, nbyte-bytes_read);
                    bytes_read = nbyte;
                }
                else{
                    memcpy((uint8_t*)buf + bytes_read, read_block, BLOCK_SIZE);
                    bytes_read += BLOCK_SIZE;
                }
                free(read_block);
            }
        }
    }
    cursor += bytes_read;
    fd_table[fildes].offset = cursor;
    return bytes_read;
}

int fs_write(int fildes, void *buf, size_t nbyte)
{
    /* check valid fd */
    if(fildes < 0 || fildes > MAX_NUM_FD - 1){
        perror("fs_write: invalid fd");
        return -1;
    }

    /* check fd in use */
    if(!fd_table[fildes].is_used){
        perror("fs_write: fd not in use");
        return -1;
    }

    /* check valid inode */
    uint16_t inode_idx = fd_table[fildes].inode_number;
    if(inode_idx < 0 || inode_idx > MAX_NUM_FILES - 1){
        perror("fs_delete: invalid inode number");
        return -1;
    }

    int cursor = fd_table[fildes].offset;
    nbyte = (cursor+ nbyte) > (MAX_FILE_SIZE )? ((MAX_FILE_SIZE )- cursor) : nbyte;
    size_t bytes_written = 0;
    int current_ind_block = cursor / BLOCK_SIZE;

    bool first_block = true;

    /* write to direct blocks */
    while(bytes_written < nbyte && current_ind_block < NUM_DIRECT_BLOCKS){
        if(first_block){
            first_block = false;
            if(cursor % BLOCK_SIZE == 0){}
            else{
                uint8_t * block = calloc(BLOCK_SIZE, 1);
                if(block_read(inode_table[inode_idx].direct_block[current_ind_block], block)){
                    perror("fs_write: unable to read first block");
                    return -1;
                }
                int write_size = nbyte > (BLOCK_SIZE - cursor % BLOCK_SIZE) ? BLOCK_SIZE - cursor % BLOCK_SIZE : nbyte;
                memcpy(block + cursor % BLOCK_SIZE, (uint8_t*)buf, write_size);
                if(block_write(inode_table[inode_idx].direct_block[current_ind_block], block)){
                    perror("fs_write: unable to write first block");
                    return -1;
                }
                bytes_written += write_size;
                ++current_ind_block;
                continue;
            }
        }
        
        unsigned short free_block;
        if(!inode_table[inode_idx].direct_block[current_ind_block]){
            free_block = find_free_bit(blocks_bitmap);
            if(!free_block){
                cursor += bytes_written;
                inode_table[inode_idx].file_size = cursor > inode_table[inode_idx].file_size ? cursor : inode_table[inode_idx].file_size;
                fd_table[fildes].offset = cursor;
                return bytes_written;
            }
            set_bit(free_block, blocks_bitmap);
            inode_table[inode_idx].direct_block[current_ind_block] = free_block;
        }
        else{
            free_block = inode_table[inode_idx].direct_block[current_ind_block];
        }
        // data to be written is larger than block size so we write the whole block
        if(nbyte - bytes_written >= BLOCK_SIZE){
            if(block_write(free_block, (uint8_t *)buf + bytes_written)){
                perror("fs_write: unable to write block to disk");
                return -1;
            }
            bytes_written += BLOCK_SIZE;
        }

        // data to be written is smaller than block size so we pad 0
        else{
            uint8_t *block = (uint8_t *)calloc(BLOCK_SIZE, 1);
            memcpy(block, (uint8_t *)buf + bytes_written, nbyte - bytes_written);
            if(block_write(free_block, block)){
                perror("fs_write: unable to write block to disk");
                return -1;
            }
            bytes_written = nbyte;
            free(block);
        }

        //increment current direct block 
        ++current_ind_block;
    }
    
    /* we use an indirect block if the data don't fit in the direct blocks */
    if(nbyte - bytes_written > 0){
        uint16_t *ind_block = calloc(BLOCK_SIZE, 1);
        int ind_block_idx = cursor/BLOCK_SIZE;
        if(inode_table[inode_idx].single_indirect_block){
            if(block_read(inode_table[inode_idx].single_indirect_block, ind_block)){
                perror("fs_write: unable to read indirect block");
                return -1;
            }
        }   
        else{
            unsigned short free_block = find_free_bit(blocks_bitmap);
            if(!free_block){
                cursor += bytes_written;
                inode_table[inode_idx].file_size = cursor > inode_table[inode_idx].file_size ? cursor : inode_table[inode_idx].file_size;
                fd_table[fildes].offset = cursor;
                return bytes_written;
            }
            set_bit(free_block, blocks_bitmap);
            inode_table[inode_idx].single_indirect_block = free_block;
            ind_block_idx = 0;
        }


        //calculate the number of blocks needed for the rest of the data
        unsigned short blocks_needed = (nbyte - bytes_written) % BLOCK_SIZE == 0 ? (nbyte - bytes_written) / BLOCK_SIZE : (nbyte - bytes_written) / BLOCK_SIZE + 1;

        //write to disk for all blocks
        for(;ind_block_idx < blocks_needed; ++ind_block_idx){
            if(first_block){
                first_block = false;
                if(cursor % BLOCK_SIZE == 0){}
                else{
                    uint8_t * block = calloc(BLOCK_SIZE, 1);
                    if(block_read(ind_block[ind_block_idx], block)){
                        perror("fs_write: unable to read first block");
                        return -1;
                    }
                    int write_size = nbyte > (BLOCK_SIZE - cursor % BLOCK_SIZE) ? BLOCK_SIZE - cursor % BLOCK_SIZE : nbyte;
                    memcpy(block + cursor % BLOCK_SIZE, (uint8_t*)buf, write_size);
                    if(block_write(ind_block[ind_block_idx], block)){
                        perror("fs_write: unable to write first block");
                        return -1;
                    }
                    bytes_written += write_size;
                        continue;
                    }

            }
            //find free block
            unsigned short free_ind_block;
            if(!ind_block[ind_block_idx]){
                free_ind_block = find_free_bit(blocks_bitmap);
                if(!free_ind_block){
                    cursor += bytes_written;
                    inode_table[inode_idx].file_size = cursor > inode_table[inode_idx].file_size ? cursor : inode_table[inode_idx].file_size;
                    fd_table[fildes].offset = cursor;
                    return bytes_written;
                }
                set_bit(free_ind_block, blocks_bitmap);
            }
            else{
                free_ind_block = ind_block[ind_block_idx];
            }
            //if it's the last block, we pad 0
            if(ind_block_idx == blocks_needed - 1 && nbyte - bytes_written < BLOCK_SIZE){
                uint8_t *last_block = calloc(BLOCK_SIZE, 1);
                memcpy(last_block, (uint8_t*)buf + bytes_written, nbyte - bytes_written);
                if(block_write(free_ind_block, last_block)){
                    perror("fs_write: unable to write indirect block to disk"); 
                    return -1;
                }
                bytes_written = nbyte;
                free(last_block);
            }

            // write to a whole block if data is big enough
            else{
                if(block_write(free_ind_block, (uint8_t*)buf + bytes_written)){
                    perror("fs_write: unable to write indirect block to disk");
                    return -1;
                }
                bytes_written += BLOCK_SIZE;
            }

            //make sure the indirect block references the block that we just wrote data to
            ind_block[ind_block_idx] = free_ind_block;
        }
        //write the indirect block to disk
        if(block_write(inode_table[inode_idx].single_indirect_block, ind_block)){
            perror("fs_write: unable to write indirect block to disk");
            return -1;
        }
    }

    cursor += bytes_written;
    inode_table[inode_idx].file_size = cursor > inode_table[inode_idx].file_size ? cursor : inode_table[inode_idx].file_size;
    // inode_table[inode_idx].file_size += bytes_written;
    fd_table[fildes].offset = cursor;
    return bytes_written;
}
int fs_get_filesize(int fildes)
{
    if(fildes < 0 || fildes > MAX_NUM_FD - 1){
        perror("fs_get_filesize: invalid fd");
        return -1;
    }
    if(!fd_table[fildes].is_used){
        perror("fs_get_filesize: fd not in use");
        return -1;
    }

    return inode_table[fd_table[fildes].inode_number].file_size;
}

int fs_listfiles(char ***files)
{
    char **f_list = (char **) malloc(MAX_NUM_FILES*sizeof(char*));

    int f_idx = 0;
    for (int i = 0; i < MAX_NUM_FILES; ++i) {
        if(dir_table[i].is_used){
            f_list[f_idx] = (char *)malloc(FILENAME_MAX*sizeof(char));
            strcpy(f_list[f_idx], (char *)dir_table[i].name);
            ++f_idx;
        }
    }

    f_list[f_idx] = NULL;
    *files = f_list;
    return 0;
}
int fs_lseek(int fildes, off_t offset)
{
    if(fildes < 0 || fildes > MAX_NUM_FD - 1){
        perror("fs_lseek: invalid fd");
        return -1;
    }
    if(offset < 0 || offset > inode_table[fd_table[fildes].inode_number].file_size){
        perror("fs_lseek: invalid offset");
        return -1;
    }
    fd_table[fildes].offset = offset;
    return 0;
}
int fs_truncate(int fildes, off_t length)
{
    if(fildes < 0 || fildes > MAX_NUM_FD - 1){
        perror("fs_truncate: invalid fd");
        return -1;
    }
    if(!fd_table[fildes].is_used){
        perror("fs_truncate: fd not in use");
        return -1;
    }

    if(length < 0 || length > inode_table[fd_table[fildes].inode_number].file_size){
        return -1;
    }
    if(length == inode_table[fd_table[fildes].inode_number].file_size){
        return 0;
    }

    uint16_t inode_idx = fd_table[fildes].inode_number;
    unsigned short starting_block = length % BLOCK_SIZE == 0 ? length / BLOCK_SIZE : length / BLOCK_SIZE + 1;

    /* clear the direct blocks (clean the blocks bitmap for those blocks) */
    for(; starting_block < NUM_DIRECT_BLOCKS; ++starting_block){
        if(inode_table[inode_idx].direct_block[starting_block]){
            clear_bit(inode_table[inode_idx].direct_block[starting_block], blocks_bitmap);
        }
        else{
            break;
        }
    }

    /* clear the blocks referenced by the single indirect block */
    if(inode_table[inode_idx].single_indirect_block && inode_table[inode_idx].single_indirect_block < DISK_BLOCKS && starting_block >= NUM_DIRECT_BLOCKS){
        uint16_t * indirect_block = calloc(BLOCK_SIZE, 1);
        if(block_read(inode_table[inode_idx].single_indirect_block, indirect_block)){
            perror("fs_delete: unable to read indirect block");
            return -1;
        }
        int num_block_entries = 2048;
        for (int i = 0; i < num_block_entries; ++i){
            if(*(indirect_block + i)){
                clear_bit(*(indirect_block + i), blocks_bitmap);
            }
        }
        free(indirect_block);
    }

    inode_table[inode_idx].file_size = length;
    if(fd_table[fildes].offset > length){
        fd_table[fildes].offset = length;
    }
    return 0;
}