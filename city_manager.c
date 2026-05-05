#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

void handle_sigchld(int sig) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        const char *msg = "\n[Parinte] Semnal SIGCHLD interceptat: Copilul si-a terminat executia in fundal.\n";
        write(STDOUT_FILENO, msg, strlen(msg));
    }
    errno = saved_errno;
}

typedef struct {
    int id;
    char inspector[32];
    float lat;
    float lon;
    char category[32];
    int severity;
    time_t timestamp;
    char description[96];
} Report;

void mode_to_string(mode_t mode, char *str) {
    strcpy(str, "---------");
    if (mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';
}

int parse_condition(const char *input, char *field, char *op, char *val) {
    return sscanf(input, "%[^:]:%[^:]:%s", field, op, val) == 3;
}

int match_condition(Report *r, const char *field, const char *op, const char *val) {
    long r_val = 0, c_val = atol(val);
    if (strcmp(field, "severity") == 0) r_val = r->severity;
    else if (strcmp(field, "timestamp") == 0) r_val = (long)r->timestamp;
    else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, val) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, val) != 0;
        return 0;
    } else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector, val) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector, val) != 0;
        return 0;
    } else return 0;

    if (strcmp(op, "==") == 0) return r_val == c_val;
    if (strcmp(op, "!=") == 0) return r_val != c_val;
    if (strcmp(op, ">") == 0)  return r_val > c_val;
    if (strcmp(op, ">=") == 0) return r_val >= c_val;
    if (strcmp(op, "<") == 0)  return r_val < c_val;
    if (strcmp(op, "<=") == 0) return r_val <= c_val;
    return 0;
}

int check_access(const char *filepath, const char *role, int require_write) {
    struct stat st;
    if (stat(filepath, &st) < 0) return 0;

    if (strcmp(role, "manager") == 0) {
        if (require_write) return (st.st_mode & S_IWUSR) ? 1 : 0;
        return (st.st_mode & S_IRUSR) ? 1 : 0;
    } else if (strcmp(role, "inspector") == 0) {
        if (require_write) return (st.st_mode & S_IWGRP) ? 1 : 0;
        return (st.st_mode & S_IRGRP) ? 1 : 0;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    struct sigaction sa_chld;
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa_chld, NULL);

    char monitor_status[128] = "";
    char *role = NULL, *user = NULL, *action = NULL, *district = NULL;
    char *extra_arg = NULL;
    int filter_start_idx = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) role = argv[++i];
        else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) user = argv[++i];
        else if (strncmp(argv[i], "--", 2) == 0 && i + 1 < argc) {
            action = argv[i] + 2;
            district = argv[++i];
            if (strcmp(action, "filter") == 0) {
                filter_start_idx = i + 1;
                break;
            }
            if (i + 1 < argc && strncmp(argv[i+1], "--", 2) != 0) extra_arg = argv[++i];
        }
    }

    if (!role || !action || !district) return 1;

    if (strcmp(action, "remove_district") == 0) {
        if (strcmp(role, "manager") != 0) { printf("Deny\n"); return 1; }

        char sym_path[256];
        snprintf(sym_path, sizeof(sym_path), "active_reports-%s", district);
        unlink(sym_path);

        pid_t pid;
        if ((pid = fork()) < 0) {
            perror("Eroare la fork");
            exit(1);
        }

        if (pid == 0) {
            execlp("rm", "rm", "-rf", district, NULL);
            perror("Eroare la execlp");
            exit(1);
        }

        printf("[Parinte] Am pornit stergerea districtului in paralel (PID copil: %d)\n", pid);
        for(int i = 0; i < 2; i++) {
            printf("[Parinte] Imi continui treaba... (secunda %d)\n", i + 1);
            sleep(1);
        }
        return 0;
    }

    struct stat st;
    if (stat(district, &st) < 0) {
        if (mkdir(district, 0750) < 0) { perror("mkdir"); return 1; }
    }

    char r_path[256], c_path[256], l_path[256], sym_path[256];
    snprintf(r_path, sizeof(r_path), "%s/reports.dat", district);
    snprintf(c_path, sizeof(c_path), "%s/district.cfg", district);
    snprintf(l_path, sizeof(l_path), "%s/logged_district", district);
    snprintf(sym_path, sizeof(sym_path), "active_reports-%s", district);

    int fd_r = open(r_path, O_CREAT | O_RDWR | O_APPEND, 0664);
    if (fd_r >= 0) { chmod(r_path, 0664); close(fd_r); }
    int fd_c = open(c_path, O_CREAT | O_RDWR, 0640);
    if (fd_c >= 0) { chmod(c_path, 0640); close(fd_c); }
    int fd_l = open(l_path, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (fd_l >= 0) { chmod(l_path, 0644); close(fd_l); }

    unlink(sym_path);
    if (symlink(r_path, sym_path) < 0) perror("symlink");

    if (strcmp(action, "add") == 0) {
        if (!check_access(r_path, role, 1)) { printf("Perm error\n"); return 1; }
        Report r; memset(&r, 0, sizeof(Report));
        if (stat(r_path, &st) == 0) r.id = st.st_size / sizeof(Report);
        else r.id = 0;

        strncpy(r.inspector, user ? user : "unknown", 31);
        r.timestamp = time(NULL);

        printf("X: "); scanf("%f", &r.lat);
        printf("Y: "); scanf("%f", &r.lon);
        printf("Category: "); scanf("%31s", r.category);
        printf("Severity (1-3): "); scanf("%d", &r.severity);
        getchar();
        printf("Desc: "); fgets(r.description, 96, stdin);
        r.description[strcspn(r.description, "\n")] = 0;

        int fd = open(r_path, O_WRONLY | O_APPEND);
        if (fd >= 0) { write(fd, &r, sizeof(Report)); close(fd); }

        strcpy(monitor_status, "Monitor could not be informed (PID file missing).\n");
        int pid_fd = open(".monitor_pid", O_RDONLY);
        if (pid_fd >= 0) {
            char pid_buf[32] = {0};
            int bytes_read = read(pid_fd, pid_buf, sizeof(pid_buf) - 1);
            close(pid_fd);

            if (bytes_read > 0) {
                pid_t target_pid = atoi(pid_buf);
                if (kill(target_pid, SIGUSR1) == 0) {
                    strcpy(monitor_status, "Monitor successfully notified via SIGUSR1.\n");
                } else {
                    strcpy(monitor_status, "Monitor could not be informed (Signal failed).\n");
                }
            }
        }
    }
    else if (strcmp(action, "list") == 0) {
        if (!check_access(r_path, role, 0)) { printf("Perm error\n"); return 1; }
        if (lstat(r_path, &st) == 0) {
            char perm[10]; mode_to_string(st.st_mode, perm);
            printf("File: %s, Size: %ld, Mod: %ld, Perm: %s\n", r_path, st.st_size, st.st_mtime, perm);
        }
        int fd = open(r_path, O_RDONLY);
        if (fd >= 0) {
            Report r;
            while (read(fd, &r, sizeof(Report)) > 0)
                printf("ID: %d | Cat: %s | Sev: %d | Insp: %s\n", r.id, r.category, r.severity, r.inspector);
            close(fd);
        }
    }
    else if (strcmp(action, "view") == 0 && extra_arg) {
        if (!check_access(r_path, role, 0)) { printf("Perm error\n"); return 1; }
        int target_id = atoi(extra_arg);
        int fd = open(r_path, O_RDONLY);
        if (fd >= 0) {
            Report r;
            while (read(fd, &r, sizeof(Report)) > 0) {
                if (r.id == target_id) {
                    printf("ID: %d\nInsp: %s\nGPS: %.2f, %.2f\nCat: %s\nSev: %d\nTime: %ld\nDesc: %s\n",
                           r.id, r.inspector, r.lat, r.lon, r.category, r.severity, (long)r.timestamp, r.description);
                    break;
                }
            }
            close(fd);
        }
    }
    else if (strcmp(action, "remove_report") == 0 && extra_arg) {
        if (!check_access(r_path, role, 1)) { printf("Perm error\n"); return 1; }
        if (strcmp(role, "manager") != 0) { printf("Deny\n"); return 1; }
        int target_id = atoi(extra_arg);
        int fd = open(r_path, O_RDWR);
        if (fd >= 0) {
            Report r; int i = 0, found = -1;
            while (read(fd, &r, sizeof(Report)) > 0) {
                if (r.id == target_id) { found = i; break; }
                i++;
            }
            if (found != -1) {
                stat(r_path, &st); int count = st.st_size / sizeof(Report);
                for (int j = found + 1; j < count; j++) {
                    lseek(fd, j * sizeof(Report), SEEK_SET);
                    read(fd, &r, sizeof(Report));
                    lseek(fd, (j - 1) * sizeof(Report), SEEK_SET);
                    write(fd, &r, sizeof(Report));
                }
                ftruncate(fd, (count - 1) * sizeof(Report));
            }
            close(fd);
        }
    }
    else if (strcmp(action, "update_threshold") == 0 && extra_arg) {
        if (!check_access(c_path, role, 1)) { printf("Perm error cfg\n"); return 1; }
        if (strcmp(role, "manager") != 0) { printf("Deny\n"); return 1; }
        int fd = open(c_path, O_WRONLY | O_TRUNC);
        if (fd >= 0) {
            write(fd, extra_arg, strlen(extra_arg));
            write(fd, "\n", 1);
            close(fd);
        }
    }
    else if (strcmp(action, "filter") == 0 && filter_start_idx != -1) {
        if (!check_access(r_path, role, 0)) { printf("Perm error\n"); return 1; }
        int fd = open(r_path, O_RDONLY);
        if (fd >= 0) {
            Report r;
            while (read(fd, &r, sizeof(Report)) > 0) {
                int match_all = 1;
                for (int i = filter_start_idx; i < argc; i++) {
                    char f[32], o[3], v[32];
                    if (parse_condition(argv[i], f, o, v)) {
                        if (!match_condition(&r, f, o, v)) {
                            match_all = 0; break;
                        }
                    }
                }
                if (match_all) printf("ID: %d | Cat: %s | Sev: %d | Insp: %s\n", r.id, r.category, r.severity, r.inspector);
            }
            close(fd);
        }
    }

    int fd_log = open(l_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd_log >= 0) {
        if (!check_access(l_path, role, 1) && strcmp(role, "manager") != 0) {
            close(fd_log);
        } else {
            char log_entry[512];
            if (strcmp(action, "add") == 0) {
                snprintf(log_entry, sizeof(log_entry), "%ld\n%s\n%s %s\n%s",
                         (long)time(NULL), user ? user : "unknown", role, action, monitor_status);
            } else {
                snprintf(log_entry, sizeof(log_entry), "%ld\n%s\n%s %s\n",
                         (long)time(NULL), user ? user : "unknown", role, action);
            }
            write(fd_log, log_entry, strlen(log_entry));
            close(fd_log);
        }
    }

    return 0;
}