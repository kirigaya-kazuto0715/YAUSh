#include "def.h"

typedef struct {
    pid_t pid;
    char cmd[MAX_CMD_LEN];
    int is_running;
} job_t;

#define MAX_JOBS 64
job_t jobs[MAX_JOBS];
int job_count = 0;

// 添加作业到作业列表
void add_job(pid_t pid, const char *cmd) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].cmd, cmd, MAX_CMD_LEN);
        jobs[job_count].is_running = 1;
        job_count++;
    }
}

// 删除已完成作业
void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

// 内置命令：jobs
void builtin_jobs() {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %s (%s)\n", i + 1, jobs[i].cmd, jobs[i].is_running ? "Running" : "Stopped");
    }
}

// 内置命令：bg
void builtin_bg(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "bg: 缺少作业号\n");
        return;
    }
    int job_id = atoi(args[1]) - 1;
    if (job_id < 0 || job_id >= job_count) {
        fprintf(stderr, "bg: 无效的作业号\n");
        return;
    }
    kill(jobs[job_id].pid, SIGCONT);
    jobs[job_id].is_running = 1;
}

// 内置命令：fg
void builtin_fg(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "fg: 缺少作业号\n");
        return;
    }
    int job_id = atoi(args[1]) - 1;
    if (job_id < 0 || job_id >= job_count) {
        fprintf(stderr, "fg: 无效的作业号\n");
        return;
    }
    int status;
    tcsetpgrp(shell_terminal, jobs[job_id].pid);
    waitpid(jobs[job_id].pid, &status, WUNTRACED);
    tcsetpgrp(shell_terminal, shell_pgid);
    if (WIFSTOPPED(status)) {
        jobs[job_id].is_running = 0;
    } else {
        remove_job(jobs[job_id].pid);
    }
}
// cd 命令
void builtin_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: 缺少参数\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    }
}

// pwd 命令
void builtin_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

// history 命令
void builtin_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

// exit 命令
void builtin_exit() {
    exit(0);
}

// 判断并执行内置命令
void execute_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        builtin_cd(args);
    } else if (strcmp(args[0], "pwd") == 0) {
        builtin_pwd();
    } else if (strcmp(args[0], "history") == 0) {
        builtin_history();
    } else if (strcmp(args[0], "exit") == 0) {
        builtin_exit();
    } else if (strcmp(args[0], "jobs") == 0) {
        builtin_jobs();
    } else if (strcmp(args[0], "bg") == 0) {
        builtin_bg(args);
    } else if (strcmp(args[0], "fg") == 0) {
        builtin_fg(args);
    } else {
        fprintf(stderr, "未知命令: %s\n", args[0]);
    }
}
