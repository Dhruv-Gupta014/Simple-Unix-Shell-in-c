#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_HISTORY 100

// history struct
struct hist_entry {
    char cmd[MAX_INPUT];
    pid_t pid;
    struct timeval start, end;
};
struct hist_entry history[MAX_HISTORY];
int hist_count = 0;

// SIGINT handler
void sigint_handler(int sig) {
    printf("\n- Session History -\n");
    for (int i = 0; i < hist_count; i++) {
        long dur = (history[i].end.tv_sec - history[i].start.tv_sec) * 1000 +
                   (history[i].end.tv_usec - history[i].start.tv_usec) / 1000;
        printf("%d) cmd=%s pid=%d start=%ld duration=%ldms\n", i+1, history[i].cmd, history[i].pid, history[i].start.tv_sec, dur);
    }
    exit(0);
}
// read input
char* read_user_input() {
    char *line = malloc(MAX_INPUT);
    if (!line) { perror("malloc"); exit(1); }
    if (fgets(line, MAX_INPUT, stdin) == NULL) {
        free(line); return NULL;
    }
    line[strcspn(line, "\n")] = '\0';
    return line;
}
// split whitespaces
char** split_command(char *line) {
    char **args = malloc(MAX_ARGS * sizeof(char*));
    int pos = 0;
    char *word = strtok(line, " \t\r\n");
    while (word && pos < MAX_ARGS-1) {
        args[pos++] = word;
        word = strtok(NULL, " \t\r\n");
    }
    args[pos] = NULL;
    return args;
}
// split pipe
int split_pipes(char *line, char **parts) {
    int count = 0;
    char *word = strtok(line, "|");
    while (word && count < MAX_ARGS) {
        parts[count++] = word;
        word = strtok(NULL, "|");
    }
    return count;
}
// craete processes and run commands
int create_process_and_run(char *line) {
    char *cmds[MAX_ARGS];
    int n = split_pipes(line, cmds);
    int in_fd = 0;
    int fd[2];
    for (int i = 0; i < n; i++) {
        char *part = cmds[i];
        while (*part == ' ') part++; 
        char **args = split_command(part);
        if (args[0] == NULL) { free(args); continue; }

        // built-in history
        if (strcmp(args[0], "history") == 0) {
            for (int j = 0; j < hist_count; j++) {
                printf("%d %s\n", j+1, history[j].cmd);
            }
            free(args);
            return 1;
        }
        if (strcmp(args[0], "exit") == 0) {
            free(args);
            return 0;
        }
        if (i < n-1) pipe(fd);
        pid_t pid = fork(); // took refrence from tut 5  about pipe with exec
        if (pid == 0) {
            if (i > 0) {
                dup2(in_fd, 0);
                close(in_fd);
            }
            if (i < n-1) {
                dup2(fd[1], 1);
                close(fd[0]); close(fd[1]);
            }
            execvp(args[0], args);
            printf("command not found: %s\n", args[0]);
            exit(1);
        } else if (pid > 0) {
            struct timeval st, en;
            gettimeofday(&st, NULL);
            waitpid(pid, NULL, 0);
            gettimeofday(&en, NULL);
            if (hist_count < MAX_HISTORY) {
                strcpy(history[hist_count].cmd, args[0]);
                history[hist_count].pid = pid;
                history[hist_count].start = st;
                history[hist_count].end = en;
                hist_count++;
            }
            if (i < n-1) {
                close(fd[1]);
                in_fd = fd[0];
            }
        } else {
            perror("fork");
        }
        free(args);
    }
    return 1;
}
int launch(char *line) { // took refrence from lecture 7
    return create_process_and_run(line);
}
// shell loop 
void shell_loop() {  // took refrence from lecture 7
    int status;
    char *line;//
    do {
        printf("iiitd@possum:~$ ");
        line = read_user_input();
        if (!line) break;
        status = launch(line);
        free(line);
    } while (status);
}
int main() {
    signal(SIGINT, sigint_handler);
    shell_loop();
    return 0;
}