/* 文件系统的基本io操作
1. 读取块和写入块
2. 连续的读取块和写入块
3. .....*/

#ifndef _IOBASIC_H_
#define _IOBASIC_H_

#include "types.h"
#include <time.h>

// 实现任意块的读出, 注意这里传入的块号是整体设计的块号，而不是不同类型块组组内的块号
__u8 *read_block(__u16 block_num);
// 实现任意块的写入, 注意这里传入的块号是整体设计的块号，而不是不同类型块组组内的块号
void write_block(__u16 block_num, __u8 *block);
// 实现任意块的任意数目的读出，返回一个指针，读出块数为blocks, 注意这里传入的块号是整体设计的块号，而不是不同类型块组组内的块号
__u8 *read_n_blocks(__u16 block_num, int blocks);
// 实现任意块的任意数目的写入，写入块数为blocks, 注意这里传入的块号是整体设计的块号，而不是不同类型块组组内的块号
void write_n_blocks(__u16 block_num, __u8 *block, int blocks);

/*将内存中的组描述符更新到"硬盘".*/
void update_group_desc();
/*载入可能已更新的组描述符*/
void reload_group_desc();
/*载入特定的索引结点*/
ext2_inode *load_inode_entry(__u16 inode_num);
/*更新特定的索引结点*/
void update_inode_entry(__u16 inode_num, ext2_inode *inode);
/*载入特定的数据块*/
__u8 *load_block_entry(__u16 data_block_offset);
/*更新特定的数据块*/
void update_block_entry(__u16 data_block_offset, __u8 *block);

// 一些上层调用使用的辅助函数
/*返回当前时间*/
time_t get_current_time();
/*加载数据块位图*/
__u8* load_data_bitmap();
/*更新数据块位图*/
void update_data_bitmap(__u8* block);
/*加载索引结点位图*/
__u8* load_inode_bitmap();
/*更新索引结点位图*/
void update_inode_bitmap(__u8* block);
/*检测位图中某个位置是否被使用*/
int check_bitmap(__u8 *block, __u16 positon);
/*从给定位置开始寻找位图中空闲位置*/
__u16 find_bitmap(__u8 *block, __u16 start);
// 锁操作
void wait();
void signal();


#endif