#include "file_op.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 创建文件
int create_file(char* filename, int permission)
{
    // 创建一个新文件
    // 输入文件名，创建一个新文件
    // 返回0表示成功，返回-1表示失败
    // 如果文件已经存在，则返回-1
    ext2_inode* cdir_inode = load_inode_entry(current_dir);

    // 先检查当前目录的权限
    int cdir_permission = cdir_inode->i_mode & 0x0007;
    if ((cdir_permission & 0x4) == 0)
    {
        fprintf(stderr, "permission denied\n");
        fprintf(stderr, "create_file() failed, current directory %s can not be write\n", current_path);
        free(cdir_inode);
        return -1;
    }


    int pos;        // 返回文件指针位置。
    int free_entry; // 返回第一个空闲表项的位置
    int offset;
    if (permission > 7 || permission < 0)
    {
        fprintf(stderr, "permission must be 0~7\n");
        return -1;
    }
    time_t current_time;
    time(&current_time);
    int ret = search_filename(filename, &pos, &free_entry, &offset);
    if (ret != 0xffff)
    {
        fprintf(stderr, "file %s already exists\n", filename);
        return -1;
    }
    // 分配inode
    __u16 inode_num = ext2_new_inode();
    if (inode_num == 0xffff)
    {
        fprintf(stderr, "ext2_new_inode() failed\n");
        return -1;
    }
    // 分配1块数据块
    // __u16 data_block_num = ext2_alloc_block();
    // if (data_block_num == 0xffff)
    //{
    // fprintf(stderr, "ext2_alloc_block() failed\nghost file risk, stop creating file\n");
    // return;
    //}
    ext2_inode* inode = load_inode_entry(inode_num);
    __u8* block = (__u8*)malloc(sizeof(__u8) * BLOCK_SIZE);
    memset(block, 0, sizeof(__u8) * BLOCK_SIZE); // 清空数据块
    for (int i = 0; i < 8; i++)
    {
        inode->i_block[i] = 0;
    }
    inode->i_mode = (0x0000 | (__u16)1 << 8) | 0x0007;
    inode->i_blocks = 0;
    inode->i_size = 0;
    inode->i_atime = current_time;
    inode->i_ctime = current_time;
    inode->i_mtime = current_time;
    inode->i_dtime = 0;
    update_inode_entry(inode_num, inode);
    // 将目录项写入数据块中
    ext2_dir_entry* new_dir = (ext2_dir_entry*)malloc(sizeof(ext2_dir_entry));
    new_dir->inode = inode_num;
    strcpy(new_dir->name, filename);
    new_dir->name_len = strlen(new_dir->name);
    new_dir->file_type = 1;
    // 目录项大小要和当前分配空间比较，因为可能是之前被删掉的目录
    new_dir->rec_len = 7 + strlen(new_dir->name);

    // ext2_inode* cdir_inode = load_inode_entry(current_dir);
    cdir_inode->i_mtime = get_current_time();
    int target_block = free_entry;
    if (target_block == -2)
    {
        target_block = ext2_alloc_block();
        if (target_block == 0xffff)
        {
            fprintf(stderr, "ext2_alloc_block() failed\n No free block for a new dir_entry\n");
            // 无法写入目录则释放分配的inode返回失败值-1
            check_bitmap(load_inode_bitmap(), inode_num) ? ext2_free_inode(inode_num) : 0;
            return -1;
        }
        update_inode_i_block(cdir_inode, current_dir, target_block);
    }
    block = load_block_entry(target_block);
    // 取出当前分配的目录项，比较长度
    // 如果是新分配的目录项，长度必定为0，如果是之前删除的，则长度大于等于新目录项的长度
    // 如果大于则需要将目录项长度拓展
    ext2_dir_entry* avai_dir = (ext2_dir_entry*)malloc(sizeof(ext2_dir_entry));
    memcpy(avai_dir, block + offset, sizeof(ext2_dir_entry));
    if (avai_dir->rec_len >= 7 + strlen(new_dir->name))
    {
        new_dir->rec_len = avai_dir->rec_len;
    }
    memcpy(block + offset, new_dir, new_dir->rec_len);
    update_block_entry(target_block, block);
    cdir_inode->i_size += new_dir->rec_len;
    update_inode_entry(inode_num, inode);
    update_inode_entry(current_dir, cdir_inode);
    free(new_dir);
    free(avai_dir);
    free(block);
    return 0;
}

// 创建目录
int create_dir(char* dirname, int permission)
{
    // 创建一个新目录
    // 输入目录名，创建一个新目录
    // 返回0表示成功，返回-1表示失败
    // 如果目录已经存在，则返回-1

    // 先检查当前目录的权限
    ext2_inode* cdir_inode = load_inode_entry(current_dir);
    int cdir_permission = cdir_inode->i_mode & 0x0007;
    if ((cdir_permission & 0x4) == 0)
    {
        fprintf(stderr, "permission denied\n");
        fprintf(stderr, "create_dir() failed, current directory %s can not be write\n", current_path);
        free(cdir_inode);
        return -1;
    }
    int pos;        // 返回文件指针位置。
    int free_entry; // 返回第一个空闲表项的位置
    int offset;
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0 || strcmp(dirname, "/") == 0 || strcmp(dirname, "") == 0)
    {
        fprintf(stderr, "The dir %s cannot be created\n", dirname);
        return -1;
    }
    if (permission > 7 || permission < 0)
    {
        fprintf(stderr, "permission must be 0~7\n");
        return -1;
    }
    time_t current_time;
    time(&current_time);
    int ret = search_filename(dirname, &pos, &free_entry, &offset);
    if (ret != 0xffff)
    {
        fprintf(stderr, "directory %s already exists\n", dirname);
        return -1;
    }
    // 分配inode
    __u16 inode_num = ext2_new_inode();
    if (inode_num == 0xffff)
    {
        fprintf(stderr, "ext2_new_inode() failed\n");
        return -1;
    }
    // 分配1块数据块
    __u16 data_block_num = ext2_alloc_block();
    if (data_block_num == 0xffff)
    {
        fprintf(stderr, "ext2_alloc_block() failed\nghost file risk, stop creating file\n");
        // 无法分配目录项则释放分配的inode返回失败值-1
        check_bitmap(load_inode_bitmap(), inode_num) ? ext2_free_inode(inode_num) : 0;
        return -1;
    }

    // 分配inode和数据块成功，开始写入数据
    ext2_inode* inode = load_inode_entry(inode_num);
    __u8* block = (__u8*)malloc(sizeof(__u8) * BLOCK_SIZE);
    memset(block, 0, sizeof(__u8) * BLOCK_SIZE); // 清空数据块
    for (int i = 0; i < 8; i++)
    {
        inode->i_block[i] = 0;
    }
    inode->i_block[0] = data_block_num;
    inode->i_mode = (0x0000 | (__u16)2 << 8) | 0x0007;
    inode->i_blocks = 1;
    inode->i_size = 17;
    inode->i_atime = current_time;
    inode->i_ctime = current_time;
    inode->i_mtime = current_time;
    inode->i_dtime = 0;
    update_inode_entry(inode_num, inode);

    // 将目录项写入当前目录中
    ext2_dir_entry* new_dir = (ext2_dir_entry*)malloc(sizeof(ext2_dir_entry));
    new_dir->inode = inode_num;
    strcpy(new_dir->name, dirname);
    new_dir->name_len = strlen(new_dir->name);
    new_dir->file_type = 2;
    new_dir->rec_len = 7 + strlen(new_dir->name);
    // ext2_inode* cdir_inode = load_inode_entry(current_dir);
    int target_block = free_entry;
    if (target_block == -2)
    {
        target_block = ext2_alloc_block();
        if (target_block == 0xffff)
        {
            fprintf(stderr, "ext2_alloc_block() failed\n No free block for a new dir_entry\n");
            // 无法写入目录则释放分配的inode返回失败值-1
            check_bitmap(load_inode_bitmap(), inode_num) ? ext2_free_inode(inode_num) : 0;
            check_bitmap(load_data_bitmap(), data_block_num) ? free_block_and_update_bitmap(data_block_num) : 0;
            return -1;
        }
        update_inode_i_block(cdir_inode, current_dir, target_block);
    }
    block = load_block_entry(target_block);
    // 取出当前分配的目录项，比较长度
    // 如果是新分配的目录项，长度必定为0，如果是之前删除的，则长度大于等于新目录项的长度
    // 如果大于则需要将目录项长度拓展
    ext2_dir_entry* avai_dir = (ext2_dir_entry*)malloc(sizeof(ext2_dir_entry));
    memcpy(avai_dir, block + offset, sizeof(ext2_dir_entry));
    if (avai_dir->rec_len >= 7 + strlen(new_dir->name))
    {
        new_dir->rec_len = avai_dir->rec_len;
    }
    memcpy(block + offset, new_dir, new_dir->rec_len);
    update_block_entry(target_block, block);
    cdir_inode->i_size += new_dir->rec_len;
    cdir_inode->i_mtime = get_current_time();
    update_inode_entry(inode_num, inode);
    update_inode_entry(current_dir, cdir_inode);

    // 新建目录的.和..目录项
    // .
    ext2_dir_entry* cur = (ext2_dir_entry*)malloc(sizeof(ext2_dir_entry));
    cur->inode = new_dir->inode;
    strcpy(cur->name, ".");
    cur->name_len = strlen(cur->name);
    cur->file_type = 2;
    cur->rec_len = 7 + strlen(cur->name);
    inode->i_size += cur->rec_len;

    // ..
    ext2_dir_entry* par = (ext2_dir_entry*)malloc(sizeof(ext2_dir_entry));
    par->inode = current_dir;
    strcpy(par->name, "..");
    par->name_len = strlen(par->name);
    par->file_type = 2;
    par->rec_len = 7 + strlen(par->name);
    inode->i_size += par->rec_len;

    reload_group_desc();
    group_desc->bg_used_dirs_count++;
    update_group_desc();

    // 将.和..目录项写入新建目录的数据块中
    block = load_block_entry(data_block_num);
    memcpy(block, cur, cur->rec_len);
    memcpy(block + cur->rec_len, par, par->rec_len);
    update_block_entry(data_block_num, block);
    free(new_dir);
    free(avai_dir);
    free(cur);
    free(par);
    free(block);
    free(inode);
    return 0;
}

// 辅助函数，删除文件前需要将文件关闭使用
int get_open_fd(char* filename)
{
    for (int i = 0; i < 16; i++)
    {
        if (fopen_table[i].filename != NULL && strcmp(fopen_table[i].filename, filename) == 0
            && fopen_table[i].path != NULL && strcmp(fopen_table[i].path, current_path) == 0)
        {
            return i;
        }
    }
    return -1;
}

// 删除文件
int delete_file(char* filename)
{
    // 删除一个文件
    // 输入文件名，删除一个文件
    // 返回0表示成功，返回-1表示失败
    // 如果文件不存在，则返回-1

    // 先检查当前目录的权限
    ext2_inode* cdir_inode = load_inode_entry(current_dir);
    int permission = cdir_inode->i_mode & 0x0007;
    if ((permission & 0x4) == 0)
    {
        fprintf(stderr, "permission denied\n");
        fprintf(stderr, "delete_file() failed, current directory %s can not be write\n", current_path);
        free(cdir_inode);
        return -1;
    }

    int pos, free_entry, offset;
    int ret = search_filename(filename, &pos, &free_entry, &offset);
    if (ret == 0xffff)
    {
        fprintf(stderr, "file %s does not exist\n", filename);
        return -1;
    }
    int fd = get_open_fd(filename);
    if (fd != -1)
    {
        fprintf(stderr, "close file %s before deleting\n", filename);
        close(fd);
    }
    // 释放inode和数据块
    ext2_inode* inode = load_inode_entry(ret);
    if (((inode->i_mode & 0xff00) >> 8) != 1)
    {
        fprintf(stderr, "%s is a direcory\n", filename);
        return -1;
    }
    inode->i_dtime = get_current_time();
    update_inode_entry(ret, inode);
    if (inode->i_blocks != 0)
    {
        ext2_free_blocks(inode);
    }
    ext2_free_inode(ret);

    // 释放目录项
    __u8* block = load_block_entry(pos);
    ext2_dir_entry* dir = (ext2_dir_entry*)(block + offset);
    dir->inode = 0xffff;
    update_block_entry(pos, block);

    // 更新当前目录的inode
    // ext2_inode* cdir_inode = load_inode_entry(current_dir);
    cdir_inode->i_size -= dir->rec_len;
    cdir_inode->i_mtime = get_current_time();
    update_inode_entry(current_dir, cdir_inode);
    return 0;
}

// 删除目录
int delete_dir(char* dirname)
{
    // 删除一个目录
    // 输入目录名，删除一个目录
    // 返回0表示成功，返回-1表示失败
    // 如果目录不存在，则返回-1

    // 先检查当前目录的权限
    ext2_inode* cdir_inode = load_inode_entry(current_dir);
    int permission = cdir_inode->i_mode & 0x0007;
    if ((permission & 0x4) == 0)
    {
        fprintf(stderr, "permission denied\n");
        fprintf(stderr, "delete_dir() failed, current directory %s can not be write\n", current_path);
        free(cdir_inode);
        return -1;
    }

    // 检查删除目录是否合法
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0
        || strcmp("/", dirname) == 0 || strcmp("", dirname) == 0)
    {
        fprintf(stderr, "The dir %s cannot be remved\n", dirname);
        return -1;
    }
    int pos, free_entry, offset;
    int ret = search_filename(dirname, &pos, &free_entry, &offset);
    if (ret == 0xffff)
    {
        fprintf(stderr, "directory %s does not exist\n", dirname);
        return -1;
    }

    // 释放inode和数据块
    ext2_inode* inode = load_inode_entry(ret);
    if (((inode->i_mode & 0xff00) >> 8) != 2)
    {
        fprintf(stderr, "%s is a file\n", dirname);
        return -1;
    }
    int check = check_dir_empty(ret);
    if (check == 0)
    {
        fprintf(stderr, "dir %s is not empty\n", dirname);
        return -1;
    }
    ext2_free_blocks(inode);
    ext2_free_inode(ret);

    // 释放目录项
    __u8* block = load_block_entry(pos);
    ext2_dir_entry* dir = (ext2_dir_entry*)(block + offset);
    dir->inode = 0xffff;
    update_block_entry(pos, block);

    // 更新当前目录的inode
    // ext2_inode* cdir_inode = load_inode_entry(current_dir);
    cdir_inode->i_size -= dir->rec_len;
    cdir_inode->i_mtime = get_current_time();
    update_inode_entry(current_dir, cdir_inode);

    // 是目录则更新组描述符
    reload_group_desc();
    group_desc->bg_used_dirs_count--;
    update_group_desc();
    return 0;
}

// 打开文件
int open(char* filename)
{
    // 检查是否已经打开
    for (int i = 0; i < 16; i++)
    {
        if (fopen_table[i].filename != NULL && strcmp(fopen_table[i].filename, filename) == 0
            && fopen_table[i].path != NULL && strcmp(fopen_table[i].path, current_path) == 0)
        {
            printf("file %s is already opened\n", filename);
            return i;
        }
    }

    // 检查文件是否存在
    int pos, free_entry, offset;
    int ret = search_filename(filename, &pos, &free_entry, &offset);
    if (ret == 0xffff)
    {
        fprintf(stderr, "open() failed, file %s does not exist\n", filename);
        return -1;
    }
    // 检查打开的是文件还是目录
    ext2_inode* inode = load_inode_entry(ret);
    if ((inode->i_mode & 0xff00) >> 8 == 2)
    {
        fprintf(stderr, "open() failed, %s is a directory\n", filename);
        return -1;
    }
    // 没打开就打开，并且修改打开文件表
    for (int i = 0; i < 16; i++)
    {
        if (fopen_table[i].inode_num == 0xffff)
        {
            fopen_table[i].inode_num = ret;
            fopen_table[i].offset = 0;
            fopen_table[i].filename = (char*)malloc(sizeof(char) * (1 + strlen(filename)));
            fopen_table[i].path = (char*)malloc(sizeof(char) * (1 + strlen(current_path)));
            strcpy(fopen_table[i].filename, filename);
            strcpy(fopen_table[i].path, current_path);
            // 修改inode的atime
            inode->i_atime = get_current_time();
            update_inode_entry(ret, inode);
            free(inode);
            printf("open success\n");
            return i;
        }
    }
    // 打开了16个文件
    print_open_file_table();
    return -2;
}


// 辅助函数，用于用户看到打开了哪些文件
void print_open_file_table()
{
    printf("********************opened files********************\n");
    for (int i = 0;i < 16;i++)
    {
        if (fopen_table[i].filename != NULL)
        {
            printf("fd = %2d, file name = %-s\n", i, fopen_table[i].filename);
        }
        else
        {
            continue;
        }
    }
}

// 读取文件
int read(int fd, __u8* dest, __u32 size)
{
    // 输入文件描述符，根据文件描述符指定的文件inode进行修改
    // dest是读取的内容，size是希望读取的大小
    // 返回实际读取的大小
    int test = test_fd(fd);
    if (test == -1)
    {
        fprintf(stderr, "read() failed, fd %d is not valid\n", fd);
        return -1;
    }
    ext2_inode* inode = load_inode_entry(fopen_table[fd].inode_num);
    // __u16 i_block_num = inode->i_blocks;
    // __u16* i_block = inode->i_block;
    __u32 offset = fopen_table[fd].offset;
    __u32 read_size = 0;
    __u8* block = (__u8*)malloc(sizeof(__u8) * BLOCK_SIZE);
    // 检查权限
    int permission = inode->i_mode & 0x0007;
    if ((permission & 0x4) == 0)
    {
        fprintf(stderr, "read() failed, file %s is read-only\n", fopen_table[fd].filename);
        free(inode);
        free(block);
        return -1;
    }

    // 更新atime
    inode->i_atime = get_current_time();
    update_inode_entry(fopen_table[fd].inode_num, inode);

    // 读取数据
    for (int index = 0; index < inode->i_blocks; index++)
    {
        int blockth = load_indexed_data_block(fopen_table[fd].inode_num, index);
        block = load_block_entry(blockth);
        if (offset < BLOCK_SIZE)
        {
            __u32 read = (size > BLOCK_SIZE - offset) ? BLOCK_SIZE - offset : size;
            memcpy(dest + read_size, block + offset, read);
            read_size += read;
            offset = 0;
            size -= read;
        }
        else
        {
            offset -= BLOCK_SIZE;
        }
        if (size == 0)
        {
            free(inode);
            free(block);
            fopen_table[fd].offset += read_size; // 更新文件指针
            return read_size;
        }
    }
    fprintf(stdout, "read() reached end of %s\n", fopen_table[fd].filename);
    free(inode);
    free(block);
    return read_size;
}

// 写入文件
int write(int fd, __u8* src, __u32 size)
{
    // 输入文件描述符，根据文件描述符指定的文件inode进行修改
    // src是写入的内容，size是写入的大小
    // 返回是否成功写入
    int test = test_fd(fd);
    if (test == -1)
    {
        fprintf(stderr, "write() failed, fd %d is not valid\n", fd);
        return -1;
    }
    ext2_inode* inode = load_inode_entry(fopen_table[fd].inode_num);
    __u16 i_block_num = inode->i_blocks;
    // __u16* i_block = inode->i_block;
    __u32 offset = fopen_table[fd].offset;
    int original_offset = offset;
    __u32 write_size = 0;
    __u8* block = (__u8*)malloc(sizeof(__u8) * BLOCK_SIZE);

    // 检查权限
    int permission = inode->i_mode & 0x0007;
    if ((permission & 0x2) == 0)
    {
        fprintf(stderr, "write() failed, file %s has no write permission\n", fopen_table[fd].filename);
        free(inode);
        free(block);
        return -1;
    }

    // 分配足够的区域
    int needed_blocks = ((offset + size) % BLOCK_SIZE == 0) ? (offset + size) / BLOCK_SIZE : (offset + size) / BLOCK_SIZE + 1;
    int* allocated_block_num = (int*)malloc(sizeof(int) * (needed_blocks - i_block_num));
    if (needed_blocks > i_block_num)
    {
        for (int i = i_block_num; i < needed_blocks; i++)
        {
            allocated_block_num[i - i_block_num] = ext2_alloc_block();
            if (allocated_block_num[i - i_block_num] == 0xffff)
            {
                fprintf(stderr, "ext2_alloc_block() failed\n");
                // 释放之前分配的数据块
                for (int j = 0; j < i - i_block_num; j++)
                {
                    check_bitmap(load_data_bitmap(), allocated_block_num[j]) ? free_block_and_update_bitmap(allocated_block_num[j]) : 0;
                }
                return -1;
            }
            update_inode_i_block(inode, fopen_table[fd].inode_num, allocated_block_num[i - i_block_num]);
        }
    }

    // 写入数据
    for (int index = 0; index < inode->i_blocks; index++)
    {
        int blockth = load_indexed_data_block(fopen_table[fd].inode_num, index);
        block = load_block_entry(blockth);
        if (offset < BLOCK_SIZE)
        {
            __u32 write = (size > BLOCK_SIZE - offset) ? BLOCK_SIZE - offset : size;
            memcpy(block + offset, src + write_size, write);
            write_size += write;
            offset = 0;
            size -= write;
            update_block_entry(blockth, block);
        }
        else
        {
            offset -= BLOCK_SIZE;
        }
        if (size == 0)
        {
            // 更新isize，只更新余量，因为在分配数据块的过程中就已经更新为分配数据块乘上数据块大小
            // 采取覆盖写入，所有若写入后没有变化则之前的数据仍然保留，只有变大而不会变小
            if (original_offset + write_size > inode->i_size)
            {
                if ((original_offset % BLOCK_SIZE) + write_size > BLOCK_SIZE)
                {
                    int left = (original_offset + write_size) % BLOCK_SIZE;
                    inode->i_size += left;
                }
                else
                {
                    inode->i_size += write_size;
                }
            }
            // 更新mtime和atime
            inode->i_mtime = get_current_time();
            inode->i_atime = get_current_time();
            update_inode_entry(fopen_table[fd].inode_num, inode);
            fopen_table[fd].offset += write_size;  // 更新文件指针
            return write_size;
        }
    }
    free(inode);
    free(block);
    fprintf(stderr, "error in write, because this file has been allocated adequate blocks, it is wrong to run here\n");
    return -1;
}

// 关闭文件
void close(int fd)
{
    int test = test_fd(fd);
    if (test == -1)
    {
        fprintf(stderr, "close() failed, fd %d is not valid\n", fd);
        return;
    }
    fopen_table[fd].filename = NULL;
    fopen_table[fd].inode_num = 0xffff;
    fopen_table[fd].offset = 0xffff;
    fopen_table[fd].path = NULL;
    return;
}

// 一个目录项处理函数，用于ls显示信息
void dir_entry_process(ext2_dir_entry* directory)
{
    static int print_prompt = 0;
    if (!print_prompt)
    {
        printf("Type\t  Size\t    cTime\t\taTime\t\t    mTime\t\tName\n");
        print_prompt = 1;
    }
    char* filename = directory->name;
    ext2_inode* cfile_inode = load_inode_entry(directory->inode);
    char wr_str[4] = "---";
    int wr = cfile_inode->i_mode & 0x0007;
    wr_str[0] = (wr & 0x4) ? 'r' : '-';
    wr_str[1] = (wr & 0x2) ? 'w' : '-';
    wr_str[2] = (wr & 0x1) ? 'x' : '-';
    int file_type = (cfile_inode->i_mode & 0xff00) >> 8;
    if (file_type == 2)
    {
        strcat(filename, "/");
    }
    char ft = (file_type == 1) ? '-' : 'd';
    int size = cfile_inode->i_size;

    // 时间处理有大坑，返回的struct tm*指针指向的是同一个地方，输出放在最后，会导致输出的时间都是最后一个文件的时间
    char ctime[80];
    struct tm* ctimeinfo = localtime((long long int*) & cfile_inode->i_ctime);
    strftime(ctime, 80, "%Y-%m-%d %H:%M:%S", ctimeinfo);

    char atime[80];
    struct tm* atimeinfo = localtime((long long int*) & cfile_inode->i_atime);
    strftime(atime, 80, "%Y-%m-%d %H:%M:%S", atimeinfo);

    char mtime[80];
    struct tm* mtimeinfo = localtime((long long int*) & cfile_inode->i_mtime);
    strftime(mtime, 80, "%Y-%m-%d %H:%M:%S", mtimeinfo);

    printf("%c%s%s%s %8d %s %s %s %s\n", ft, wr_str, wr_str, wr_str, size, ctime, atime, mtime, filename);
    fflush(stdout);
}

// 对一个数据块的目录项进行处理
void process_dir_block(__u16 block_num, ext2_dir_entry* dir)
{
    __u8* block = load_block_entry(block_num);
    for (int j = 0; j < BLOCK_SIZE; j += dir->rec_len)
    {
        memcpy(dir, block + j, sizeof(ext2_dir_entry));
        if (dir->rec_len == 0)
        {
            // reclen 为0不再代表找到所有目录的结尾
            // 但是代表该块找到结尾了，直接跳出加载新的块
            return;
        }
        // 被删除的目录不再列出
        if (dir->inode == 0xffff)
        {
            continue;
        }
        dir_entry_process(dir);
        if (j + 7 >= BLOCK_SIZE)
        {
            break;
        }
    }
}

// 检查目录是否为空
int check_dir_empty(__u16 inode_num)
{
    ext2_inode* inode = load_inode_entry(inode_num);
    __u16 i_block_num = inode->i_blocks;
    if (i_block_num != 1)
    {
        free(inode);
        return 0;
    }

    __u8* block = load_block_entry(inode->i_block[0]);
    ext2_dir_entry* dir = (ext2_dir_entry*)block;
    int limit = BLOCK_SIZE;
    for (int j = 0; j < limit; j += dir->rec_len)
    {
        dir = (ext2_dir_entry*)(block + j);
        if (dir->inode == 0xffff)
        {
            continue;
        }
        if (dir->rec_len == 0)
        {
            break;
        }
        if (strcmp(dir->name, ".") != 0 && strcmp(dir->name, "..") != 0)
        {
            free(inode);
            free(block);
            return 0;
        }
    }
    free(inode);
    free(block);
    return 1;
}

// 辅助函数，将路径进行解析
#define MAX_DEPTH 10  // 最大递归深度
#define MAX_ARG_LEN 200 // 最大路径长度
char** parse_path(char* pathname, int* arg_num)
{
    // 从pathname中解析出文件名
    // 返回二维数组，第一维是文件名，第二维是文件名的长度
    *arg_num = 0;
    if (strlen(pathname) == 0 || strlen(pathname) > MAX_ARG_LEN)
    {
        fprintf(stderr, "invalid pathname\n");
        return NULL;
    }
    char** ret = (char**)malloc(sizeof(char*) * MAX_DEPTH);
    for (int i = 0; i < MAX_DEPTH; i++)
    {
        ret[i] = (char*)malloc(sizeof(char) * MAX_ARG_LEN);
    }
    if (pathname[0] == '/')
    {
        ret[(*arg_num)++] = "/";
    }
    char* token = strtok(pathname, "/");
    while (token != NULL)
    {
        ret[(*arg_num)++] = token;
        token = strtok(NULL, "/");
    }
    return ret;
}
