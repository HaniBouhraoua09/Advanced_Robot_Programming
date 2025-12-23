#include "drone.h"
#include <ncurses.h>
#include <signal.h>
#include <time.h>

// Structure to track monitored processes
typedef struct {
    char name[32];
    int pid;
    int last_seen;
} MonitoredProcess;

MonitoredProcess processes[10];
int process_count = 0;

void load_pids() {
    process_count = 0;
    FILE *f = fopen(PID_FILE, "r");
    if (!f) return;
    
    int fd = fileno(f);
    struct flock lock;
    lock.l_type = F_RDLCK; 
    lock.l_whence = SEEK_SET; lock.l_start = 0; lock.l_len = 0;
    fcntl(fd, F_SETLKW, &lock);

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *ptr_name = strchr(line, ']'); 
        if (!ptr_name) continue;
        ptr_name++; 
        
        char *ptr_msg = strchr(ptr_name, ']');
        if (!ptr_msg) continue;
        
        int name_len = ptr_msg - (ptr_name + 2);
        if (name_len > 30) name_len = 30;
        
        char name_buf[32];
        strncpy(name_buf, ptr_name + 2, name_len);
        name_buf[name_len] = '\0';
        
        int pid = atoi(ptr_msg + 2);
        
        if (pid > 0) {
            strncpy(processes[process_count].name, name_buf, 32);
            processes[process_count].pid = pid;
            process_count++;
            if(process_count >= 10) break;
        }
    }
    
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    fclose(f);
}

int main(int argc, char *argv[]) {
    initscr(); cbreak(); noecho(); curs_set(0);
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    
    WINDOW *win = newwin(20, 50, 0, 0); 
    wbkgd(win, COLOR_PAIR(1));
    box(win, 0, 0);
    mvwprintw(win, 1, 20, "WATCHDOG");
    wrefresh(win);
    
    log_message(LOG_FILE_sys, "WATCHDOG", "Process started.");

    while(1) {
        // --- CHANGED: Read Sleep Time from Params ---
        // Default value if reading file fails
        int watchdog_T = 2; 
        FILE *fp = fopen("params.txt", "r");
        if (fp) {
            char key[32]; int val;
            while(fscanf(fp, "%s", key) != EOF) {
                if (strcmp(key, "WatchdogT") == 0) {
                    fscanf(fp, "%d", &val);
                    watchdog_T = val;
                }
            }
            fclose(fp);
        }
        if (watchdog_T < 1) watchdog_T = 1;
        // -------------------------------------------

        load_pids();
        
        werase(win);
        box(win, 0, 0);
        wattron(win, A_BOLD);
        mvwprintw(win, 1, 2, "Monitoring %d processes...", process_count);
        wattroff(win, A_BOLD);

        int y = 3;
        for(int i=0; i<process_count; i++) {
            if (kill(processes[i].pid, 0) == 0) {
                kill(processes[i].pid, SIGUSR1);
                
                wattron(win, COLOR_PAIR(2));
                mvwprintw(win, y++, 2, "%-12s [%d]: ALIVE (Signaled)", processes[i].name, processes[i].pid);
                wattroff(win, COLOR_PAIR(2));
            } else {
                wattron(win, COLOR_PAIR(3));
                mvwprintw(win, y++, 2, "%-12s [%d]: DEAD/MISSING", processes[i].name, processes[i].pid);
                wattroff(win, COLOR_PAIR(3));
                
                char err_msg[64];
                snprintf(err_msg, sizeof(err_msg), "%s is unresponsive!", processes[i].name);
                log_message(LOG_FILE_sys, "WATCHDOG", err_msg);
            }
        }
        
        wrefresh(win);
        log_message(LOG_FILE_wd, "WATCHDOG", "Cycle completed.");
        
        // --- CHANGED: Use the variable time ---
        sleep(watchdog_T); 
    }
    
    endwin();
    return 0;
}
