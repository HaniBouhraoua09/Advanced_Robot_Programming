// logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>

// File paths
#define LOG_FILE_sys "system.log"
#define LOG_FILE_wd  "watchdog.log"
#define PID_FILE     "pids.txt"

// Function Prototypes
void log_message(const char *filename, const char *process_name, const char *msg);
void register_pid(const char *process_name, int pid);

#endif
