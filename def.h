#ifndef DEF_H
#define DEF_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_CMD_LEN 1024
#define MAX_ARG_NUM 128
#define MAX_HISTORY 100
#define MAX_PIPE_NUM 5

// 全局变量
extern char *history[MAX_HISTORY];  // 历史命令
extern int history_count;           // 历史命令计数
extern pid_t shell_pgid;            // Shell的进程组ID
extern int shell_terminal;          // 终端文件描述符
extern int shell_is_interactive;    // 是否为交互式

// 函数声明
void execute_builtin(char **args);   // 执行内置命令
void parse_and_execute(char *cmd);   // 解析并执行外部命令
void add_job(pid_t pid, const char *cmd); //添加作业

#endif
