#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CMD 1024

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void handle_start_monitor() {
    pid_t hub_mon_pid = fork();

    if (hub_mon_pid < 0) {
        perror("Eroare la crearea procesului hub_mon\n");
        return;
    }

    if (hub_mon_pid == 0) {
        int pfd[2];
        pid_t monitor_pid;
        FILE *stream;
        char buffer[256];

        if (pipe(pfd) < 0) {
            perror("Eroare la crearea pipe-ului\n");
            exit(1);
        }

        if ((monitor_pid = fork()) < 0) {
            perror("Eroare la fork pentru monitor\n");
            exit(1);
        }

        if (monitor_pid == 0) {
            close(pfd[0]);

            dup2(pfd[1], 1);

            execl("./monitor_reports", "monitor_reports", NULL);
            perror("Eroare la exec monitor_reports\n");
            exit(1);
        }

        close(pfd[1]);

        stream = fdopen(pfd[0], "r");
        if (stream == NULL) {
            perror("Eroare la fdopen\n");
            exit(1);
        }

        while (fgets(buffer, sizeof(buffer), stream) != NULL) {
            printf("[HUB_MON] %s", buffer);
            fflush(stdout);
        }

        printf("[HUB_MON] ATENTIE: Monitorul s-a incheiat!\n");

        fclose(stream);
        exit(0);
    }

    printf("Monitor pornit in background (PID hub_mon: %d).\n", hub_mon_pid);
}

void handle_calculate_scores(char* args) {
    char *district = strtok(args, " \n");

    printf("\n=== WORKLOAD SCORES ===\n");

    while (district != NULL) {
        int pfd[2];
        pid_t scorer_pid;
        FILE *stream;
        char buffer[256];

        if (pipe(pfd) < 0) {
            perror("Eroare la crearea pipe-ului\n");
            break;
        }

        if ((scorer_pid = fork()) < 0) {
            perror("Eroare la fork pentru scorer\n");
            break;
        }

        if (scorer_pid == 0) {
            close(pfd[0]);

            dup2(pfd[1], 1);

            execl("./scorer", "scorer", district, NULL);
            perror("Eroare la exec scorer\n");
            exit(1);
        }

        close(pfd[1]);

        printf("--- District: %s ---\n", district);

        stream = fdopen(pfd[0], "r");
        while (fgets(buffer, sizeof(buffer), stream) != NULL) {
            printf("%s", buffer);
        }
        fclose(stream);

        waitpid(scorer_pid, NULL, 0);

        district = strtok(NULL, " \n");
    }
    printf("=======================\n\n");
}

int main() {
    char command[MAX_CMD];

    signal(SIGCHLD, sigchld_handler);

    printf("Welcome to City Hub CLI. Available commands:\n");
    printf(" - start_monitor\n");
    printf(" - calculate_scores <district1> <district2> ...\n");
    printf(" - exit\n");

    while (1) {
        printf("city_hub> ");
        if (fgets(command, MAX_CMD, stdin) == NULL) {
            break;
        }

        command[strcspn(command, "\n")] = 0;

        if (strlen(command) == 0) continue;

        if (strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "start_monitor") == 0) {
            handle_start_monitor();
        } else if (strncmp(command, "calculate_scores", 16) == 0) {
            char *args = command + 16;
            handle_calculate_scores(args);
        } else {
            printf("Comanda necunoscuta.\n");
        }
    }
    
    return 0;
}