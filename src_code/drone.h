// drone.h
#ifndef DRONE_H
#define DRONE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <arpa/inet.h>  // NEW: Network
#include <sys/socket.h> // NEW: Network
#include <netinet/in.h> // NEW: Network
#include "logger.h"

// Constants
#define PORT 8080       // [cite: 368] > 5000
#define MSG_STOP 0
#define MSG_DRONE_POS 1
#define MSG_OBSTACLE 2
#define MSG_TARGET 3
#define MSG_INPUT 4
#define MSG_HIT 5

// Message Structure (Internal IPC)
typedef struct {
    int type;
    int id;
    float x, y;
    float vx, vy;
    float fx, fy;
    char key;
} Message;

typedef struct {
    float M, K, T;
    int W, H;
    int WatchdogT;
} Params;

// Global status for Watchdog
char global_current_status[64];
char global_process_name[32];

// ... (Keep existing get_params and watchdog setup functions) ...
void get_params(Params *p) {
    // Default values
    p->M = 1.0; p->K = 0.5; p->T = 0.1; p->W = 80; p->H = 24; p->WatchdogT = 2;
    FILE *f = fopen("params.txt", "r");
    if (!f) return;
    char key[32];
    while(fscanf(f, "%s", key) != EOF) {
        if (strcmp(key, "M") == 0) fscanf(f, "%f", &p->M);
        else if (strcmp(key, "K") == 0) fscanf(f, "%f", &p->K);
        else if (strcmp(key, "T") == 0) fscanf(f, "%f", &p->T);
        else if (strcmp(key, "Width") == 0) fscanf(f, "%d", &p->W);
        else if (strcmp(key, "Height") == 0) fscanf(f, "%d", &p->H);
        else if (strcmp(key, "WatchdogT") == 0) fscanf(f, "%d", &p->WatchdogT);
    }
    fclose(f);
}

// ... (Keep setup_watchdog logic) ...
void watchdog_handler(int sig) {
    if (sig == SIGUSR1) log_message(LOG_FILE_wd, global_process_name, global_current_status);
}
void setup_watchdog(const char *name) {
    strncpy(global_process_name, name, 31);
    register_pid(name, getpid());
    struct sigaction sa;
    sa.sa_handler = watchdog_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);
}

#endif
