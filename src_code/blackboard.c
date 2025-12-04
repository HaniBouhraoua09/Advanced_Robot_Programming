#include "drone.h"

int main(int argc, char *argv[]) {
    if (argc < 8) return 1;
    // Input FDs
    int fd_ctrl = atoi(argv[1]);
    int fd_obs  = atoi(argv[2]);
    int fd_targ = atoi(argv[3]);
    int fd_dyn  = atoi(argv[4]);
    // Output FDs
    int out_dyn = atoi(argv[5]);
    int out_map = atoi(argv[6]);
    int out_ctrl= atoi(argv[7]);

    printf("BB Running. Inputs: %d %d %d %d\n", fd_ctrl, fd_obs, fd_targ, fd_dyn);

    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(fd_ctrl, &master_set);
    FD_SET(fd_obs, &master_set);
    FD_SET(fd_targ, &master_set);
    FD_SET(fd_dyn, &master_set);
    
    int max_fd = fd_ctrl;
    if (fd_obs > max_fd) max_fd = fd_obs;
    if (fd_targ > max_fd) max_fd = fd_targ;
    if (fd_dyn > max_fd) max_fd = fd_dyn;

    Message msg;
    int running = 1;

    while(running) {
        read_set = master_set;
        if (select(max_fd+1, &read_set, NULL, NULL, NULL) > 0) {
            
            // 1. Control -> Dynamics
            if (FD_ISSET(fd_ctrl, &read_set)) {
                if (read(fd_ctrl, &msg, sizeof(Message)) > 0) {
                    if (msg.type == MSG_STOP) running = 0;
                    write(out_dyn, &msg, sizeof(Message)); 
                }
            }
            // 2. Obstacles -> Dynamics & Map
            if (FD_ISSET(fd_obs, &read_set)) {
                if (read(fd_obs, &msg, sizeof(Message)) > 0) {
                    write(out_dyn, &msg, sizeof(Message));
                    write(out_map, &msg, sizeof(Message));
                }
            }
            // 3. Targets -> Map & Dynamics
            if (FD_ISSET(fd_targ, &read_set)) {
                if (read(fd_targ, &msg, sizeof(Message)) > 0) {
                    write(out_dyn, &msg, sizeof(Message)); // For collision
                    write(out_map, &msg, sizeof(Message)); // For display
                }
            }
            // 4. Dynamics -> Map & Control
            if (FD_ISSET(fd_dyn, &read_set)) {
                if (read(fd_dyn, &msg, sizeof(Message)) > 0) {
                    if (msg.type == MSG_DRONE_POS) {
                        write(out_map, &msg, sizeof(Message));
                        write(out_ctrl, &msg, sizeof(Message));
                    }
                    else if (msg.type == MSG_HIT) {
                        write(out_map, &msg, sizeof(Message));
                    }
                }
            }
        }
    }
    
    // Stop Signal
    msg.type = MSG_STOP;
    write(out_dyn, &msg, sizeof(Message));
    write(out_map, &msg, sizeof(Message));
    write(out_ctrl, &msg, sizeof(Message));

    return 0;
}
