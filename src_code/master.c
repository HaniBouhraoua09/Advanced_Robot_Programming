#include "drone.h"

int main() {
    // 1. Setup Params (Only if file doesn't exist)
    if (access("params.txt", F_OK) != 0) {
        FILE *f = fopen("params.txt", "w");
        if (f) { 
            // CHANGED: Added WatchdogT 2
            fprintf(f, "M 1.0\nK 0.5\nT 0.1\nWidth 80\nHeight 24\nWatchdogT 2\n"); 
            fclose(f); 
        }
    }

    // 2. CREATE 7 PIPES
    int p_ctrl_bb[2];  
    int p_obs_bb[2];   
    int p_targ_bb[2];  
    int p_dyn_bb[2];   
    
    int p_bb_dyn[2];   
    int p_bb_map[2];   
    int p_bb_ctrl[2];  

    if (pipe(p_ctrl_bb) < 0 || pipe(p_obs_bb) < 0 || pipe(p_targ_bb) < 0 || pipe(p_dyn_bb) < 0 ||
        pipe(p_bb_dyn) < 0 || pipe(p_bb_map) < 0 || pipe(p_bb_ctrl) < 0) {
        perror("Pipe creation failed"); exit(1);
    }
    

    // Clear old log files
    FILE *f;
    f = fopen(LOG_FILE_sys, "w"); if(f) fclose(f);
    f = fopen(LOG_FILE_wd, "w");  if(f) fclose(f);
    f = fopen(PID_FILE, "w");     if(f) fclose(f);

    // Initialize logger for Master
    log_message(LOG_FILE_sys, "MASTER", "System starting up...");
    

    char arg1[10], arg2[10], arg3[10], arg4[10], arg5[10], arg6[10], arg7[10];

    // 3. LAUNCH BLACKBOARD
    pid_t bb_pid = fork();
    if (bb_pid == 0) {
        sprintf(arg1, "%d", p_ctrl_bb[0]);
        sprintf(arg2, "%d", p_obs_bb[0]);
        sprintf(arg3, "%d", p_targ_bb[0]);
        sprintf(arg4, "%d", p_dyn_bb[0]);
        sprintf(arg5, "%d", p_bb_dyn[1]);
        sprintf(arg6, "%d", p_bb_map[1]);
        sprintf(arg7, "%d", p_bb_ctrl[1]);
        execlp("./blackboard", "./blackboard", arg1, arg2, arg3, arg4, arg5, arg6, arg7, NULL);
        exit(1);
    }

    // 4. LAUNCH DYNAMICS
    pid_t dyn_pid = fork();
    if (dyn_pid == 0) {
        sprintf(arg1, "%d", p_bb_dyn[0]);
        sprintf(arg2, "%d", p_dyn_bb[1]);
        execlp("./dynamics", "./dynamics", arg1, arg2, NULL);
        exit(1);
    }

    // 5. LAUNCH OBSTACLES (Capture PID)
    pid_t obs_pid = fork();
    if (obs_pid == 0) {
        sprintf(arg1, "%d", p_obs_bb[1]);
        execlp("./obstacles", "./obstacles", arg1, NULL);
        exit(0);
    }

    // 6. LAUNCH TARGETS (Capture PID)
    pid_t targ_pid = fork();
    if (targ_pid == 0) {
        sprintf(arg1, "%d", p_targ_bb[1]);
        execlp("./targets", "./targets", arg1, NULL);
        exit(0);
    }

    // 7. LAUNCH MAP WINDOW
    pid_t map_pid = fork();
    if (map_pid == 0) {
        sprintf(arg1, "%d", p_bb_map[0]);
        int null = open("/dev/null", O_WRONLY); dup2(null, 2); 
        execlp("konsole", "konsole", "-e", "./map_window", arg1, NULL);
        exit(0);
    }
    

    // 8. LAUNCH CONTROL WINDOW
    pid_t ctrl_pid = fork();
    if (ctrl_pid == 0) {
        sprintf(arg1, "%d", p_ctrl_bb[1]); 
        sprintf(arg2, "%d", p_bb_ctrl[0]); 
        int null = open("/dev/null", O_WRONLY); dup2(null, 2);
        execlp("konsole", "konsole", "-e", "./control", arg1, arg2, NULL);
        exit(0);
    }
    
    // 9. Watchdog Launch
    pid_t wd_pid = fork();
    if (wd_pid == 0) {
        // Opens in a new terminal window
        execlp("konsole", "konsole", "-e", "./watchdog", NULL);
        exit(0);
    }
    
    // --- PRINT PIDs ---
    printf("Blackboard pid is %d\n", bb_pid);
    printf("Dynamics pid is %d\n", dyn_pid);
    printf("Control_window pid is %d\n", ctrl_pid);
    printf("Konsole of Map pid is %d\n", map_pid);
    printf("Target pid is %d\n", targ_pid);
    printf("Obstacles pid is %d\n", obs_pid);
    // --------------------------------------------

    // 9. CLOSE PIPES IN MASTER
    close(p_ctrl_bb[0]); close(p_ctrl_bb[1]);
    close(p_obs_bb[0]);  close(p_obs_bb[1]);
    close(p_targ_bb[0]); close(p_targ_bb[1]);
    close(p_dyn_bb[0]);  close(p_dyn_bb[1]);
    close(p_bb_dyn[0]);  close(p_bb_dyn[1]);
    close(p_bb_map[0]);  close(p_bb_map[1]);
    close(p_bb_ctrl[0]); close(p_bb_ctrl[1]);

    // 10. Wait and Cleanup
    waitpid(bb_pid, NULL, 0);

    printf("Stopping...\n");
    kill(dyn_pid, SIGTERM);
    kill(map_pid, SIGTERM);
    kill(ctrl_pid, SIGTERM);
    
    // Now we can kill these specifically instead of using killall
    kill(obs_pid, SIGTERM);
    kill(targ_pid, SIGTERM);
    
    // Safety net
    system("killall obstacles targets map_window control 2>/dev/null");
    
    return 0;
}
