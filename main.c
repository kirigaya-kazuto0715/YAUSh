#include "def.h"

char *history[MAX_HISTORY];  
int history_count = 0;
int shell_terminal;
int shell_is_interactive;
pid_t shell_pgid;

void init_shell() {
    // 设置Shell为交互模式
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        shell_pgid = getpid();
        setpgid(shell_pgid, shell_pgid);
        tcsetpgrp(shell_terminal, shell_pgid);

        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
    }
}

void add_to_history(char *cmd) {
    if (history_count < MAX_HISTORY) {
        history[history_count++] = strdup(cmd);
    } else {
        free(history[0]);
        memmove(&history[0], &history[1], (MAX_HISTORY - 1) * sizeof(char *));
        history[MAX_HISTORY - 1] = strdup(cmd);
    }
}

int main() {
    char cmd[MAX_CMD_LEN];

    init_shell();

    while (1) {
        printf("shell> ");
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
            break;
        }

        cmd[strcspn(cmd, "\n")] = '\0';  // 去除换行符
        if (strlen(cmd) == 0) continue;

        add_to_history(cmd);

        // 检查并执行命令
        parse_and_execute(cmd);
    }

    return 0;
}
