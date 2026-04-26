//link-ul GitHub https://github.com/Iudzjin/SO_Proiect

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

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



int main(int argc, char *argv[]) {
    char *role = NULL, *user = NULL, *action = NULL, *district = NULL;



    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) role = argv[++i];
        else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) user = argv[++i];
        else if (strncmp(argv[i], "--", 2) == 0 && i + 1 < argc) {
            action = argv[i] + 2;
            district = argv[++i];
            if (strcmp(action, "filter") == 0) {

                break;
            }
        }
    }

    if (!role || !action || !district) return 1;

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
    }
    int fd_log = open(l_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd_log >= 0) {
            char log_entry[256];
            snprintf(log_entry, sizeof(log_entry), "%ld\n%s\n%s %s\n", (long)time(NULL), user ? user : "unknown", role, action);
            write(fd_log, log_entry, strlen(log_entry));
            close(fd_log);
        }

    return 0;
}