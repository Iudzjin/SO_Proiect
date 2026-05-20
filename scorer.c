#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

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

typedef struct {
    char name[32];
    int total_severity;
    int report_count;
} InspectorScore;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Eroare: Lipseste argumentul district.\n");
        return 1;
    }

    char *district = argv[1];
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("  Nu s-au gasit rapoarte sau districtul '%s' nu exista.\n", district);
        return 0;
    }

    InspectorScore scores[100];
    int inspector_count = 0;
    memset(scores, 0, sizeof(scores));

    Report r;
    while (read(fd, &r, sizeof(Report)) > 0) {
        int found = -1;
        for (int i = 0; i < inspector_count; i++) {
            if (strcmp(scores[i].name, r.inspector) == 0) {
                found = i;
                break;
            }
        }

        if (found != -1) {
            scores[found].total_severity += r.severity;
            scores[found].report_count++;
        } else if (inspector_count < 100) {
            strncpy(scores[inspector_count].name, r.inspector, 31);
            scores[inspector_count].total_severity = r.severity;
            scores[inspector_count].report_count = 1;
            inspector_count++;
        }
    }

    close(fd);

    if (inspector_count == 0) {
        printf("  Nu exista rapoarte inregistrate in acest district.\n");
    } else {
        for (int i = 0; i < inspector_count; i++) {
            printf("  Inspector %s: %d (%d rapoarte)\n",
                   scores[i].name, scores[i].total_severity, scores[i].report_count);
        }
    }

    return 0;
}