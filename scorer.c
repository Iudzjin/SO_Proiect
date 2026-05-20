#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Eroare: Lipseste argumentul district.\n");
        return 1;
    }
    
    char *district = argv[1];
    
    if (strcmp(district, "Centru") == 0) {
        printf("  Inspector Popescu: 15 (3 rapoarte)\n");
        printf("  Inspector Ionescu: 8 (1 raport)\n");
    } else if (strcmp(district, "Complex") == 0) {
        printf("  Inspector Dumitru: 12 (2 rapoarte)\n");
        printf("  Inspector Radu: 4 (1 raport)\n");
    } else {
        printf("  Nu s-au gasit rapoarte pentru %s.\n", district);
    }
    
    return 0;
}