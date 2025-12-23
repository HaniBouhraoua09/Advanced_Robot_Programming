#include "drone.h"
#include <math.h>

// Caps
#define MAX_REPULSION 100.0
#define MAX_V 20.0 

int main(int argc, char *argv[]) {

    // This now does Registering AND Listening
    setup_watchdog("DYNAMICS"); 
    
    if (argc < 3) return 1;
    int fd_in = atoi(argv[1]);  
    int fd_out = atoi(argv[2]); 

    signal(SIGPIPE, SIG_IGN); 
    Params p;
    Message state, out_msg;
    
    float x = 10, y = 10, vx = 0, vy = 0;
    float current_fx = 0, current_fy = 0; 
    
    struct { float x, y; int active; } obs[40];
    struct { float x, y; int active; } targ[40]; 

    int next_sequence_id = 0; 
    for(int i=0; i<40; i++) { obs[i].active = 0; targ[i].active = 0; }

    int prev_W = -1, prev_H = -1;

    // Directions
    float diag = 0.7071f; 
    float dir_x[8] = {1.0,  diag, 0.0, -diag, -1.0, -diag,  0.0,  diag};
    float dir_y[8] = {0.0, -diag, -1.0, -diag,  0.0,  diag,  1.0,  diag}; 
    
    while(1) {
        strcpy(global_current_status, "Reading Params");
        get_params(&p); 

        // Resize Logic
        if (prev_W != -1 && (p.W != prev_W || p.H != prev_H)) {
            float scale_x = (float)p.W / (float)prev_W;
            float scale_y = (float)p.H / (float)prev_H;
            x *= scale_x; y *= scale_y;
            for(int i=0; i<40; i++) {
                if(obs[i].active) { obs[i].x *= scale_x; obs[i].y *= scale_y; }
                if(targ[i].active) { targ[i].x *= scale_x; targ[i].y *= scale_y; }
            }
        }
        prev_W = p.W; prev_H = p.H;

        fd_set fds; 
        struct timeval tv = {0, 0};
        FD_ZERO(&fds); FD_SET(fd_in, &fds);
        
        strcpy(global_current_status, "Processing Inputs");
        while (select(fd_in+1, &fds, NULL, NULL, &tv) > 0) {
            if (read(fd_in, &state, sizeof(Message)) > 0) {
                if (state.type == MSG_STOP) return 0;

                if (state.type == MSG_INPUT) {
                    current_fx = state.fx; current_fy = state.fy;
                }
                else if (state.type == MSG_OBSTACLE) {
                    if (state.id >= 0 && state.id < 40) {
                        obs[state.id].x = state.x; obs[state.id].y = state.y;
                        obs[state.id].active = 1;
                    }
                }
                else if (state.type == MSG_TARGET) {
                    if (state.id >= 0 && state.id < 40) {
                        if (state.id == 0) next_sequence_id = 0;
                        targ[state.id].x = state.x; targ[state.id].y = state.y;
                        targ[state.id].active = 1;
                    }
                }
            } else break;
            FD_ZERO(&fds); FD_SET(fd_in, &fds);
        }

        strcpy(global_current_status, "Calculating Physics");

        // 1. Raw Repulsion
        float rep_x = 0, rep_y = 0;
        float eta = 100.0, rho = 5.0; 

        for(int i=0; i<40; i++) {
            if(!obs[i].active) continue;
            float dx = x - obs[i].x; float dy = y - obs[i].y;
            float dist = sqrt(dx*dx + dy*dy);
            
            if (dist < rho && dist > 0.1) {
                float mag = eta * (1.0/dist - 1.0/rho) / (dist*dist);
                if (mag > MAX_REPULSION) mag = MAX_REPULSION;
                rep_x += mag * (dx/dist); 
                rep_y += mag * (dy/dist);
            }
        }

        // 2. Virtual Key
        float virtual_fx = 0, virtual_fy = 0;
        if (fabs(rep_x) > 0.1 || fabs(rep_y) > 0.1) {
            float max_dot = -1.0; int best_dir = -1;
            for(int k=0; k<8; k++) {
                float dot = (rep_x * dir_x[k]) + (rep_y * dir_y[k]);
                if (dot > max_dot) { max_dot = dot; best_dir = k; }
            }
            if (best_dir != -1 && max_dot > 0) {
                virtual_fx = dir_x[best_dir] * max_dot;
                virtual_fy = dir_y[best_dir] * max_dot;
            }
        }

        // 3. Targets
        for(int i=0; i<40; i++) {
            if(!targ[i].active) continue;
            float dx = x - targ[i].x; float dy = y - targ[i].y;
            if (sqrt(dx*dx + dy*dy) < 2.0) {
                if (i == next_sequence_id) {
                    targ[i].active = 0; 
                    Message hit_msg; hit_msg.type = MSG_HIT; hit_msg.id = i;
                    write(fd_out, &hit_msg, sizeof(Message));
                    next_sequence_id++;
                }
            }
        }

        // 4. Integration
        float total_fx = current_fx + virtual_fx;
        float total_fy = current_fy + virtual_fy;

        float ax = (total_fx - p.K * vx) / p.M;
        float ay = (total_fy - p.K * vy) / p.M;
        
        vx += ax * p.T; vy += ay * p.T;
        if(vx > MAX_V) vx = MAX_V; if(vx < -MAX_V) vx = -MAX_V;
        if(vy > MAX_V) vy = MAX_V; if(vy < -MAX_V) vy = -MAX_V;
        
        x  += vx * p.T; y  += vy * p.T;

        // Walls
        if(x <= 1) { x = 1.1; vx = -vx * 0.8; }
        if(x >= p.W - 2) { x = p.W - 2.1; vx = -vx * 0.8; }
        if(y <= 3) { y = 3.1; vy = -vy * 0.8; } 
        if(y >= p.H - 1) { y = p.H - 1.1; vy = -vy * 0.8; }

        out_msg.type = MSG_DRONE_POS;
        out_msg.x = x; out_msg.y = y;
        out_msg.vx = vx; out_msg.vy = vy;
        out_msg.fx = current_fx; out_msg.fy = current_fy;
        
        write(fd_out, &out_msg, sizeof(Message));

        usleep(30000); 
    }
    return 0;
}
