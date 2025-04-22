// treasure_manager.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"
#define HUNTS_DIR "hunts"
#define MAX_PATH 512

// Fixed-size Treasure structure
typedef struct {
    int id;
    char username[32];
    float latitude, longitude;
    char clue[128];
    int value;
} Treasure;

// Utility: Create hunt directory if it doesn't exist
int ensure_hunt_dir(const char *hunt_id) {
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s/%s", HUNTS_DIR, hunt_id);
    if (mkdir(HUNTS_DIR, 0755) == -1 && errno != EEXIST) return -1;
    if (mkdir(path, 0755) == -1 && errno != EEXIST) return -1;
    return 0;
}

// Utility: Log an action
void log_action(const char *hunt_id, const char *action) {
    char log_path[MAX_PATH];
    snprintf(log_path, MAX_PATH, "%s/%s/%s", HUNTS_DIR, hunt_id, LOG_FILE);

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) return;

    dprintf(fd, "%s\n", action);
    close(fd);

    // Create symbolic link
    char symlink_name[MAX_PATH];
    snprintf(symlink_name, MAX_PATH, "logged_hunt-%s", hunt_id);
    unlink(symlink_name); // ensure old link is removed
    symlink(log_path, symlink_name);
}

// ADD command
treasure_add(const char *hunt_id) {
    if (ensure_hunt_dir(hunt_id) != 0) {
        perror("mkdir");
        return;
    }

    Treasure t;
    printf("Enter Treasure ID: "); scanf("%d", &t.id);
    printf("Enter Username: "); scanf("%s", t.username);
    printf("Enter Latitude: "); scanf("%f", &t.latitude);
    printf("Enter Longitude: "); scanf("%f", &t.longitude);
    printf("Enter Clue: "); getchar(); fgets(t.clue, sizeof(t.clue), stdin);
    t.clue[strcspn(t.clue, "\n")] = 0; // Remove newline
    printf("Enter Value: "); scanf("%d", &t.value);

    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s/%s", HUNTS_DIR, hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }

    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_action(hunt_id, "add");
    printf("Treasure added successfully.\n");
}

// LIST command
void treasure_list(const char *hunt_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s/%s", HUNTS_DIR, hunt_id, TREASURE_FILE);

    struct stat st;
    if (stat(file_path, &st) == -1) {
        perror("stat");
        return;
    }

    printf("Hunt ID: %s\n", hunt_id);
    printf("File Size: %ld bytes\n", st.st_size);
    printf("Last Modified: %s", ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    Treasure t;
    printf("\nTreasure List:\n");
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d, User: %s, Lat: %.2f, Long: %.2f, Value: %d, Clue: %s\n",
               t.id, t.username, t.latitude, t.longitude, t.value, t.clue);
    }

    close(fd);
    log_action(hunt_id, "list");
}

// VIEW command
void treasure_view(const char *hunt_id, int target_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s/%s", HUNTS_DIR, hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == target_id) {
            printf("ID: %d\nUser: %s\nLat: %.2f\nLong: %.2f\nValue: %d\nClue: %s\n",
                   t.id, t.username, t.latitude, t.longitude, t.value, t.clue);
            close(fd);
            log_action(hunt_id, "view");
            return;
        }
    }

    printf("Treasure with ID %d not found.\n", target_id);
    close(fd);
}

// REMOVE_TREASURE command
void treasure_remove(const char *hunt_id, int target_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s/%s", HUNTS_DIR, hunt_id, TREASURE_FILE);

    char tmp_path[MAX_PATH];
    snprintf(tmp_path, MAX_PATH, "%s.tmp", file_path);

    int fd = open(file_path, O_RDONLY);
    int tmp_fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1 || tmp_fd == -1) {
        perror("open");
        return;
    }

    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == target_id) {
            found = 1;
            continue;
        }
        write(tmp_fd, &t, sizeof(Treasure));
    }

    close(fd);
    close(tmp_fd);
    rename(tmp_path, file_path);

    if (found) printf("Treasure removed.\n");
    else printf("Treasure not found.\n");

    log_action(hunt_id, "remove_treasure");
}

// REMOVE_HUNT command
void hunt_remove(const char *hunt_id) {
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s/%s", HUNTS_DIR, hunt_id);

    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    char file_path[MAX_PATH];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        snprintf(file_path, MAX_PATH, "%s/%s", path, entry->d_name);
        unlink(file_path);
    }

    closedir(dir);
    rmdir(path);
    char symlink_name[MAX_PATH];
    snprintf(symlink_name, MAX_PATH, "logged_hunt-%s", hunt_id);
    unlink(symlink_name);

    log_action(hunt_id, "remove_hunt");
    printf("Hunt %s removed.\n", hunt_id);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <command> <hunt_id> [<id>]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "add") == 0) {
        treasure_add(argv[2]);
    } else if (strcmp(argv[1], "list") == 0) {
        treasure_list(argv[2]);
    } else if (strcmp(argv[1], "view") == 0 && argc == 4) {
        treasure_view(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "remove_treasure") == 0 && argc == 4) {
        treasure_remove(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "remove_hunt") == 0) {
        hunt_remove(argv[2]);
    } else {
        fprintf(stderr, "Invalid command or arguments.\n");
    }

    return 0;
}
