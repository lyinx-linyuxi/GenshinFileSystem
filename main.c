#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "fs.h"
#include "init.h"

#define MAX_LINE 80 /* 80 chars per line, per command */

int main(void)
{
    char* args[MAX_LINE / 2 + 1]; /* command line (of 80) has max of 40 arguments */
    // history feature
    int should_run = 1;
    // initialize_disk();
    initialize_memory();

    while (should_run)
    {
        check_login();
        char indicator = (current_user.uid == 0) ? '#' : '$';
        printf("\033[0;32m[%s@localhost ~]\033[0m:\033[0;34m%s\033[0m%c ", current_user.username, current_path, indicator);
        fflush(stdout);

        /**
         * After reading user input, the steps are:
         * (1) fork a child process
         * (2) the child process will invoke execvp()
         * (3) if command included &, parent will invoke wait()
         */

         // step 1
        char c = 'c';
        int argNums = -1;
        while (c != '\n')
        {
            args[++argNums] = (char*)malloc(sizeof(char) * 30);
            scanf("%s", args[argNums]);
            c = getchar();
        }
        argNums++;
        if (strcmp("ls", args[0]) == 0)
        {
            shell_ls(args, argNums);
            continue;
        }
        if (strcmp("cd", args[0]) == 0)
        {
            shell_cd(args, argNums);
            continue;
        }
        if (strcmp("mkdir", args[0]) == 0)
        {
            shell_mkdir(args, argNums);
            continue;
        }
        if (strcmp("rmdir", args[0]) == 0)
        {
            shell_rmdir(args, argNums);
            continue;
        }
        if (strcmp("touch", args[0]) == 0)
        {
            shell_touch(args, argNums);
            continue;
        }
        if (strcmp("rm", args[0]) == 0)
        {
            shell_rm(args, argNums);
            continue;
        }
        if (strcmp("open", args[0]) == 0)
        {
            shell_open(args, argNums);
            continue;
        }
        if (strcmp("exit", args[0]) == 0 && argNums == 1)
        {
            should_run = 0;
            break;
        }
        if (strcmp("write", args[0]) == 0)
        {
            shell_write(args, argNums);
            continue;
        }
        if (strcmp("chmod", args[0]) == 0)
        {
            shell_attrib(args, argNums);
            continue;
        }
        if (strcmp("read", args[0]) == 0)
        {
            shell_read(args, argNums);
            continue;
        }
        if (strcmp("close", args[0]) == 0)
        {
            shell_close(args, argNums);
            continue;
        }
        if (strcmp("rmdir_rec", args[0]) == 0)
        {
            shell_rmdir_rec(args, argNums);
            continue;
        }
        if (strcmp("format", args[0]) == 0)
        {
            format();
            continue;
        }
        if (strcmp("su", args[0]) == 0)
        {
            char password[16];
            printf("Current user: %s\n", current_user.username);
            printf("Password: ");
            scanf("%s", password);
            su(password);
            continue;
        }
        if (strcmp("logout", args[0]) == 0)
        {
            logout();
            continue;
        }
        if (strcmp("name", args[0]) == 0)
        {
            if(current_user.uid == 0){
                fprintf(stderr,"root user's name can only be \"root\"\n");
                continue;
            }
            char username[16];
            printf("Current user: %s\n", current_user.username);
            printf("Username: ");
            scanf("%s", username);
            change_username(username);
            continue;
        }
        if (strcmp("passwd", args[0]) == 0)
        {
            char old_password[16];
            char new_password[16];
            char target_username[16];
            printf("Current user: %s\n", current_user.username);
            if (current_user.uid == 0)
            {
                printf("Target Username: ");
                scanf("%s", target_username);
            }
            else
            {
                strcpy(target_username, current_user.username);
            }
            if (current_user.uid != 0)
            {
                printf("Old Password: ");
                scanf("%s", old_password);
            }
            else
            {
                strcpy(old_password, current_user.password);
            }
            printf("New Password: ");
            scanf("%s", new_password);
            change_password(target_username, old_password, new_password);
            continue;
        }
        if (strcmp("cls", args[0]) == 0)
        {
            if (strcmp(OS, "Windows") == 0)
            {
                system("cls");
            }
            else if (strcmp(OS, "Linux") == 0)
            {
                system("clear");
            }
            else
            {
                printf("Unknown OS\n");
            }
            continue;
        }
        if (strcmp("help", args[0]) == 0)
        {
            help();
            continue;
        }
        if (strcmp("ftable",args[0]) == 0)
        {
            print_open_file_table();
            continue;
        }

        fflush(stdout);
    }

    return 0;
}
