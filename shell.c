#include "init.h"
#include "shell.h"
#include "fs.h"
#include "file_op.h"
#include "lowlevel.h"
#include "iobasic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


int chance = 3; // 登录机会
// 检查是否登录
void check_login()
{
    char username[16];
    char password[16];
    while (is_login == 0)
    {
        printf("********************PLEASE LOGIN********************\n");
        if (chance == 0)
        {
            printf("Thanks for using ext2 file system\n");
            exit(0);
        }
        printf("Welcom to use ext2 file system\n");
        printf("Please login\n");
        printf("If you don't have an account, try default user\n(username:\"user\" and password: \"123456\")\n");
        printf("username:");
        fflush(stdout);
        scanf("%s", username);
        printf("Password:");
        fflush(stdout);
        scanf("%s", password);
        if (login(username, password) == 0)
        {
            printf("LOGIN SUCCESSFUL\n");
            is_login = 1;
        }
        else
        {
            printf("Username or password is wrong\n");
            printf("\x1b[31mLOGIN FAILED\x1b[0m\n");
            chance--;
        }
    }
}

// 用于shell的ls操作
void shell_ls(char** args, int argNums)
{
    if (argNums == 1)
    {
        ls();
    }
    else if (argNums == 2)
    {
        int depth;
        int no_ls = 0;
        char** ret = parse_path(args[1], &depth);
        if (ret == NULL)
        {
            return;
        }
        char* path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
        int inode_num = current_dir;
        if (depth == 0)
        {
            ls();
        }
        else
        {
            strcpy(path, current_path);
            for (int i = 0; i < depth; i++)
            {
                int retval = cd(ret[i]);
                if (retval == -1)
                {
                    no_ls = 1;
                    current_dir = inode_num;
                    strcpy(current_path, path);
                    break;
                }
            }
            no_ls == 1 ? 0 : ls();
        }
        current_dir = inode_num;
        strcpy(current_path, path);
    }
}

// 用于shell的cd操作
void shell_cd(char** args, int argNums)
{
    if (argNums == 1)
    {
        printf("\x1b[31minvalid parameter\x1b[0m!\n");
        return;
    }
    __u16 inode_num = current_dir;
    char* path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
    strcpy(path, current_path);
    int arg_num = 0;
    char** arg = parse_path(args[1], &arg_num);
    if (arg == NULL)
    {
        return;
    }
    if (arg_num == 0)
    {
        return;
    }
    for (int i = 0; i < arg_num; i++)
    {
        int ret = cd(arg[i]);
        if (ret == -1)
        {
            current_dir = inode_num;
            strcpy(current_path, path);
            break;
        }
    }
}

// 用于shell的mkdir操作
void shell_mkdir(char** args, int argNums)
{
    if (argNums == 1)
    {
        printf("\x1b[31minvalid parameter!\x1b[0m\n");
        return;
    }
    else if (argNums == 2)
    {
        int fg = 1;
        int depth;
        char** ret = parse_path(args[1], &depth);
        if (ret == NULL)
        {
            return;
        }
        char* path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
        int inode_num = current_dir;
        strcpy(path, current_path);
        for (int i = 0; i < depth - 1; i++)
        {
            int retval = cd(ret[i]);
            if (retval == -1)
            {
                current_dir = inode_num;
                strcpy(current_path, path);
                fg = 0;
                break;
            }
        }
        fg == 1 ? mkdir(ret[depth - 1]) : 0;
        current_dir = inode_num;
        strcpy(current_path, path);
    }
}

// 用于shell的rmdir操作
void shell_rmdir(char** args, int argNums)
{
    if (argNums == 1)
    {
        printf("\x1b[31minvalid parameter!\x1b[0m\n");
        return;
    }
    else if (argNums == 2)
    {
        int fg = 1;
        int depth;
        char** ret = parse_path(args[1], &depth);
        if (ret == NULL)
        {
            return;
        }
        char* path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
        int inode_num = current_dir;
        strcpy(path, current_path);
        for (int i = 0; i < depth - 1; i++)
        {
            int retval = cd(ret[i]);
            if (retval == -1)
            {
                current_dir = inode_num;
                strcpy(current_path, path);
                fg = 0;
                break;
            }
        }
        fg == 1 ? rmdir(ret[depth - 1]) : 0;
        current_dir = inode_num;
        strcpy(current_path, path);
    }
}

// 用于shell的touch操作
void shell_touch(char** args, int argNums)
{
    if (argNums == 1)
    {
        printf("\x1b[31minvalid parameter!\x1b[0m\n");
        return;
    }
    else if (argNums == 2)
    {
        int fg = 1;
        int depth;
        char** ret = parse_path(args[1], &depth);
        if (ret == NULL)
        {
            return;
        }
        char* path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
        int inode_num = current_dir;
        strcpy(path, current_path);
        for (int i = 0; i < depth - 1; i++)
        {
            int retval = cd(ret[i]);
            if (retval == -1)
            {
                current_dir = inode_num;
                strcpy(current_path, path);
                fg = 0;
                break;
            }
        }
        fg == 1 ? touch(ret[depth - 1]) : 0;
        current_dir = inode_num;
        strcpy(current_path, path);
    }
}

// 用于shell的rm操作
void shell_rm(char** args, int argNums)
{
    if (argNums == 1)
    {
        printf("\x1b[31minvalid parameter!\x1b[0m\n");
        return;
    }
    else if (argNums == 2)
    {
        int fg = 1;
        int depth;
        char** ret = parse_path(args[1], &depth);
        if (ret == NULL)
        {
            return;
        }
        char* path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
        int inode_num = current_dir;
        strcpy(path, current_path);
        for (int i = 0; i < depth - 1; i++)
        {
            int retval = cd(ret[i]);
            if (retval == -1)
            {
                current_dir = inode_num;
                strcpy(current_path, path);
                fg = 0;
                break;
            }
        }
        fg == 1 ? rm(ret[depth - 1]) : 0;
        current_dir = inode_num;
        strcpy(current_path, path);
    }
}

// 用于shell的attrib操作
void shell_attrib(char** args, int argNums)
{
    if (argNums != 3)
    {
        printf("\x1b[31minvalid parameter!\x1b[0m\n");
        return;
    }
    printf("\x1b[34mFile need to be in current directory!\x1b[0m\n");
    char* endptr; // 用于指示转换结束的指针
    long permission = strtol(args[2], &endptr, 10);

    // 检查转换是否成功
    if (*endptr != '\0' && !isspace(*endptr))
    {
        printf("无效的输入: %s\n", args[2]);
    }
    attrib(args[1], permission);
}

// 用于shell的open操作
void shell_open(char** args, int argNums)
{
    if (argNums == 1)
    {
        printf("\x1b[31minvalid parameter!\x1b[0m\n");
        return;
    }
    else if (argNums == 2)
    {
        int fg = 1;
        int depth;
        char** ret = parse_path(args[1], &depth);
        if (ret == NULL)
        {
            return;
        }
        char* path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
        int inode_num = current_dir;
        strcpy(path, current_path);
        for (int i = 0; i < depth - 1; i++)
        {
            int retval = cd(ret[i]);
            if (retval == -1)
            {
                current_dir = inode_num;
                strcpy(current_path, path);
                fg = 0;
                break;
            }
        }
        fg == 1 ? open(ret[depth - 1]) : 0;
        current_dir = inode_num;
        strcpy(current_path, path);
    }
}

// 用于shell的write操作
void shell_write(char** args, int argNums)
{
    char* filename = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
    int fd;
    if (argNums == 1)
    {
        printf("\x1b[31minvalid parameter!\x1b[0m\n");
        return;
    }
    else if (argNums == 2)
    {
        int depth;
        char** ret = parse_path(args[1], &depth);
        if (ret == NULL)
        {
            return;
        }
        char* saved_path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
        int inode_num = current_dir;
        strcpy(saved_path, current_path);
        for (int i = 0; i < depth - 1; i++)
        {
            int retval = cd(ret[i]);
            if (retval == -1)
            {
                current_dir = inode_num;
                strcpy(current_path, saved_path);
                return;
            }
        }
        strcpy(filename, ret[depth - 1]);
        fd = get_open_fd(filename);
        current_dir = inode_num;
        strcpy(current_path, saved_path);
        if (fd == -1)
        {
            fprintf(stderr, "file not open!\n");
            return;
        }
    }
    // 重要，实际上一次写入的buffer最大为4094在windows下,4095在linux下
    __u8* src = (__u8*)malloc(sizeof(char) * MAX_BUFFER_SIZE);
    printf("Please input content:\n");
    fflush(stdout);
    int num = 0;
    char c = getchar();
    while (c != '\n')
    {
        src[num++] = c;
        c = getchar();
    }
    printf("Please input write size(0 stands for default size):\n");
    printf("[default size = %d, max size = %d] size = ", num, MAX_BUFFER_SIZE);
    int temp = num;
    int size = temp;
    scanf("%d", &temp);
    if (temp != 0)
    {
        size = temp;
    }
    printf("size = %d\n", size);
    ext2_inode* inode = load_inode_entry(fopen_table[fd].inode_num);
    // 是否需要移动文件指针
    fopen_table[fd].offset = inode->i_size; // 一般从尾开始写
    free(inode);
    printf("Do you want to move file pointer?(y/n)\n");
    printf("current file pointer = %d\n", fopen_table[fd].offset);
    fflush(stdout);
    char choice[10];
    scanf("%s", choice);
    printf("choice = %s\n", choice);
    if (strcmp("yes", choice) == 0 || strcmp("y", choice) == 0)
    {
        printf("Please input offset:\n");
        fflush(stdout);
        scanf("%d", &fopen_table[fd].offset);
    }
    else
    {
        printf("File pointer will not be moved!\n");
    }
    int ret = write(fd, src, size);
    if (ret != -1)
    {
        printf("Write success!\n");
    }
    else
    {
        printf("\x1b[31mWrite failed!\x1b[0m\n");
    }
}

// 用于shell的read操作
void shell_read(char** args, int argNums)
{
    int fd;
    char* filename = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
    if (argNums == 1)
    {
        printf("\x1b[31minvalid parameter!\x1b[0m\n");
        return;
    }
    else if (argNums == 2)
    {
        int fg = 1;
        int depth;
        char** ret = parse_path(args[1], &depth);
        if (ret == NULL)
        {
            return;
        }
        char* saved_path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
        int inode_num = current_dir;
        strcpy(saved_path, current_path);
        for (int i = 0; i < depth - 1; i++)
        {
            int retval = cd(ret[i]);
            if (retval == -1)
            {
                current_dir = inode_num;
                strcpy(current_path, saved_path);
                fg = 0;
                return;
            }
        }
        strcpy(filename, ret[depth - 1]);
        fd = get_open_fd(filename);
        current_dir = inode_num;
        strcpy(current_path, saved_path);
        if (fd == -1)
        {
            fprintf(stderr, "file not open!\n");
            return;
        }
    }
    __u8* dst = (char*)malloc(sizeof(char) * MAX_BUFFER_SIZE);
    printf("Please input read size(0 stands for default size):\n");
    printf("[default size = %d, max size = %d] size = ", load_inode_entry(fopen_table[fd].inode_num)->i_size, MAX_BUFFER_SIZE);
    int size = 0;
    scanf("%d", &size);
    if (size == 0)
    {
        size = load_inode_entry(fopen_table[fd].inode_num)->i_size;
    }
    printf("size = %d\n", size);
    // 是否需要移动文件指针
    fopen_table[fd].offset = 0; // 一般从头开始读
    printf("Do you want to move file pointer?(y/n)\n");
    printf("current file pointer = %d\n", fopen_table[fd].offset);
    fflush(stdout);
    char choice[10];
    scanf("%s", choice);
    if (strcmp(choice, "yes") == 0 || strcmp(choice, "y") == 0)
    {
        printf("Please input offset:\n");
        fflush(stdout);
        scanf("%d", &fopen_table[fd].offset);
    }
    else
    {
        printf("File pointer will not be moved!\n");
    }
    int ret = read(fd, dst, size);
    dst[ret] = '\0';
    if (ret != -1)
    {
        printf("Read success!\n");
        printf("\x1b[34mContent:\x1b[0m\n");
        puts(dst);
    }
    else
    {
        printf("\x1b[31mRead failed!\x1b[0m\n");
    }
}

// 用于shell的close操作
void shell_close(char** args, int argNums)
{
    int fd;
    char* filename = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
    if (argNums == 1)
    {
        printf("\x1b[31minvalid parameter!\x1b[0m\n");
        return;
    }
    else if (argNums == 2)
    {
        int fg = 1;
        int depth;
        char** ret = parse_path(args[1], &depth);
        if (ret == NULL)
        {
            return;
        }
        char* path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
        int inode_num = current_dir;
        strcpy(path, current_path);
        for (int i = 0; i < depth - 1; i++)
        {
            int retval = cd(ret[i]);
            if (retval == -1)
            {
                current_dir = inode_num;
                strcpy(current_path, path);
                fg = 0;
                break;
            }
        }
        fd = fg == 1 ? get_open_fd(ret[depth - 1]) : -1;
        fg == 1 ? close(fd) : 0;
        current_dir = inode_num;
        strcpy(current_path, path);
    }
}

// 用于shell的rmdir_rec操作
void shell_rmdir_rec(char** args, int argNums)
{
    if (argNums == 1)
    {
        printf("\x1b[31minvalid parameter!\x1b[0m\n");
        return;
    }
    else if (argNums == 2)
    {
        int fg = 1;
        int depth;
        char** ret = parse_path(args[1], &depth);
        if (ret == NULL)
        {
            return;
        }
        char* path = (char*)malloc(sizeof(char) * EXT2_NAME_LEN);
        int inode_num = current_dir;
        strcpy(path, current_path);
        for (int i = 0; i < depth - 1; i++)
        {
            int retval = cd(ret[i]);
            if (retval == -1)
            {
                current_dir = inode_num;
                strcpy(current_path, path);
                fg = 0;
                break;
            }
        }
        fg == 1 ? rmdir_recursive(ret[depth - 1]) : 0;
        current_dir = inode_num;
        strcpy(current_path, path);
        super_user = 0;
    }
}

void help()
{
    printf(" *\n");
    printf(" | EXT2 File System Commands\n");
    printf(" |------------------------------------------------------------------|\n");
    printf(" | 01.list items       : ls         | 02.change dir     : cd        |\n");
    printf(" | 03.create file      : touch      | 04.delete file    : rm        |\n");
    printf(" | 05.create dir       : mkdir      | 06.delete dir     : rmdir     |\n");
    printf(" | 07.open file        : open       | 08.write file     : write     |\n");
    printf(" | 09.read file        : read       | 10.close file     : close     |\n");
    printf(" |------------------------------------------------------------------|\n");
    printf(" | 11.change mode      : chmod      | 12.rmdir recursive: rmdir_rec |\n");
    printf(" | 13.format disk      : format     | 14.logoff system  : exit      |\n");
    printf(" | 15.open this table  : help       | 16.change password: passwd    |\n");
    printf(" | 17.change user name : name       | 18.super user     : su        |\n");
    printf(" | 19.logout           : logout     | 20.list open file : ftable    |\n");
    printf(" | 21.clear screen     : cls        |                               |\n");
    printf(" |------------------------------------------------------------------|\n");
    printf(" *\n");
}
