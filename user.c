#include "types.h"
#include "user.h"
#include "fs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 登录
int login(char* username, char* password)
{
    for (int i = 0; i < MAX_USER_NUM;i++)
    {
        if (strcmp(username, group_desc->user[i].username) == 0)
        {
            if (strcmp(password, group_desc->user[i].password) == 0)
            {
                super_user = (i == 0) ? 1 : 0;
                current_user = group_desc->user[i];
                return 0;
            }
            else
            {
                printf("Password is wrong\n");
                return -1;
            }
        }
    }
    printf("Username is wrong\n");
    return -1;
}

// 修改密码
int change_password(char* username, char* old_password, char* new_password)
{
    if (!super_user && strcmp(username, current_user.username) != 0)
    {
        printf("You don't have the permission to change other's password\n");
        return -1;
    }
    for (int i = 0; i < MAX_USER_NUM; i++)
    {
        if (strcmp(username, group_desc->user[i].username) == 0)
        {
            if (i==0 && current_user.uid != 0)
            {
                printf("You don't have the permission to change root's password\n");
                return -1;
            }
            if (strcmp(old_password, group_desc->user[i].password) == 0 || current_user.uid == 0)
            {
                strcpy(current_user.password, new_password);
                strcpy(group_desc->user[i].password, new_password);
                update_group_desc();
                return 0;
            }
            else
            {
                printf("Password is wrong\n");
                return -1;
            }
        }
    }
    printf("Username is wrong\n");
    return -1;
}

// 退出登录
int logout()
{
    current_user.uid = -1;
    current_user.gid = -1;
    strcpy(current_user.username, "");
    strcpy(current_user.password, "");
    strcpy(current_user.home, "");
    is_login = 0;
    super_user = 0;
    return 0;
}

/*  su权限，限制当前用户使用，提升当前用户权限，需要输入密码
    进行一条重要操作时需要使用，使用后清除权限
    重要操作：格式化，递归删除目录
*/
int su(char* password)
{
    if (super_user)
    {
        printf("You are already super user\n");
        return -1;
    }
    if (strcmp(password, current_user.password) == 0)
    {
        super_user = 1;
        return 0;
    }
    else
    {
        printf("Password is wrong\n");
        return -1;
    }
    printf("Username is wrong\n");
    return -1;
}

// 更改当前用户名
int change_username(char* username)
{
    strcpy(current_user.username, username);
    strcpy(group_desc->user[current_user.uid].username, username);
    update_group_desc();
    return 0;
}

