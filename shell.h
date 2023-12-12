/*
本文件是有关shell内的操作，由于在使用shell调用文件系统操作时有很多的判断
不便于调试和修改，分文件写
*/


#ifndef _SHELL_H_
#define _SHELL_H_

#include "types.h"
#include "fs.h"

#define MAX_BUFFER_SIZE 4096
#ifdef _WIN32
#define OS "Windows"
#elif __linux__
#define OS "Linux"
#else
#define OS "Unknown"
#endif

// 用于shell的文件系统操作
// 检查是否登录
void check_login();
// 用于shell的ls操作
void shell_ls(char** args, int argNums);
// 用于shell的cd操作
void shell_cd(char** args, int argNums);
// 用于shell的mkdir操作
void shell_mkdir(char** args, int argNums);
// 用于shell的rmdir操作
void shell_rmdir(char** args, int argNums);
// 用于shell的touch操作
void shell_touch(char** args, int argNums);
// 用于shell的rm操作
void shell_rm(char** args, int argNums);
// 用于shell的attrib操作
void shell_attrib(char** args, int argNums);
// 用于shell的open操作
void shell_open(char** args, int argNums);
// 用于shell的write操作
void shell_write(char** args, int argNums);
// 用于shell的read操作
void shell_read(char** args, int argNums);
// 用于shell的close操作
void shell_close(char** args, int argNums);
// 用于shell的rmdir_rec操作
void shell_rmdir_rec(char** args, int argNums);
// 用户帮助help
void help();

#endif