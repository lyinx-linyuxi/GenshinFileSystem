#include "fs.h"
#include "file_op.h"
#include "init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 注意块号的计算，从0开始
// 0号块为组描述符
// 1号块为数据块位图
// 2号块为inode位图
// 3号块~514号块为inode表
// 515号块~4610号块为数据块

char fs_file[10] = "fs.bin";

// 列出文件
void ls()
{
    ext2_inode* inode = load_inode_entry(current_dir);

    // 权限检查
    int permission = inode->i_mode & 0x0007;
    if ((permission & 0x4) == 0)
    {
        fprintf(stderr, "permission denied\n");
        fprintf(stderr, "ls() failed, current directory %s can not be read\n", current_path);
        free(inode);
        return;
    }

    inode->i_atime = get_current_time();
    update_inode_entry(current_dir, inode);
    __u16 i_block_num = inode->i_blocks;
    __u16* i_block = inode->i_block;
    ext2_dir_entry* dir = (ext2_dir_entry*)malloc(sizeof(ext2_dir_entry));
    if (i_block_num == 0)
    {
        fprintf(stderr, "i_block_num is 0, no data block allocated\n");
        return;
    }

    if (i_block_num)
    {
        int block_num = (i_block_num < 7) ? i_block_num : 6;
        for (int i = 0; i < block_num; i++)
        {
            process_dir_block(i_block[i], dir);
        }
    }
    if (i_block_num >= 7)
    {
        __u16* oneLevelIndex = (__u16*)load_block_entry(i_block[6]);
        for (int k = 0; k < 256 && oneLevelIndex[k] != 0xffff; k++)
        {
            process_dir_block(oneLevelIndex[k], dir);
        }
    }
    if (i_block_num >= 263)
    {
        __u16* TwoLevelIndex = (__u16*)load_block_entry(i_block[7]);
        for (int k = 0; k < 256 && TwoLevelIndex[k] != 0xffff; k++)
        {
            __u16* oneLevelIndex = (__u16*)load_block_entry(TwoLevelIndex[k]);
            for (int m = 0; m < 256 && oneLevelIndex[m] != 0xffff; m++)
            {
                process_dir_block(oneLevelIndex[m], dir);
            }
        }
    }
    free(inode);
    free(dir);
    return;
}

// 创建目录
void mkdir(char* dirname)
{
    if (create_dir(dirname, 7) == -1)
    {
        fprintf(stderr, "mkdir() failed\n");
    }
    return;
}

// 删除目录
void rmdir(char* dirname)
{
    if (delete_dir(dirname) == -1)
    {
        fprintf(stderr, "rmdir() failed\n");
    }
}

// 改变当前目录
int cd(char* dirname)
{
    if (strcmp(dirname, ".") != 0 || strcmp(dirname, "..") != 0 || strcmp(dirname, "/") != 0)
    {
        // 权限检查
        ext2_inode* cdir_inode = load_inode_entry(current_dir);
        int permission = cdir_inode->i_mode & 0x0007;
        if ((permission & 0x1) == 0)
        {
            fprintf(stderr, "permission denied\n");
            fprintf(stderr, "cd() failed, current directory %s can not be execute\n", current_path);
            free(cdir_inode);
            return -1;
        }
    }


    // 特殊情况，cd到根目录，不用查找，直接修改当前目录inode
    if (strcmp(dirname, "/") == 0)
    {
        current_dir = 0;
        strcpy(current_path, "/");
        return 0;
    }
    // 需要返回当前目录的inode号，如果不是目录返回-1代表错误
    int isRoot = 1;
    if (strcmp(current_path, "/") != 0)
    {
        isRoot = 0;
    }
    int pos, free_entry, offset;
    int ret = search_filename(dirname, &pos, &free_entry, &offset);
    if (ret == 0xffff)
    {
        fprintf(stderr, "directory %s does not exist\n", dirname);
        return -1;
    }
    ext2_inode* inode = load_inode_entry(ret);
    if (((inode->i_mode & 0xff00) >> 8) != 2)
    {
        fprintf(stderr, "%s is not a directory\n", dirname);
        return -1;
    }
    // 修改当前目录索引节点
    current_dir = ret;
    inode->i_atime = get_current_time();
    update_inode_entry(ret, inode);
    // 修改当前路径
    if (strcmp(dirname, "..") == 0)
    {
        if (isRoot == 1)
        {
            return 0;
        }
        int i = strlen(current_path) - 1;
        while (current_path[i] != '/')
        {
            current_path[i] = '\0';
            i--;
        }
        if (strcmp(current_path, "/"))
        {
            current_path[i] = '\0';
        }
    }
    else if (strcmp(".", dirname) != 0)
    {
        if (isRoot != 1)
        {
            strcat(current_path, "/");
        }
        strcat(current_path, dirname);
    }
    return ret;
}

// 改变文件权限
void attrib(char* filename, int permission)
{
    int pos, free_entry, offset;
    int ret = search_filename(filename, &pos, &free_entry, &offset);
    if (ret == 0xffff)
    {
        fprintf(stderr, "attrib() failed, file %s does not exist\n", filename);
        return;
    }
    if (permission > 7 || permission < 0)
    {
        fprintf(stderr, "permission must be 0~7\n");
        return;
    }
    ext2_inode* inode = load_inode_entry(ret);
    inode->i_mode = (inode->i_mode & 0xff00) | permission;
    update_inode_entry(ret, inode);
}

// 创建文件
void touch(char* filename)
{
    if (create_file(filename, 7) == -1)
    {
        fprintf(stderr, "touch() failed\n");
    }
    return;
}

// 删除文件
void rm(char* filename)
{
    if (delete_file(filename) == -1)
    {
        fprintf(stderr, "rm() failed\n");
    }
}

// 格式化文件系统
void format()
{
    if (super_user == 0)
    {
        fprintf(stderr, "You don't have the permission to format\n");
        return;
    }
    initialize_disk();
    initialize_memory();
    super_user = 0;
}

void rmdir_recursive(char* dirname)
{

    if (super_user == 0)
    {
        fprintf(stderr, "You don't have the permission to delete dir recursive\n");
        return;
    }

    // 检查权限
    ext2_inode* cdir_inode = load_inode_entry(current_dir);
    int permission = cdir_inode->i_mode & 0x0007;
    if ((permission & 0x4) == 0)
    {
        fprintf(stderr, "permission denied\n");
        fprintf(stderr, "rmdir_rec() failed, current directory %s can not be write\n", current_path);
        free(cdir_inode);
        return;
    }

    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0
        || strcmp("/", dirname) == 0 || strcmp("", dirname) == 0)
    {
        fprintf(stderr, "The dir %s cannot be remved\n", dirname);
        return;
    }
    int pos, free_entry, offset;
    int ret = search_filename(dirname, &pos, &free_entry, &offset);
    if (ret == 0xffff)
    {
        fprintf(stderr, "directory %s does not exist\n", dirname);
        return;
    }
    ext2_inode* inode = load_inode_entry(ret);
    for (int i = 0;i < inode->i_blocks;i++)
    {
        __u8* block = load_block_entry(load_indexed_data_block(ret, i));
        ext2_dir_entry* dir = (ext2_dir_entry*)malloc(sizeof(ext2_dir_entry));
        for (int j = 0; j < BLOCK_SIZE; j += dir->rec_len)
        {
            memcpy(dir, block + j, sizeof(ext2_dir_entry));
            if (dir->inode == 0xffff)
            {
                continue;
            }
            if (strcmp(dir->name, ".") == 0 || strcmp(dir->name, "..") == 0)
            {
                continue;
            }
            if (dir->rec_len == 0)
            {
                break;
            }
            if (((load_inode_entry(dir->inode)->i_mode & 0xff00) >> 8) == 2)
            {
                int ck = check_dir_empty(dir->inode);
                if (ck == 0)
                {
                    cd(dirname);
                    rmdir_recursive(dir->name);
                    printf("delete dir %s\n", dir->name);
                    cd("..");
                }
                else
                {
                    cd(dirname);
                    delete_dir(dir->name);
                    printf("delete empty dir %s\n", dir->name);
                    cd("..");
                }
            }
            else
            {
                cd(dirname);
                delete_file(dir->name);
                printf("delete file %s\n", dir->name);
                cd("..");
            }
        }
    }
    delete_dir(dirname);
    return;
}