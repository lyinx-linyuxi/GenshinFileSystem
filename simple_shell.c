/**
 * Simple shell interface program.
 *
 * Operating System Concepts - Tenth Edition
 * Copyright John Wiley & Sons - 2018
 * modified by lin 2023/9/30
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command */
#define MAX_HISTORY_NUM 100
#define READ_END 0
#define WRITE_END 1

void Save_History(int *his_now, char ***history, char **args, int *his_argnum, int argNums);
void Print_History(int his_now, int *his_argnum, char ***history);

int main(void)
{
    char *args[MAX_LINE / 2 + 1]; /* command line (of 80) has max of 40 arguments */
    // history feature
    char ***history = (char ***)malloc(sizeof(char **) * MAX_HISTORY_NUM);
    int his_now = 0;
    int *his_argnum = (int *)malloc(sizeof(int) * MAX_HISTORY_NUM);
    int should_run = 1;

    while (should_run)
    {
        printf("osh>");
        fflush(stdout);
        char NEED_CHILD = 1;
        char WAIT_CHILD = 1; // '&'
        // redirection '>' '<'
        char OUTPUT_REDIRECTION = -1;
        char INPUT_REDIRECTION = -1;
        // pipe '|'
        char PIPE_NEED = 0;
        char *output_file;
        char *input_file;
        int arg1num;
        int arg2num;
        char **arg1;
        char **arg2;
        /**
         * After reading user input, the steps are:
         * (1) fork a child process
         * (2) the child process will invoke execvp()
         * (3) if command included &, parent will invoke wait()
         */

        // step 1
        char c = 'a';
        int argNums = -1;
        while (c != '\n')
        {
            args[++argNums] = (char *)malloc(sizeof(char) * 30);
            scanf("%s", args[argNums]);
            c = getchar();
        }
        argNums++;
        // check call history,if not, save history
        if (strcmp("!!", args[0]) == 0 && argNums == 1)
        {
            if (his_now > 0)
            {
                for (int i = 0; i < his_argnum[his_now - 1]; i++)
                {
                    if (i < argNums)
                    {
                        strcpy(args[i], history[his_now - 1][i]);
                    }
                    else
                    {
                        args[i] = (char *)malloc(sizeof(char) * (strlen(history[his_now - 1][i]) + 1));
                        strcpy(args[i], history[his_now - 1][i]);
                    }
                    printf("%s ", args[i]);
                }
                printf("\n");
                fflush(stdout);
                argNums = his_argnum[his_now - 1];
            }
            else
            {
                fprintf(stderr, "No history\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            Save_History(&his_now, history, args, his_argnum, argNums);
        }
        for (int i = 0; i < argNums; i++)
        {
            if (!strcmp(">", args[i]))
            {
                OUTPUT_REDIRECTION = i;
                output_file = (char *)malloc(sizeof(char) * (1 + strlen(args[i + 1])));
                strcpy(output_file, args[i + 1]);
                for (int j = i + 2; j < argNums; j++)
                {
                    strcpy(args[j - 2], args[j]);
                }
                argNums -= 2;
            }
            if (!strcmp("<", args[i]))
            {
                INPUT_REDIRECTION = i;
                input_file = (char *)malloc(sizeof(char) * (1 + strlen(args[i - 1])));
                strcpy(input_file, args[i + 1]);
                for (int j = i + 2; j < argNums; j++)
                {
                    strcpy(args[i - 2], args[i]);
                }
                argNums -= 2;
            }
            if (!strcmp("|", args[i]))
            {
                PIPE_NEED = 1;
                arg1num = i;
                arg2num = argNums - i - 1;
                arg1 = (char **)malloc(sizeof(char *) * (arg1num + 1));
                arg2 = (char **)malloc(sizeof(char *) * (arg2num + 1));
                for (int j = 0; j < arg1num; j++)
                {
                    arg1[j] = (char *)malloc(sizeof(char) * (strlen(args[j]) + 1));
                    strcpy(arg1[j], args[j]);
                }
                arg1[arg1num] = NULL;
                for (int j = 0; j < arg2num; j++)
                {
                    arg2[j] = (char *)malloc(sizeof(char) * (strlen(args[i + 1 + j]) + 1));
                    strcpy(arg2[j], args[i + 1 + j]);
                }
                arg2[arg2num] = NULL;
            }
        }
        if (!strcmp(args[0], "history"))
        {
            if (his_now > 0)
            {
                NEED_CHILD = 0;
                Print_History(his_now, his_argnum, history);
            }
            else
            {
                fprintf(stderr, "No history\n");
            }
        }
        // check if a '&' indicate to run concurrently
        if (strcmp(args[argNums - 1], "&") == 0)
        {
            WAIT_CHILD = 0;
            args[argNums--] = NULL;
            free(args[argNums + 1]);
        }
        // printf("%d\n",argNums);
        // for (int i=0;i<argNums;i++){
        // 	printf("%s\n",args[i]);
        // }
        if (strcmp("exit", args[0]) == 0 && argNums == 1)
        {
            should_run = 0;
            break;
        }

        pid_t pid;
        if (NEED_CHILD)
        {
            pid = fork();
            if (pid < 0)
            {
                fprintf(stderr, "fork failed\n");
                return -1;
            }
            if (pid > 0)
            {
                /* parent process */
                if (WAIT_CHILD)
                {
                    wait(NULL);
                }
                // printf("child finish\n");
            }
            else
            {
                /* child process */
                if (!PIPE_NEED)
                {
                    args[argNums++] = NULL;
                    if (OUTPUT_REDIRECTION > 0)
                    {
                        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0)
                        {
                            perror("open");
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd, STDOUT_FILENO);
                    }
                    if (INPUT_REDIRECTION > 0)
                    {
                        int fd = open(input_file, O_RDONLY);
                        if (fd == -1)
                        {
                            perror("open");
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd, STDIN_FILENO);
                    }

                    if (execvp(args[0], args) == -1)
                    {
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    // parent executes command 2 stored in arg2, and child executes command1
                    /* create pipe */
                    int fd[2];
                    if (pipe(fd) == -1)
                    {
                        fprintf(stderr, "Pipe failed\n");
                        return 1;
                    }

                    /* fork a child process */
                    pid_t ppid = fork();
                    if (ppid < 0)
                    {
                        fprintf(stderr, "fork fail2\n");
                        exit(EXIT_FAILURE);
                    }
                    if (ppid == 0)
                    {
                        /* child process */
                        close(fd[READ_END]);
                        dup2(fd[WRITE_END], STDOUT_FILENO);
                        execvp(arg1[0], arg1);
                    }
                    else
                    {
                        /* parent process */
                        wait(NULL);
                        close(fd[WRITE_END]);
                        dup2(fd[READ_END], STDIN_FILENO);
                        execvp(arg2[0], arg2);
                    }
                }
            }
        }
        wait(NULL);
        fflush(stdout);
    }

    return 0;
}
void Save_History(int *his_now, char ***history, char **args, int *his_argnum, int argNums)
{
    his_argnum[(*his_now)] = argNums;
    history[(*his_now)] = (char **)malloc(sizeof(char *) * (argNums)); // one for null
    for (int i = 0; i < argNums; i++)
    {
        history[(*his_now)][i] = (char *)malloc(sizeof(char) * (strlen(args[i]) + 1));
        strcpy(history[(*his_now)][i], args[i]);
    }
    (*his_now)++;
}
void Print_History(int his_now, int *his_argnum, char ***history)
{
    for (int i = 0; i < his_now; i++)
    {
        printf("%d ", i + 1);
        for (int j = 0; j < his_argnum[i]; j++)
        {
            printf("%s ", history[i][j]);
        }
        printf("\n");
    }
}