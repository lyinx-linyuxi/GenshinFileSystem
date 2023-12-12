#pragma once
#ifndef _TYPES_H_
#define _TYPES_H_

#include "user.h"

typedef char __s8;
typedef unsigned char __u8;

typedef short __s16;
typedef unsigned short __u16;

typedef int __s32;
typedef unsigned int __u32;

typedef long long __s64;
typedef unsigned long long __u64;

#define DATA_BLOCK_NUM 512 * 8
#define INOE_BLOCK_NUM 512 // 512*8*64/512
#define EXT2_NAME_LEN 256  //
#define BLOCK_SIZE 512
#define INODE_BLOCK_START 3
#define DATA_BLOCK_START 515
#define MAX_USER_NUM 4
// 1个组描述符
// 1个inode索引和1个数据块索引
// 512个inode块
// 4096个数据块

// 打开文件表项
struct open_file
{
    char* filename;
    __u16 inode_num;
    char* path;
    int offset;
};

__u8 gd_lock;
char fs_file[10];
struct open_file fopen_table[16];     /*文件打开表，最多可以同时打开16个文件*/
unsigned short last_alloc_inode;    /*上次分配的索引结点号*/
unsigned short last_alloc_block;    /*上次分配的数据块号*/
unsigned short current_dir;         /*当前目录(索引结点）*/
char current_path[256];             /*当前路径(字符串) */
struct ext2_group_desc *group_desc; /*组描述符*/
// 新增
char is_login;                      /*是否登录*/
struct user current_user;           /*当前用户*/
char super_user;                    /*是否为超级用户*/

typedef struct ext2_group_desc
{
    // 类型  域	                   释意                     bytes
    char bg_volume_name[16];    // 卷名                   16
    __u16 bg_block_bitmap;      // 保存块位图的块号          2
    __u16 bg_inode_bitmap;      // 保存索引结点位图的块号     2
    __u16 bg_inode_table;       // 索引结点表的起始块号       2
    __u16 bg_free_blocks_count; // 本组空闲块的个数           2
    __u16 bg_free_inodes_count; // 本组空闲索引结点的个数     2
    __u16 bg_used_dirs_count;   // 本组目录的个数             2
    char bg_pad[4];             // 填充(0xff)                 4
    // 合计32个字节，由于只有一个组，且占用一个块，故需要填充剩下的512-32=480字节。
    struct user user[4]; // 用户，64字节，4个用户，256字节，剩下的224字节填充
    char extra_padding[224];
} ext2_group_desc;

typedef struct ext2_inode
{
    // 类型	  域	                释意           字节长度
    __u16 i_mode;     // 文件类型及访问权限         2
    __u16 i_blocks;   // 文件的数据块个数(实际上用的时候作为了iblock的长度)           2
    __u32 i_size;     // 大小(字节)                 4
    __u64 i_atime;    // 访问时间                   8
    __u64 i_ctime;    // 创建时间                   8
    __u64 i_mtime;    // 修改时间                   8
    __u64 i_dtime;    // 删除时间                   8
    __u16 i_block[8]; // 指向数据块的指针          2*8=16
    char i_pad[8];    // 填充1(0xff)                8
    // 合计64个字节
} ext2_inode;

typedef struct ext2_dir_entry
{
    //  Type	Field	                    释意                Bytes
    __u16 inode;              // 索引节点号
    __u16 rec_len;            // 目录项长度
    __u8 name_len;            // 文件名长度
    __u8 file_type;           // 文件类型(1:普通文件，2:目录…)
    char name[EXT2_NAME_LEN]; // 文件名
} ext2_dir_entry;

#endif