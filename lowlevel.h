#ifndef LOWLEVEL_H
#define LOWLEVEL_H

#include "iobasic.h"

// 底层
/*分配一个新的索引结点*/
__u16 ext2_new_inode();
/*分配一个新的数据块*/
__u16 ext2_alloc_block();
/*释放特定的索引结点,并写入文件系统*/
__u16 ext2_free_inode(__u16 inode_num);
/*释放特定块号的数据块位图*/
__u16 ext2_free_block_bitmap(__u16 data_block_offset);
/*根据多级索引机制更新索引结点的数据块信息域*/
int update_inode_i_block(ext2_inode *inode, int inode_num, __u16 added_block_num);
/*释放特定文件的所有数据块,修改数据位图*/
__u16 ext2_free_blocks(ext2_inode *inode);
/*在当前目录中查找文件*/
__u16 search_filename(char *filename, int *pos, int *free_entry, int *offset);
/* 检测文件打开ID(fd)是否有效. */
int test_fd(int fd);

// 一些辅助函数
/*清空数据块同时更新位图*/
void free_block_and_update_bitmap(__u16 block_num);
/*按照索引顺序载入数据块*/
__u16 load_indexed_data_block(__u16 inode_num, __u16 index);

#endif // LOWLEVEL_H
