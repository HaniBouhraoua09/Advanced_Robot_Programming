#include "logger.h"

void log_message(const char *filename, const char *process_name, const char *msg) {
    // 1. Open the file in Append mode
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) return;

    // 2. Setup the Lock Structure
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK; // Write Lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;        // Lock the entire file

    // 3. Acquire Lock (Blocking wait)
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        close(fd);
        return;
    }

    // 4. Write Content
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    // Remove newline from ctime result if present
    if (time_str[strlen(time_str) - 1] == '\n') {
        time_str[strlen(time_str) - 1] = '\0';
    }

    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[%s] [%s] %s\n", time_str, process_name, msg);
    write(fd, buffer, strlen(buffer));

    // --- CRITICAL FIX: Flush to disk before unlocking ---
    fsync(fd);
    // --------------------------------------------------

    // 5. Release Lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);

    // 6. Close File
    close(fd);
}

// Registers the process in a file so the Watchdog knows who to check
void register_pid(const char *process_name, int pid) {
    char msg[64];
    snprintf(msg, sizeof(msg), "%d", pid);
    // Reuse the safe logging function to write to pids.txt
    log_message(PID_FILE, process_name, msg);
}
