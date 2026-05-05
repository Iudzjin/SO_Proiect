#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    keep_running = 0;
    const char *msg = "\n[Monitor] Received SIGINT. Shutting down...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

void handle_sigusr1(int sig) {
    const char *msg = "[Monitor] Received SIGUSR1: A new report has been added!\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main() {
    pid_t pid = getpid();

    int fd = open(".monitor_pid", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Error creating .monitor_pid");
        return 1;
    }

    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d\n", pid);
    write(fd, pid_str, strlen(pid_str));
    close(fd);

    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_usr1;
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    printf("Monitor started with PID %d. Waiting for signals...\n", pid);

    while (keep_running) {
        pause();
    }

    unlink(".monitor_pid");
    printf("[Monitor] .monitor_pid deleted. Exiting cleanly.\n");

    return 0;
}