// treasure_hub.c - Phase 2 simplified implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#define HUNTS_DIR "hunts"
#define TREASURE_FILE "treasures.dat"
#define MAX_PATH 512

pid_t monitor_pid = -1;

void list_hunts() {
    DIR *d = opendir(HUNTS_DIR);
    struct dirent *e;
    if (!d) return;
    while ((e = readdir(d))) {
        if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
            char file[MAX_PATH];
            snprintf(file, sizeof(file), "%s/%s/%s", HUNTS_DIR, e->d_name, TREASURE_FILE);
            int fd = open(file, O_RDONLY);
            int count = 0;
            if (fd != -1) {
                struct stat st;
                fstat(fd, &st);
                count = st.st_size / sizeof(struct { int id; char username[32]; float lat, lon; char clue[128]; int value; });
                close(fd);
            }
            printf("Hunt: %s, Treasures: %d\n", e->d_name, count);
        }
    }
    closedir(d);
}

void monitor_code() {
    while (1) {
        pause(); // Wait for signals
    }
}

void start_monitor() {
    if ((monitor_pid = fork()) == 0) {
        printf("Monitor started with PID %d\n", getpid());
        signal(SIGUSR1, SIG_IGN); // Placeholder
        signal(SIGUSR2, SIG_IGN); // Placeholder
        signal(SIGTERM, exit);    // Allow exit from kill(SIGTERM)
        monitor_code();
        exit(0);
    }
}

void stop_monitor() {
    if (monitor_pid > 0) {
        kill(monitor_pid, SIGTERM);
        waitpid(monitor_pid, NULL, 0);
        printf("Monitor stopped.\n");
        monitor_pid = -1;
    } else {
        printf("Monitor not running.\n");
    }
}

void list_treasures(const char *hunt) {
    char file[MAX_PATH];
    snprintf(file, sizeof(file), "%s/%s/%s", HUNTS_DIR, hunt, TREASURE_FILE);
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        printf("Cannot open treasures for hunt.\n");
        return;
    }
    struct { int id; char username[32]; float lat, lon; char clue[128]; int value; } t;
    while (read(fd, &t, sizeof(t)) == sizeof(t)) {
        printf("ID: %d, User: %s, Value: %d, Clue: %s\n", t.id, t.username, t.value, t.clue);
    }
    close(fd);
}

void view_treasure(const char *hunt, int id) {
    char file[MAX_PATH];
    snprintf(file, sizeof(file), "%s/%s/%s", HUNTS_DIR, hunt, TREASURE_FILE);
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        printf("Cannot open treasures for hunt.\n");
        return;
    }
    struct { int id; char username[32]; float lat, lon; char clue[128]; int value; } t;
    while (read(fd, &t, sizeof(t)) == sizeof(t)) {
        if (t.id == id) {
            printf("ID: %d\nUser: %s\nLat: %.2f\nLong: %.2f\nValue: %d\nClue: %s\n", t.id, t.username, t.lat, t.lon, t.value, t.clue);
            close(fd);
            return;
        }
    }
    close(fd);
    printf("Treasure not found.\n");
}

int main() {
    char cmd[128], hunt[64];
    int id;
    while (1) {
        printf("hub> ");
        scanf("%s", cmd);

        if (!strcmp(cmd, "start_monitor")) start_monitor();
        else if (!strcmp(cmd, "stop_monitor")) stop_monitor();
        else if (!strcmp(cmd, "list_hunts")) list_hunts();
        else if (!strcmp(cmd, "list_treasures")) {
            scanf("%s", hunt);
            list_treasures(hunt);
        }
        else if (!strcmp(cmd, "view_treasure")) {
            scanf("%s %d", hunt, &id);
            view_treasure(hunt, id);
        }
        else if (!strcmp(cmd, "exit")) {
            stop_monitor();
            break;
        }
        else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}
