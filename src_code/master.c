// master.c
#include "drone.h"

int main() {
    // 1. Ask User for Mode
    char mode;
    char ip_addr[64] = "127.0.0.1";
    printf("Select Mode:\n");
    printf("1. Standalone (Assignment 2)\n");
    printf("2. Server (Assignment 3)\n");
    printf("3. Client (Assignment 3)\n");
    printf("Choice: ");
    scanf(" %c", &mode);

    if (mode == '3') {
        printf("Enter Server IP: ");
        scanf("%s", ip_addr);
    }

    // 2. Setup Params
    if (access("params.txt", F_OK) != 0) {
        FILE *f = fopen("params.txt", "w");
        if (f) { 
            fprintf(f, "M 1.0\nK 0.5\nT 0.1\nWidth 80\nHeight 24\nWatchdogT 2\n"); 
            fclose(f); 
        }
    }

    // 3. Create Pipes
    int p_ctrl_bb[2], p_obs_bb[2], p_targ_bb[2], p_dyn_bb[2];
    int p_bb_dyn[2], p_bb_map[2], p_bb_ctrl[2];

    pipe(p_ctrl_bb); pipe(p_obs_bb); pipe(p_targ_bb); pipe(p_dyn_bb);
    pipe(p_bb_dyn);  pipe(p_bb_map);  pipe(p_bb_ctrl);

    // Cleanup Logs
    FILE *f_log;
    f_log = fopen(LOG_FILE_sys, "w"); if(f_log) fclose(f_log);
    f_log = fopen(LOG_FILE_wd, "w");  if(f_log) fclose(f_log);
    f_log = fopen(PID_FILE, "w");     if(f_log) fclose(f_log);

    // 4. Launch Blackboard with Mode Arguments
    pid_t bb_pid = fork();
    if (bb_pid == 0) {
        char arg_fds[7][10];
        sprintf(arg_fds[0], "%d", p_ctrl_bb[0]);
        sprintf(arg_fds[1], "%d", p_obs_bb[0]);
        sprintf(arg_fds[2], "%d", p_targ_bb[0]);
        sprintf(arg_fds[3], "%d", p_dyn_bb[0]);
        sprintf(arg_fds[4], "%d", p_bb_dyn[1]);
        sprintf(arg_fds[5], "%d", p_bb_map[1]);
        sprintf(arg_fds[6], "%d", p_bb_ctrl[1]);

        char mode_str[2] = {mode, '\0'};
        // Pass Mode and IP to Blackboard
        execlp("./blackboard", "./blackboard", 
               arg_fds[0], arg_fds[1], arg_fds[2], arg_fds[3], 
               arg_fds[4], arg_fds[5], arg_fds[6], 
               mode_str, ip_addr, NULL);
        exit(1);
    }

    // 5. Launch Dynamics
    pid_t dyn_pid = fork();
    if (dyn_pid == 0) {
        char in[10], out[10];
        sprintf(in, "%d", p_bb_dyn[0]);
        sprintf(out, "%d", p_dyn_bb[1]);
        execlp("./dynamics", "./dynamics", in, out, NULL);
        exit(1);
    }

    // 6. Launch Windows
    pid_t map_pid = fork();
    if (map_pid == 0) {
        char in[10]; sprintf(in, "%d", p_bb_map[0]);
        execlp("konsole", "konsole", "-e", "./map_window", in, NULL);
        exit(0);
    }
    pid_t ctrl_pid = fork();
    if (ctrl_pid == 0) {
        char in[10], out[10];
        sprintf(out, "%d", p_ctrl_bb[1]); sprintf(in, "%d", p_bb_ctrl[0]);
        execlp("konsole", "konsole", "-e", "./control", out, in, NULL);
        exit(0);
    }

    // 7. Conditional Launch (Only Standalone) 
    pid_t obs_pid = -1, targ_pid = -1, wd_pid = -1;

    if (mode == '1') {
        obs_pid = fork();
        if (obs_pid == 0) {
            char out[10]; sprintf(out, "%d", p_obs_bb[1]);
            execlp("./obstacles", "./obstacles", out, NULL);
            exit(0);
        }
        targ_pid = fork();
        if (targ_pid == 0) {
            char out[10]; sprintf(out, "%d", p_targ_bb[1]);
            execlp("./targets", "./targets", out, NULL);
            exit(0);
        }
        wd_pid = fork();
        if (wd_pid == 0) {
            execlp("konsole", "konsole", "-e", "./watchdog", NULL);
            exit(0);
        }
    }

    // Close unused pipes
    close(p_ctrl_bb[0]); close(p_ctrl_bb[1]);
    close(p_obs_bb[0]);  close(p_obs_bb[1]);
    close(p_targ_bb[0]); close(p_targ_bb[1]);
    close(p_dyn_bb[0]);  close(p_dyn_bb[1]);
    close(p_bb_dyn[0]);  close(p_bb_dyn[1]);
    close(p_bb_map[0]);  close(p_bb_map[1]);
    close(p_bb_ctrl[0]); close(p_bb_ctrl[1]);

    waitpid(bb_pid, NULL, 0);

    kill(dyn_pid, SIGTERM);
    kill(map_pid, SIGTERM);
    kill(ctrl_pid, SIGTERM);
    if (obs_pid > 0) kill(obs_pid, SIGTERM);
    if (targ_pid > 0) kill(targ_pid, SIGTERM);
    if (wd_pid > 0) kill(wd_pid, SIGTERM);
    
    system("killall obstacles targets map_window control 2>/dev/null");
    return 0;
}
