#ifndef __USER_H_
#define __USER_H_


#include "types.h"
#include <string.h>

// 用户
struct user
{
    // 注意对齐标准，计组学的时候讲过
    char username[16];// 用户名
    char password[16];// 密码
    int uid; // 用户ID，0为root
    int gid; // 用户组ID，暂时不用，可以拓展
    char home[24]; // 用户主目录.暂时不用，可以拓展
};

// 登录
int login(char *username, char *password);
// 修改密码
int change_password(char *username, char *old_password, char *new_password);
// 退出登录
int logout();
// su权限
int su(char *password);
// 更改当前用户名
int change_username(char *username);



#endif