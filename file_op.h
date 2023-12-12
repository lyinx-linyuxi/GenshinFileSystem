#ifndef FILE_OP_H
#define FILE_OP_H

#include "lowlevel.h"

/*辅助函数*/
/*创建文件*/
int create_file(char *filename, int permission);
/*创建目录*/
int create_dir(char *dirname, int permission);
/*删除文件*/
int delete_file(char *filename);
/*删除目录*/
int delete_dir(char *dirname);
/*打开文件*/
int open(char* filename);
/*读取文件*/
int read(int fd, __u8 *dest, __u32 size);
/*写入文件*/
int write(int fd, __u8*src, __u32 size);
/*关闭文件*/
void close(int fd);

// 辅助函数
/*一个目录项处理函数，用于ls显示信息*/
void dir_entry_process(ext2_dir_entry *directory);
/*对一个数据块的目录项进行处理*/
void process_dir_block(__u16 block_num, ext2_dir_entry *dir);
/*检查目录是否为空*/
int check_dir_empty(__u16 inode_num);
/*路径解析*/
char** parse_path(char* pathname, int* arg_num);
// 辅助函数，用于用户看到打开了哪些文件
void print_open_file_table();
#endif // FILE_OP_H