#include "def.h"
extern int shell_terminal;
extern pid_t shell_pgid;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// **解析单个命令的参数**，将参数存储到 args 数组中
void parse_command(char *cmd, char **args) {
    int i = 0;
    char *token = strtok(cmd, " \t\n"); // 以空格、制表符、换行符分割命令
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL; // 最后设置为 NULL 表示参数结束
}

// **管道执行函数**
void execute_pipeline(char *cmd) {
    char *commands[MAX_PIPE_NUM];  // 存储管道中的子命令
    int cmd_count = 0;             // 命令数量

    // **解析管道符 '|'，分割命令**
    char *token = strtok(cmd, "|");
    while (token != NULL) {
        commands[cmd_count++] = token;
        token = strtok(NULL, "|");
    }
    
    int prev_pipe_fd = -1; // 保存前一个管道的读端

    // **遍历每个子命令**
    for (int i = 0; i < cmd_count; i++) {
        char *args[MAX_ARG_NUM];  // 存储当前命令的参数
        parse_command(commands[i], args); // 解析当前命令参数

        int pipe_fd[2]; // 创建管道
        if (i < cmd_count - 1) { // 如果不是最后一个命令，创建管道
            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork(); // 创建子进程
        if (pid == 0) { // 子进程
            if (prev_pipe_fd != -1) { // 如果有前一个管道，则重定向输入
                dup2(prev_pipe_fd, STDIN_FILENO);
                close(prev_pipe_fd);
            }

            if (i < cmd_count - 1) { // 如果不是最后一个命令，则重定向输出到管道
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[0]);
                close(pipe_fd[1]);
            }

            // 执行命令
            execvp(args[0], args);
            perror("execvp"); // 执行失败
            exit(EXIT_FAILURE);
        } else if (pid > 0) { // 父进程
            waitpid(pid, NULL, 0); // 等待子进程完成

            if (prev_pipe_fd != -1) // 关闭前一个管道的读端
                close(prev_pipe_fd);

            if (i < cmd_count - 1) { // 更新 prev_pipe_fd 指向当前管道的读端
                prev_pipe_fd = pipe_fd[0];
                close(pipe_fd[1]); // 关闭写端
            }
        } else { // fork 失败
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }
}

// 处理输入输出重定向
void handle_redirection(char **args, int *redirect_in, int *redirect_out, char **in_file, char **out_file, int *append_mode) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {  // 输入重定向
            *redirect_in = 1;
            *in_file = args[i + 1];
            args[i] = NULL;  // 断开参数
        } else if (strcmp(args[i], ">") == 0) {  // 输出重定向
            *redirect_out = 1;
            *out_file = args[i + 1];
            *append_mode = 0;
            args[i] = NULL;
        } else if (strcmp(args[i], ">>") == 0) {  // 输出追加重定向
            *redirect_out = 1;
            *out_file = args[i + 1];
            *append_mode = 1;
            args[i] = NULL;
        }
    }
}

// 执行命令并处理后台运行、重定向等
void parse_and_execute(char *cmd) {
    char *args[MAX_ARG_NUM];
    int i = 0;
    // 判断是否是管道命令
    if (strchr(cmd, '|')) {
         execute_pipeline(cmd);  // 处理管道命令
         return;
    }
    // 将命令按空格分割成多个参数
    args[i] = strtok(cmd, " ");
    while (args[i] != NULL) args[++i] = strtok(NULL, " ");

    if (args[0] == NULL) return;  // 空命令，直接返回
    
    // 处理重定向
    int redirect_in = 0, redirect_out = 0, append_mode = 0;
    char *in_file = NULL, *out_file = NULL;

    handle_redirection(args, &redirect_in, &redirect_out, &in_file, &out_file, &append_mode);

    // 判断是否是内置命令
    if (strcmp(args[0], "cd") == 0) {
        execute_builtin(args);  // 执行内置命令 cd
    } else if (strcmp(args[0], "pwd") == 0) {
        execute_builtin(args);  // 执行内置命令 pwd
    } else if (strcmp(args[0], "history") == 0) {
        execute_builtin(args);  // 执行内置命令 history
    } else if (strcmp(args[0], "exit") == 0) {
        execute_builtin(args);  // 执行内置命令 exit
    } else if (strcmp(args[0], "jobs") == 0) {
        execute_builtin(args);  // 执行内置命令 jobs
    } else if (strcmp(args[0], "bg") == 0) {
        execute_builtin(args);  // 执行内置命令 bg
    } else if (strcmp(args[0], "fg") == 0) {
        execute_builtin(args);  // 执行内置命令 fg
    } else {  // 不是内置命令，执行外部命令
        // 检查后台执行符号
        int is_bg = 0;
        if (strcmp(args[i - 1], "&") == 0) {
            is_bg = 1;
            args[i - 1] = NULL;  // 移除 &
        }

        pid_t pid = fork();
        if (pid == 0) {  // 子进程
            // 处理输入重定向
            if (redirect_in) {
                int fd_in = open(in_file, O_RDONLY);
                if (fd_in == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd_in, 0);
                close(fd_in);
            }

            // 处理输出重定向
            if (redirect_out) {
                int fd_out;
                if (append_mode) {
                    fd_out = open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                } else {
                    fd_out = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                }
                if (fd_out == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd_out, 1);
                close(fd_out);
            }

            if (is_bg) {
                setpgid(0, 0);  // 后台命令设置进程组
            }

            execvp(args[0], args);  // 执行外部命令
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {  // 父进程
            if (is_bg) {
                add_job(pid, cmd);  // 将后台任务添加到作业控制
            } else {
                int status;
                waitpid(pid, &status, 0);  // 等待前台任务结束
            }
        } else {
            perror("fork");
        }
    }
}
