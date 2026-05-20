#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    keep_running = 0;
    const char *msg = "\nINFO: Semnal SIGINT primit. Se opreste executia...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

void handle_sigusr1(int sig) {
    const char *msg = "EVENT: A fost adaugat un raport nou in sistem!\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main() {
    int fd = open(".monitor_pid", O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd < 0) {
        FILE *f = fopen(".monitor_pid", "r");
        int existing_pid = -1;
        if (f != NULL) {
            fscanf(f, "%d", &existing_pid);
            fclose(f);
        }
        printf("ERROR: Un alt monitor ruleaza deja cu PID-ul: %d\n", existing_pid);
        fflush(stdout);
        return 1;
    }

    pid_t my_pid = getpid();
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d\n", my_pid);
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

    printf("SUCCESS: Monitor initializat (PID %d). Astept semnale...\n", my_pid);
    fflush(stdout);

    while (keep_running) {
        pause();
    }

    printf("INFO: Monitorul se opreste normal.\n");
    fflush(stdout);
    unlink(".monitor_pid");

    return 0;
}