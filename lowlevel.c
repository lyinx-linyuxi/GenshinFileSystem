#include "lowlevel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 在寻找空闲目录项时的标志，表示找到空闲目录项
static int first_match = 1;

/*分配一个新的索引结点*/
__u16 ext2_new_inode()
{
    // 返回一个空闲的inode号
    // 如果没有空闲的inode，则返回0xffff
    // 不存在0xffff号inode
    __u8 *block = read_block(group_desc->bg_inode_bitmap);
    __u16 ret = find_bitmap(block, last_alloc_inode);
    if (ret == -1)
    {
        fprintf(stderr, "No free inode\n");
        return 0xffff;
    }
    last_alloc_inode = ret;
    block[ret / 8] |= (0x80 >> (ret % 8));
    write_block(group_desc->bg_inode_bitmap, block);
    reload_group_desc();
    group_desc->bg_free_inodes_count--;
    update_group_desc();
    return ret;
}

/*分配一个新的数据块*/
__u16 ext2_alloc_block()
{
    // 返回一个空闲的数据块号
    // 如果没有空闲的数据块，则返回0xffff
    __u8 *block = read_block(group_desc->bg_block_bitmap);
    __u16 ret = find_bitmap(block, last_alloc_block);
    if (ret == 0xffff)
    {
        fprintf(stderr, "No free block\n");
        return 0xffff;
    }
    last_alloc_block = ret;
    block[ret / 8] |= (0x80 >> (ret % 8));
    write_block(group_desc->bg_block_bitmap, block);
    reload_group_desc();
    group_desc->bg_free_blocks_count--;
    update_group_desc();
    return ret;
}

/*释放特定的索引结点,并写入文件系统*/
__u16 ext2_free_inode(__u16 inode_num)
{
    // 释放一个特定的已分配的inode
    // 返回0表示成功，返回-1表示失败
    // 如果特定结点并没用被分配，则返回0xffff
    __u8 *block = read_block(group_desc->bg_inode_bitmap);
    if ((block[inode_num / 8] & (0x80 >> (inode_num % 8))) == 0)
    {
        fprintf(stderr, "inode %d is not allocated\n", inode_num);
        return 0xffff;
    }
    block[inode_num / 8] &= ~(0x80 >> (inode_num % 8));
    write_block(group_desc->bg_inode_bitmap, block);
    reload_group_desc();
    group_desc->bg_free_inodes_count++;
    update_group_desc();
    return 0;
}

/*释放特定块号的数据块位图*/
__u16 ext2_free_block_bitmap(__u16 data_block_offset)
{
    // 释放特定数据块在位图中的注册
    // 返回0表示成功，返回-1表示失败
    // 如果特定结点并没用被分配，则返回-1
    __u8 *block = read_block(group_desc->bg_block_bitmap);
    if ((block[data_block_offset / 8] & (0x80 >> (data_block_offset % 8))) == 0)
    {
        fprintf(stderr, "data block %d is not allocated\n", data_block_offset);
        return 0xffff;
    }
    block[data_block_offset / 8] &= ~(0x80 >> (data_block_offset % 8));
    write_block(group_desc->bg_block_bitmap, block);
    reload_group_desc();
    group_desc->bg_free_blocks_count++;
    update_group_desc();
    free(block);
    return 0;
}

/*根据多级索引机制更新索引结点的数据块信息域*/
int update_inode_i_block(ext2_inode *inode, int inode_num, __u16 added_block_num)
{
    // 传入inode结构体，和需要新增加的数据块号
    // 将新增加的数据块号写入到inode中的i_block中
    int iblocks = inode->i_blocks;

    if (iblocks < 6)
    {
        // 直接索引没用完直接用
        inode->i_block[iblocks] = added_block_num;
        inode->i_size = inode->i_blocks * BLOCK_SIZE;
        inode->i_blocks++;
        inode->i_mtime = get_current_time();
    }
    if (iblocks == 6)
    {
        // 直接索引已经用完，需要分配一级索引
        inode->i_block[6] = ext2_alloc_block();
        inode->i_size = inode->i_blocks * BLOCK_SIZE;
        inode->i_blocks++;
        inode->i_mtime = get_current_time();
        __u16 *block = (__u16 *)load_block_entry(inode->i_block[6]);
        block[0] = added_block_num;
        block[1] = 0xffff;
        update_block_entry(inode->i_block[6], (__u8 *)block);
        free(block);
    }
    if (iblocks >= 7 && inode->i_blocks < 262)
    {
        // 一级索引没用完
        __u16 *oneLevelIndex = (__u16 *)load_block_entry(inode->i_block[6]);
        int tar = (inode->i_blocks - 6) % 256;
        oneLevelIndex[tar] = added_block_num;
        oneLevelIndex[tar + 1] = 0xffff;
        inode->i_size = inode->i_blocks * BLOCK_SIZE;
        inode->i_blocks++;
        inode->i_mtime = get_current_time();
        update_block_entry(inode->i_block[6], (__u8 *)oneLevelIndex);
    }
    if (iblocks == 262)
    {
        // 一级索引用完，需要分配二级索引
        inode->i_block[7] = ext2_alloc_block();

        __u16 *block = (__u16 *)load_block_entry(inode->i_block[7]);
        block[0] = ext2_alloc_block();
        block[1] = 0xffff;
        update_block_entry(inode->i_block[7], (__u8 *)block);
        __u16 *temp = (__u16 *)load_block_entry(block[0]);
        temp[0] = added_block_num;
        temp[1] = 0xffff;
        inode->i_size = inode->i_blocks * BLOCK_SIZE;
        inode->i_blocks++;
        inode->i_mtime = get_current_time();
        update_block_entry(block[0], (__u8 *)temp);
        free(block);
        free(temp);
    }
    if (iblocks > 262 && inode->i_blocks < 4096)
    {
        // 二级索引没用完，肯定用不完
        __u16 *block = (__u16 *)load_block_entry(inode->i_block[7]);
        int tar = (inode->i_blocks - 262);
        if (block[tar / 256] == 0xffff)
        {
            block[tar / 256] = ext2_alloc_block();
            block[tar / 256 + 1] = 0xffff;
            update_block_entry(inode->i_block[7], (__u8 *)block);
        }
        __u16 *oneLevelIndex = (__u16 *)load_block_entry(block[tar / 256]);
        oneLevelIndex[tar % 256] = added_block_num;
        oneLevelIndex[tar % 256 + 1] = 0xffff;
        update_block_entry(block[tar / 256], (__u8 *)oneLevelIndex);
        inode->i_size = inode->i_blocks * BLOCK_SIZE;
        inode->i_blocks++;
        inode->i_mtime = get_current_time();
        free(block);
        free(oneLevelIndex);
    }
    update_inode_entry(inode_num, inode);
    return 0;
}

// 辅助函数，用于释放数据块同时更新位图
// ext2_free_blocks使用
void free_block_and_update_bitmap(__u16 block_num)
{
    __u8* block = (__u8*)malloc(sizeof(__u8) * BLOCK_SIZE);
    memset(block, 0, sizeof(__u8) * BLOCK_SIZE);
    ext2_free_block_bitmap(block_num);
    update_block_entry(block_num, block);
    free(block);
}

// 辅助函数，用于释放多个数据块同时更新位图
// ext2_free_blocks使用
void free_blocks(__u16 *block_indices, int num_blocks)
{
    for (int i = 0; i < num_blocks && block_indices[i] != 0xffff; i++)
    {
        free_block_and_update_bitmap(block_indices[i]);
    }
}

// 释放特定文件的所有数据块,修改数据位图
__u16 ext2_free_blocks(ext2_inode *inode)
{
    // 释放一个文件的所有数据块
    // 输入文件的inode号，根据inode中的i_block信息释放所有数据块
    // 同时释放数据块位图
    // ext2_inode *inode = load_inode_entry(inode_num);
    __u16 i_block_num = inode->i_blocks;
    __u16 *i_block = inode->i_block;
    __u8 *block = (__u8 *)malloc(sizeof(__u8) * BLOCK_SIZE);
    memset(block, 0, sizeof(__u8) * BLOCK_SIZE);

    if (i_block_num == 0)
    {
        fprintf(stderr, "i_block_num is 0, no data block allocated\n");
        return 0xffff;
    }
    if (i_block_num)
    {
        int block_num = (i_block_num >= 7) ? 6 : i_block_num; // 前6个直接索引，后面的都是间接索引
        for (int i = 0; i < block_num; i++)
        {
            free_block_and_update_bitmap(i_block[i]);
        }
    }
    if (i_block_num >= 7 && i_block_num <= 262)
    {
        __u16 *oneLevelIndex = (__u16 *)load_block_entry(i_block[6]); // load the first level index table
        free_blocks(oneLevelIndex, 256);
        ext2_free_block_bitmap(i_block[6]);
    }
    if (i_block_num > 263 && i_block_num <= 4096)
    {
        __u16 *TwoLevelIndex = (__u16 *)load_block_entry(i_block[7]); // load the second level index table
        for (int k = 0; k < 256; k++)
        {
            if (TwoLevelIndex[k] == 0xffff)
            {
                return 0xffff;
            }
            __u16 *oneLevelIndex = (__u16 *)load_block_entry(TwoLevelIndex[k]);
            free_blocks(oneLevelIndex, 256);
            ext2_free_block_bitmap(TwoLevelIndex[k]);
        }
    }
    free(inode);
    free(block);
    return 0;
}


// 辅助函数，用于直接索引块找到目录
// search_filename和oneLevelIndex_entry_find使用
int direct_entry_find(char *filename, int *pos, int *free_entry, int *offset, __u16 *i_block, __u16 i_block_num)
{
    // pos and offset is the number of data block
    // offset is the offset of the data block
    __u8 *block = (__u8 *)malloc(sizeof(__u8) * BLOCK_SIZE);
    for (int i = 0; i < i_block_num; i++)
    {
        block = load_block_entry(i_block[i]);
        ext2_dir_entry *dir = (ext2_dir_entry *)block;
        int j = 0;
        while (j < BLOCK_SIZE)
        {
            // compare filename
            if (strcmp(dir->name, filename) == 0)
            {
                // if match, return inode number, and set pos to the position of the dir_entry
                if (dir->inode != 0xffff)
                {
                    *pos = i_block[i];
                    *offset = j;
                    *free_entry = -1;
                    return dir->inode;
                }
                fprintf(stderr, "matched inode is 0xffff, the file/directory %s has been deleted\n", filename);
            }
            if (dir->inode == 0xffff && dir->rec_len >= (strlen(filename) + 7) && first_match)
            {
                // 如果当前目录项是空闲的，且有足够的空间存放新文件
                // 返回空闲目录项的位置
                first_match = 0;
                *pos = -1;
                *free_entry = i_block[i];
                *offset = j;
            }
            if (dir->rec_len != 0 && j + dir->rec_len + 7 + strlen(filename) >= BLOCK_SIZE)
            {
                // 如果当块内偏移加上前目录项的长度以及需要分配的目录超过块大小
                // 将当前目录项拓展到块尾，由于没有写入文件系统实际上大小没有改变，外碎片
                // 这样调整才能使得dir->rec_len = 0是遍历完所有目录项的条件
                dir->rec_len = BLOCK_SIZE - j;
            }
            if (dir->rec_len == 0)
            {
                // 注意，这个条件并本来不能作为遍历完所有目录项的条件
                // 由于分配过程中每一块块尾会出现外部碎片，初始化为0，读出来长度为0
                // 分配目录很长的时候，会出现这种外碎片的情况
                if (first_match)
                {
                    // 如果没有找到空闲的目录项，返回该目录项的尾部
                    // 根据该块剩余的大小，不够返回下页，够返回该页的尾部
                    if (j + 7 + strlen(filename) >= BLOCK_SIZE)
                    {
                        // -2 stands for new_page
                        *free_entry = -2;
                        *pos = -1;
                        *offset = 0;
                        first_match = 0;
                    }
                    else
                    {
                        *free_entry = i_block[i];
                        *pos = -1;
                        *offset = j;
                        first_match = 0;
                    }
                    free(block);
                    return 0xffff;
                }
                return 0xffff;
            }
            j += dir->rec_len;
            if (j + 7 >= BLOCK_SIZE)
            {
                // 剩余空间不可能存放一个目录项，直接跳出，加载下一块
                break;
            }
            dir = (ext2_dir_entry *)(block + j);
        }
    }
    *pos = -1;
    *offset = 0;
    *free_entry = -2;
    return 0xffff;
}

// 辅助函数，用于一级索引块找到目录
// search_filename和twoLevelIndex_entry_find使用
int oneLevelIndex_entry_find(char *filename, int *pos, int *free_entry, int *offset, __u16 *oneLevelIndex)
{
    // 调用direct_entry_find实现
    __u16 *i_block = (__u16 *)malloc(sizeof(__u16) * 256);
    __u16 i_block_num = 0;
    for (i_block_num; oneLevelIndex[i_block_num] != 0xffff && i_block_num < 256; i_block_num++)
    {
        i_block[i_block_num] = oneLevelIndex[i_block_num];
    }
    int ret = direct_entry_find(filename, pos, free_entry, offset, i_block, i_block_num);
    free(i_block);
    return ret;
}

// 辅助函数，用于二级索引块找到目录
// search_filename使用
int twoLevelIndex_entry_find(char *filename, int *pos, int *free_entry, int *offset, __u16 *twoLevelIndex)
{
    // 调用oneLevelIndex_entry_find实现
    int oneLevelIndex_num = 0;
    int *oneLevelIndex = (int *)malloc(sizeof(int) * 256);
    for (oneLevelIndex; twoLevelIndex[oneLevelIndex_num] != 0xffff && oneLevelIndex_num < 256; oneLevelIndex_num++)
    {
        oneLevelIndex[oneLevelIndex_num] = twoLevelIndex[oneLevelIndex_num];
    }
    for (int i = 0; i < oneLevelIndex_num; i++)
    {
        int ret = oneLevelIndex_entry_find(filename, pos, free_entry, offset, (__u16 *)load_block_entry(oneLevelIndex[i]));
        free(oneLevelIndex);
        return ret;
    }
}

// 在当前目录中查找文件
__u16 search_filename(char *filename, int *pos, int *free_entry, int *offset)
{
    // 返回indoe号，如果没有找到，返回0xffff
    // pos是找到文件的位置，如果没有找到，返回null
    // free_entry是找到的空闲目录项的位置，如果没有找到，返回null
    *pos = 0;
    *free_entry = 0;
    *offset = 0;
    first_match = 1;
    ext2_inode *inode = load_inode_entry(current_dir);
    __u16 i_block_num = inode->i_blocks;
    __u16 *i_block = inode->i_block;

    // 无论返回值是什么类型，0xffff都是一个异常的返回值
    // 因为我们只有4096个块和4096个索引结点，没有什么可以是0xffff
    // 所以0xffff是一个表示错误或者表格结束的好选择
    if (i_block_num == 0)
    {
        fprintf(stderr, "i_block_num is 0, no data block allocated\n");
        return 0xffff;
    }

    // 直接索引块查找
    if (i_block_num)
    {
        int block_num = (i_block_num < 7) ? i_block_num : 6; // 前6个直接索引，后面的都是间接索引
        int ret = direct_entry_find(filename, pos, free_entry, offset, i_block, block_num);
        if (i_block_num <= 6 || ret != 0xffff)
        {
            return ret;
        }
    }

    // 只有当直接索引用完的时候，才会检查一级索引
    // 一级索引检查：
    if (i_block_num > 6)
    {
        int ret = oneLevelIndex_entry_find(filename, pos, free_entry, offset, (__u16 *)load_block_entry(i_block[6]));
        if (i_block_num <= 262 || ret != 0xffff)
        {
            return ret;
        }
    }

    // 只有当一级索引用完的时候，才会检查二级索引
    // 二级索引检查：
    if (i_block_num > 262)
    {
        int ret = twoLevelIndex_entry_find(filename, pos, free_entry, offset, (__u16 *)load_block_entry(i_block[7]));
        if (i_block_num <= 4096 || ret != 0xffff)
        {
            return ret;
        }
    }
    free(inode);
    return 0xffff;
}

// 检测文件打开ID(fd)是否有效
int test_fd(int fd)
{
    // 检测文件打开ID(fd)是否有效
    // 返回0表示有效，返回-1表示无效
    if (fopen_table[fd].inode_num == 0xffff)
    {
        return -1;
    }
    return fd;
}

// 按索引顺序载入数据块
__u16 load_indexed_data_block(__u16 inode_num, __u16 index)
{
    // 载入指定的inode节点数据块
    // 从0开始的第index块
    // 返回数据块号调用函数去load
    // 有多级索引
    ext2_inode *inode = load_inode_entry(inode_num);
    int i_block_num = inode->i_blocks;
    if (index < 0 || index > i_block_num)
    {
        fprintf(stderr, "load_indexed_data_block() failed, index %d is not valid\n", index);
        return -1;
    }
    if (index < 6)
    {
        return inode->i_block[index];
    }
    else if (index < 262)
    {
        __u16 *oneLevelIndex = (__u16 *)load_block_entry(inode->i_block[6]);
        return oneLevelIndex[index - 6];
    }
    else
    {
        __u16 *TwoLevelIndex = (__u16 *)load_block_entry(inode->i_block[7]);
        __u16 *oneLevelIndex = (__u16 *)load_block_entry(TwoLevelIndex[(index - 262) / 256]);
        return oneLevelIndex[(index - 262) % 256];
    }
}