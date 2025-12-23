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
#include "logger.h"

// Message Codes
#define MSG_STOP      0
#define MSG_DRONE_POS 1
#define MSG_OBSTACLE  2
#define MSG_TARGET    3
#define MSG_INPUT     4 
#define MSG_HIT       5

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
} Params;

// --- CHANGED: Global status variable ---
char global_current_status[64] = "Initializing";

// Global variable to store the name 
char global_process_name[32] = "UNKNOWN";

void get_params(Params *p) {
    p->M = 1.0; p->K = 0.5; p->T = 0.1; p->W = 80; p->H = 24;
    
    FILE *f = fopen("params.txt", "r");
    if (!f) return;
    char key[32];
    while(fscanf(f, "%s", key) != EOF) {
        if (strcmp(key, "M") == 0) fscanf(f, "%f", &p->M);
        else if (strcmp(key, "K") == 0) fscanf(f, "%f", &p->K);
        else if (strcmp(key, "T") == 0) fscanf(f, "%f", &p->T);
        else if (strcmp(key, "Width") == 0) fscanf(f, "%d", &p->W);
        else if (strcmp(key, "Height") == 0) fscanf(f, "%d", &p->H);
    }
    fclose(f);
}

// The function that replies to the Watchdog 
void watchdog_handler(int sig) {
    if (sig == SIGUSR1) {
        // --- CHANGED: Logs the dynamic status ---
        log_message(LOG_FILE_wd, global_process_name, global_current_status);
    }
}

// The "All-in-One" Setup Function 
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
