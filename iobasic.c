#include "iobasic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__u8 gd_lock = 0;

void wait()
{
    while (gd_lock)
        ;
    gd_lock = 1;
}

void signal()
{
    gd_lock = 0;
}

// 注意块号的计算，从0开始
// 0号块为组描述符
// 1号块为数据块位图
// 2号块为inode位图
// 3号块~514号块为inode表
// 515号块~4610号块为数据块

__u8 *read_block(__u16 block_num)
{
    // 实现任意块的读出
    // 注意这里传入的块号是整体设计的块号，而不是不同类型块组组内的块号
    __u8 *block = (__u8 *)malloc(sizeof(__u8) * BLOCK_SIZE);
    FILE *fp = fopen(fs_file, "rb+");
    if (!fp)
    {
        fprintf(stderr, "fopen() failed");
    }
    fseek(fp, block_num * BLOCK_SIZE, SEEK_SET);
    fread(block, sizeof(__u8), BLOCK_SIZE, fp);
    fclose(fp);
    return block;
}

void write_block(__u16 block_num, __u8 *block)
{
    // 实现任意块的写入
    // 注意这里传入的块号是整体设计的块号，而不是不同类型块组组内的块号
    FILE *fp = fopen(fs_file, "rb+");
    if (!fp)
    {
        fprintf(stderr, "fopen() failed\n");
    }
    fseek(fp, block_num * BLOCK_SIZE, SEEK_SET);
    fwrite(block, sizeof(__u8), BLOCK_SIZE, fp);
    fclose(fp);
}

__u8 *read_n_blocks(__u16 block_num, int blocks)
{
    // 实现任意块的任意数目的读出，返回一个指针，读出块数为blocks
    // 注意这里传入的块号是整体设计的块号，而不是不同类型块组组内的块号
    __u8 *block = (__u8 *)malloc(sizeof(__u8) * BLOCK_SIZE * blocks);
    FILE *fp = fopen(fs_file, "rb+");
    if (!fp)
    {
        fprintf(stderr, "fopen() failed");
    }
    fseek(fp, block_num * BLOCK_SIZE, SEEK_SET);
    fread(block, sizeof(__u8), BLOCK_SIZE * blocks, fp);
    fclose(fp);
    return block;
}


// 实现任意块的任意数目的写入，写入块数为blocks
// 注意这里传入的块号是整体设计的块号，而不是不同类型块组组内的块号
void write_n_blocks(__u16 block_num, __u8 *block, int blocks)
{
    FILE *fp = fopen(fs_file, "rb+");
    if (!fp)
    {
        fprintf(stderr, "fopen() failed\n");
    }
    fseek(fp, block_num * BLOCK_SIZE, SEEK_SET);
    fwrite(block, sizeof(__u8), BLOCK_SIZE * blocks, fp);
    fclose(fp);
}

// 将内存中的组描述符更新到"硬盘".
void update_group_desc()
{
    wait();
    FILE *fp = fopen(fs_file, "rb+");
    if (!fp)
    {
        fprintf(stderr, "fopen() failed\n");
    }
    fseek(fp, 0, SEEK_SET);
    fwrite(group_desc, sizeof(ext2_group_desc), 1, fp);
    fclose(fp);
    signal();
}

// 载入可能已更新的组描述符
void reload_group_desc()
{
    FILE *fp = fopen(fs_file, "rb+");
    if (!fp)
    {
        fprintf(stderr, "fopen() failed\n");
    }
    fseek(fp, 0, SEEK_SET);
    fread(group_desc, sizeof(ext2_group_desc), 1, fp);
    fclose(fp);
}

// 载入特定的索引结点
ext2_inode *load_inode_entry(__u16 inode_num)
{
    // 传入inode号即可，函数内部会自动计算inode所在的块号,并对应整体设计的块号
    ext2_inode *inode = (ext2_inode *)malloc(sizeof(ext2_inode));
    __u8 *block = read_block(INODE_BLOCK_START + inode_num / 8);
    memcpy(inode, block + (inode_num % 8) * 64, sizeof(ext2_inode));
    return inode;
}

// 更新特定的索引结点
void update_inode_entry(__u16 inode_num, ext2_inode *inode)
{
    // 传入inode号即可，函数内部会自动计算inode所在的块号,并对应整体设计的块号
    __u8 *block = read_block(INODE_BLOCK_START + inode_num / 8);
    memcpy(block + (inode_num % 8) * 64, inode, sizeof(ext2_inode));
    write_block(INODE_BLOCK_START + inode_num / 8, block);
}

// 载入特定的数据块
__u8 *load_block_entry(__u16 data_block_offset)
{
    // 注意传入的是数据块相对于起始数据块的偏移量
    // 例如传入0，表示读取起始数据块，对应整体设计的515块
    // 传入1表示读取起始数据块之后的第一个数据块，对应整体设计的516块
    __u8 *block = read_block(DATA_BLOCK_START + data_block_offset);
    return block;
}

// 更新特定的数据块
void update_block_entry(__u16 data_block_offset, __u8 *block)
{
    // 注意传入的是数据块相对于起始数据块的偏移量
    // 例如传入0，表示读取起始数据块，对应整体设计的515块
    // 传入1表示读取起始数据块之后的第一个数据块，对应整体设计的516块
    write_block(DATA_BLOCK_START + data_block_offset, block);
}

// 返回当前时间
time_t get_current_time()
{
    time_t current_time;
    time(&current_time);
    return current_time;
}

// 加载数据块位图
__u8* load_data_bitmap()
{
    __u8* block = read_block(group_desc->bg_block_bitmap);
    return block;
}

// 更新数据块位图
void update_data_bitmap(__u8* block)
{
    write_block(group_desc->bg_block_bitmap, block);
    return;
}

// 加载索引结点位图
__u8* load_inode_bitmap()
{
    __u8* block = read_block(group_desc->bg_inode_bitmap);
    return block;
}

// 更新索引结点位图
void update_inode_bitmap(__u8* block)
{
    write_block(group_desc->bg_inode_bitmap, block);
    return;
}

// 检测位图中某个位置是否被使用
int check_bitmap(__u8 *block, __u16 positon)
{
    // 检测某个位置对一个块是否被占用
    // 占用返回1，空闲返回0
    return block[positon / 8] & (0x80 >> (positon % 8));
}

// 从给定位置开始寻找位图中空闲位置
__u16 find_bitmap(__u8 *block, __u16 start)
{
    // 传入一个块，返回该块中第一个为0的位的索引
    // 如果块中所有位都为1，则返回16位最大值,即0xffff,-1
    __u16 count = start / 8;
    while (count < 256)
    {
        if (block[count] != 0xff)
        {
            break;
        }
        count++;
    }
    if (count == 256)
    {
        return 0xffff;
    }
    for (int i = 0; i < 8; i++)
    {
        if ((block[count] & (0x80 >> i)) == 0)
        {
            return count * 8 + i;
        }
    }
    return 0xffff;
}