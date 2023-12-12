#ifndef _FS_H_
#define _FS_H_


#include "init.h"
#include "file_op.h"

// 初始化文件系统函数
extern void initialize_disk();
extern void initialize_memory();

// 底层
/* 将内存中的组描述符更新到"硬盘". */
extern void update_group_desc();
/* 载入可能已更新的组描述符. */
extern void reload_group_desc();
/* 载入特定的索引结点. */
extern ext2_inode* load_inode_entry(__u16 inode_num);
/* 更新特定的索引结点. */
extern void update_inode_entry(__u16 inode_num, ext2_inode* inode);
/* 载入特定的数据块. */
extern __u8* load_block_entry(__u16 data_block_offset);
/* 更新特定的数据块. */
extern void update_block_entry(__u16 data_block_offset, __u8* block);
/* 根据多级索引机制更新索引结点的数据块信息域. */
extern int update_inode_i_block(ext2_inode* inode, int inode_num, __u16 added_block_num);
/* 分配一个新的索引结点. */
extern __u16 ext2_new_inode();
/* 分配一个新的数据块. */
extern __u16 ext2_alloc_block();
/* 释放特定的索引结点,并写入文件系统. */
extern __u16 ext2_free_inode(__u16 inode_num);
/* 释放特定块号的数据块位图. */
extern __u16 ext2_free_block_bitmap(__u16 data_block_offset);
/* 释放特定文件的所有数据块,修改数据位图. */
extern __u16 ext2_free_blocks(ext2_inode* inode);
/* 在当前目录中查找文件. */
extern __u16 search_filename(char* filename, int* pos, int* free_entry, int* offset);
/* 检测文件打开ID(fd)是否有效. */
extern int test_fd(int fd);


// 文件基本操作
/*打开文件*/
extern int open(char* filename);
/*关闭文件*/
extern void close(int fd);
/*读取文件*/
extern int read(int fd, __u8* dest, __u32 size);
/*写入文件*/
extern int write(int fd, __u8* src, __u32 size);
/*创建文件*/
extern int create_file(char* filename, int permission);
/*创建目录*/
extern int create_dir(char* dirname, int permission);
/*删除文件*/
extern int delete_file(char* filename);
/*删除目录*/
extern int delete_dir(char* dirname);
// 一些辅助函数
/*路径解析*/
extern char** parse_path(char* pathname, int* arg_num);
/*通过文件名获得fd*/
extern int get_open_fd(char* filename);
/*一个目录项处理函数，用于ls显示信息*/
extern void dir_entry_process(ext2_dir_entry* directory);
/*对一个数据块的目录项进行处理*/
extern void process_dir_block(__u16 block_num, ext2_dir_entry* dir);


// 文件系统操作
/*列出文件*/
void ls();
/*创建目录*/
void mkdir(char* filename);
/*删除目录*/
void rmdir(char* filename);
/*改变当前目录*/
int cd(char* filename);
/*改变文件权限*/
void attrib(char* filename, int permission);
/*创建文件*/
void touch(char* filename);
/*删除文件*/
void rm(char* filename);
/*格式化文件系统*/
void format();
/*递归删除目录*/
void rmdir_recursive(char* dirname);


// 用户管理
// 登录
extern int login(char *username, char *password);
// 修改密码
extern int change_password(char *username, char *old_password, char *new_password);
// 退出登录
extern int logout();
// su权限
extern int su(char *password);
// 更改当前用户名
extern int change_username(char *username);

// 初始化
extern void initialize_disk();
extern void initialize_memory();

#endif