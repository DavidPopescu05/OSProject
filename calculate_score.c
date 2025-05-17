#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#define HUNTS_DIR "hunts"
#define TREASURE_FILE "treasures.dat"

typedef struct {
    int id;
    char username[32];
    float lat, lon;
    int value;
    char clue[128];
} Treasure;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <username>\n", argv[0]);
        return 1;
    }

    const char *target_user = argv[1];
    DIR *dir = opendir(HUNTS_DIR);
    if (!dir) {
        perror("Could not open hunts directory");
        return 1;
    }

    struct dirent *entry;
    int total = 0;

    while ((entry = readdir(dir))) {
        if (entry->d_type != DT_DIR || entry->d_name[0] == '.') continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s/%s", HUNTS_DIR, entry->d_name, TREASURE_FILE);

        int fd = open(path, O_RDONLY);
        if (fd < 0) continue;

        Treasure t;
        while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
            if (strcmp(t.username, target_user) == 0) {
                total += t.value;
            }
        }

        close(fd);
    }

    closedir(dir);
    printf("Total score for %s: %d points\n", target_user, total);
    return 0;
}

