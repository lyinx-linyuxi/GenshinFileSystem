#include "init.h"
#include "lowlevel.h"
#include "iobasic.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void initialize_disk()
{
    FILE *fp = fopen(fs_file, "rb+");
    if (!fp)
    {
        fp = fopen(fs_file, "wb+");
    }
    __u8 *block = (__u8 *)malloc(sizeof(__u8) * BLOCK_SIZE);
    if (!fp)
    {
        fprintf(stderr, "fopen() failed\n");
    }
    int init_arry[128] = {0};
    fseek(fp, 0, SEEK_SET);
    for (int i = 0; i < 4611; i++)
    {
        fwrite(init_arry, sizeof(int), 128, fp);
    }
    group_desc = (ext2_group_desc *)malloc(sizeof(ext2_group_desc));
    memset(group_desc, 0, sizeof(ext2_group_desc));
    strcpy(group_desc->bg_volume_name, "ext2_fs");
    group_desc->bg_block_bitmap = 1;
    group_desc->bg_inode_bitmap = 2;
    group_desc->bg_inode_table = 3;
    group_desc->bg_free_blocks_count = 4096;
    group_desc->bg_free_inodes_count = 4096;
    group_desc->bg_used_dirs_count = 0;

    // root用户
    group_desc->user[0].gid = 0;
    group_desc->user[0].uid = 0;
    strcpy(group_desc->user[0].username, "root");
    strcpy(group_desc->user[0].password, "root");
    strcpy(group_desc->user[0].home, "/root");
    
    // 默认用户
    // group_desc->user.gid = 0;
    group_desc->user[1].uid = 1;
    strcpy(group_desc->user[1].username, "user");
    strcpy(group_desc->user[1].password, "123456");
    char temp[30] = {0};
    strcpy(temp, "/home");
    strcat(temp, group_desc->user[1].username);
    strcpy(group_desc->user[1].home, temp);
    strcpy(group_desc->bg_pad, "m3");
    strcpy(group_desc->extra_padding, "Just something to fill out the allocated blocks");
    update_group_desc();

    // root directory
    time_t current_time;
    time(&current_time);
    ext2_inode *root = (ext2_inode *)malloc(sizeof(ext2_inode)); // 分配inode
    root->i_mode = (0x0000 | 0x0200) | 0x7;
    root->i_blocks = 1;
    root->i_size = 0;
    root->i_atime = current_time;
    root->i_ctime = current_time;
    root->i_mtime = current_time;
    root->i_dtime = 0;
    root->i_block[0] = DATA_BLOCK_START;
    last_alloc_inode = 0;

    // .
    ext2_dir_entry *root_cur = (ext2_dir_entry *)malloc(sizeof(ext2_dir_entry));
    root_cur->inode = 0;
    strcpy(root_cur->name, ".");
    root_cur->name_len = strlen(root_cur->name);
    root_cur->file_type = 2;
    root_cur->rec_len = 7 + strlen(root_cur->name);
    root->i_size += root_cur->rec_len;

    // ..
    ext2_dir_entry *root_par = (ext2_dir_entry *)malloc(sizeof(ext2_dir_entry));
    root_par->inode = 0;
    strcpy(root_par->name, "..");
    root_par->name_len = strlen(root_par->name);
    root_par->file_type = 2;
    root_par->rec_len = 7 + strlen(root_par->name);
    root->i_size += root_par->rec_len;

    // 修改组描述符的两个位图
    reload_group_desc();
    group_desc->bg_free_blocks_count--;
    group_desc->bg_free_inodes_count--;
    group_desc->bg_used_dirs_count++;
    block = read_block(group_desc->bg_block_bitmap);
    block[0] |= 0x80;
    write_block(group_desc->bg_block_bitmap, block);
    block = read_block(group_desc->bg_inode_bitmap);
    block[0] |= 0x80;
    write_block(group_desc->bg_inode_bitmap, block);
    update_group_desc();

    // 将inode写入inode表中
    block = read_block(INODE_BLOCK_START);
    memcpy(block, root, sizeof(__u8) * 32);
    write_block(INODE_BLOCK_START, block);

    // 将目录项写入数据块中
    block = read_block(DATA_BLOCK_START);
    // block[511] = 66;
    // write_block(0,block);
    memcpy(block, root_cur, sizeof(__u8) * root_cur->rec_len);
    memcpy(block + root_cur->rec_len, root_par, sizeof(__u8) * root_par->rec_len);
    write_block(DATA_BLOCK_START, block);

    free(block);
    free(root);
    free(root_cur);
    free(root_par);
    fclose(fp);
}

void initialize_memory()
{
    group_desc = (ext2_group_desc *)malloc(sizeof(ext2_group_desc));
    reload_group_desc();
    struct open_file init_table;
    init_table.filename = NULL;
    init_table.path = NULL;
    init_table.inode_num = 0xffff;
    init_table.offset = -3;
    for (int i = 0; i < 16; i++)
    {
        fopen_table[i] = init_table;
    }
    is_login = 0;
    super_user = 0;
    last_alloc_inode = 0;
    last_alloc_block = 0;
    current_dir = 0;
    strcpy(current_path, "/");
}