#include "drone.h"
#include <sys/time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <locale.h> 

// --- HELPER: Send with small delay (Prevents packet merging) ---
void net_send(int sock, const char *msg) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s\n", msg); // Ensure newline
    write(sock, buf, strlen(buf)); 
    usleep(1000); // 1ms delay is enough
}

// --- HELPER: Smart Reader (Byte-by-byte to handle TCP Stream) ---
int net_recv_line(int sock, char *buf, int size) {
    memset(buf, 0, size);
    char c;
    int i = 0;
    while (i < size - 1) {
        int n = read(sock, &c, 1);
        if (n <= 0) return -1; // Error or Disconnect
        
        // Skip leading nulls/junk
        if (i == 0 && (c == '\0' || c == '\r' || c == '\n')) continue;

        if (c == '\n') break; // Stop at newline
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

// --- HELPER: Robust Coordinate Parser ---
int parse_coordinates(char *buf, float *x, float *y) {
    // 1. Replace commas with spaces (Fix European format)
    for(int k=0; buf[k]; k++) if(buf[k] == ',') buf[k] = ' ';
    
    // 2. Find the first digit or minus sign
    char *ptr = buf;
    while (*ptr && !isdigit(*ptr) && *ptr != '-') ptr++;

    // 3. Scan
    if (sscanf(ptr, "%f %f", x, y) == 2) return 1; // Success
    return 0; // Fail
}

// Coordinate Transform
float to_net_y(float y, int H) { return (float)(H - 1) - y; }
float from_net_y(float y, int H) { return (float)(H - 1) - y; }

// --- CLAMP FUNCTION (Keeps drone on screen) ---
void clamp_coordinates(float *x, float *y, int W, int H) {
    if (*x < 1) *x = 1;
    if (*x >= W-1) *x = W-2;
    if (*y < 1) *y = 1;
    if (*y >= H-1) *y = H-2;
}

int main(int argc, char *argv[]) {
    // Force Dot for decimals (e.g. 10.5 instead of 10,5)
    setlocale(LC_NUMERIC, "C");
    if (argc < 10) return 1;

    int fd_ctrl = atoi(argv[1]);
    int fd_obs  = atoi(argv[2]);
    int fd_targ = atoi(argv[3]);
    int fd_dyn  = atoi(argv[4]);
    int out_dyn = atoi(argv[5]);
    int out_map = atoi(argv[6]);
    int out_ctrl= atoi(argv[7]);
    
    char mode = argv[8][0]; 
    char *ip_addr = argv[9];

    // Params Setup
    Params p; 
    p.W = 80; p.H = 24; // Defaults in case file fails
    get_params(&p);     // Load actual params (void return, so no check needed)

    int max_fd = (fd_dyn > fd_ctrl) ? fd_dyn : fd_ctrl;
    if(mode == '1') {
        if(fd_obs > max_fd) max_fd = fd_obs;
        if(fd_targ > max_fd) max_fd = fd_targ;
    }

    int net_sock = -1;
    struct sockaddr_in serv_addr;
    char buffer[256];
    
    // --- NETWORK SETUP ---
    if (mode == '2') { // SERVER
        int server_fd;
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { perror("socket"); exit(1); }
        int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
        
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(PORT);
        
        if(bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Bind failed"); exit(1);
        }
        listen(server_fd, 3);
        printf("SERVER: Waiting for client on port %d...\n", PORT);
        
        int addrlen = sizeof(serv_addr);
        net_sock = accept(server_fd, (struct sockaddr *)&serv_addr, (socklen_t*)&addrlen);
        printf("SERVER: Connected!\n");

        // PROTOCOL: Handshake
        net_send(net_sock, "ok");       
        net_recv_line(net_sock, buffer, 256); // Rcv "ook"
        
        sprintf(buffer, "%d,%d", p.W, p.H); 
        net_send(net_sock, buffer);     
        net_recv_line(net_sock, buffer, 256); // Rcv "sok"

    } else if (mode == '3') { // CLIENT
        if ((net_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { perror("socket"); exit(1); }
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr);

        if (connect(net_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("Connection Failed. Make sure Server is running.\n");
            exit(1);
        }
        printf("CLIENT: Connected to %s\n", ip_addr);

        // PROTOCOL: Handshake
        net_recv_line(net_sock, buffer, 256);
        net_send(net_sock, "ook");      
        
        net_recv_line(net_sock, buffer, 256);
        
        // Parse size aggressively
        for(int k=0; buffer[k]; k++) if(buffer[k] == ',') buffer[k] = ' ';
        char *ptr = buffer;
        while (*ptr && !isdigit(*ptr)) ptr++;
        sscanf(ptr, "%d %d", &p.W, &p.H); 
        
        sprintf(buffer, "sok %d %d", p.W, p.H); 
        //stef changed this net_send(net_sock, buffer);
        net_send(net_sock,"sok");
    }

    Message msg;
    float my_x = 10, my_y = 10;
    
    setup_watchdog(mode == '2' ? "SERVER_BB" : (mode == '3' ? "CLIENT_BB" : "BB_SOLO"));
    
    fd_set read_set;
    int running = 1;

    struct timeval last_net_time, current_time;
    gettimeofday(&last_net_time, NULL);

    while(running) {
        strcpy(global_current_status, "Processing");
        struct timeval tv_zero = {0, 0};
        fd_set check_set;

        // 1. READ LOCAL PIPES (Drain everything!)

        // A. Control (Drain Loop)
        FD_ZERO(&check_set); FD_SET(fd_ctrl, &check_set);
        if (select(fd_ctrl+1, &check_set, NULL, NULL, &tv_zero) > 0) {
            while (read(fd_ctrl, &msg, sizeof(Message)) > 0) {
                if (msg.type == MSG_STOP) running = 0;
                write(out_dyn, &msg, sizeof(Message));

                FD_ZERO(&check_set); FD_SET(fd_ctrl, &check_set);
                if (select(fd_ctrl+1, &check_set, NULL, NULL, &tv_zero) <= 0) break;
            }
        }

        // B. Dynamics (Drain Loop)
        FD_ZERO(&check_set); FD_SET(fd_dyn, &check_set);
        if (select(fd_dyn+1, &check_set, NULL, NULL, &tv_zero) > 0) {
            while (read(fd_dyn, &msg, sizeof(Message)) > 0) {
                if (msg.type == MSG_DRONE_POS) {
                    my_x = msg.x; my_y = msg.y;
                    write(out_map, &msg, sizeof(Message));
                    write(out_ctrl, &msg, sizeof(Message));
                }
                // --- FIX: HANDLE HIT MESSAGES ---
                else if (msg.type == MSG_HIT) {
                    write(out_map, &msg, sizeof(Message));
                }
                // --------------------------------

                FD_ZERO(&check_set); FD_SET(fd_dyn, &check_set);
                if (select(fd_dyn+1, &check_set, NULL, NULL, &tv_zero) <= 0) break;
            }
        }

        // C. Obstacles/Targets (Only Mode 1)
        if (mode == '1') {
            FD_ZERO(&read_set);
            FD_SET(fd_obs, &read_set);
            FD_SET(fd_targ, &read_set);
            if (select(max_fd+1, &read_set, NULL, NULL, &tv_zero) > 0) {
                if (FD_ISSET(fd_obs, &read_set)) {
                    read(fd_obs, &msg, sizeof(Message));
                    write(out_map, &msg, sizeof(Message));
                    write(out_dyn, &msg, sizeof(Message));
                }
                if (FD_ISSET(fd_targ, &read_set)) {
                    read(fd_targ, &msg, sizeof(Message));
                    write(out_map, &msg, sizeof(Message));
                    write(out_dyn, &msg, sizeof(Message));
                }
            }
        }

        // 2. SERVER LOGIC (Rate Limited)
        if (mode == '2') { 
            gettimeofday(&current_time, NULL);
            long elapsed = (current_time.tv_sec - last_net_time.tv_sec) * 1000000 + 
                           (current_time.tv_usec - last_net_time.tv_usec);

            if (elapsed >= 50000) {  // 20Hz (50ms)
                last_net_time = current_time; 

                net_send(net_sock, "drone");
                
                sprintf(buffer, "%.2f %.2f", my_x, to_net_y(my_y, p.H));
                net_send(net_sock, buffer);
                
                if (net_recv_line(net_sock, buffer, 256) > 0) { // Wait Ack
                    net_send(net_sock, "obst"); 
                    if (net_recv_line(net_sock, buffer, 256) > 0) { // Get Coords
                        float cx, cy;
                        if (parse_coordinates(buffer, &cx, &cy)) {
                            // Clamp and Draw
                            float screen_y = from_net_y(cy, p.H);
                            clamp_coordinates(&cx, &screen_y, p.W, p.H);
                            
                            net_send(net_sock, "pok"); 
                            
                            Message obs_msg;
                            obs_msg.type = MSG_OBSTACLE; 
                            obs_msg.id = 0; 
                            obs_msg.x = cx; 
                            obs_msg.y = screen_y;
                            write(out_map, &obs_msg, sizeof(Message));
                            write(out_dyn, &obs_msg, sizeof(Message));
                        }
                    }
                }
            } 
        }
        
        // 3. CLIENT LOGIC (ALWAYS CHECK - NOT Rate Limited!)
        if (mode == '3') { 
            fd_set net_set;
            FD_ZERO(&net_set);
            FD_SET(net_sock, &net_set);
            struct timeval net_tv = {0, 0};

            if (select(net_sock+1, &net_set, NULL, NULL, &net_tv) > 0) {
                if (net_recv_line(net_sock, buffer, 256) > 0) {
                    
                    if (strncmp(buffer, "drone", 5) == 0) {
                        float sx, sy;
                        int found = parse_coordinates(buffer, &sx, &sy);
                        
                        // Handle possible merged packets
                        if (!found) {
                            if (net_recv_line(net_sock, buffer, 256) > 0) {
                                found = parse_coordinates(buffer, &sx, &sy);
                            }
                        }

                        if (found) {
                            // Clamp and Draw
                            float screen_y = from_net_y(sy, p.H);
                            clamp_coordinates(&sx, &screen_y, p.W, p.H);

                            net_send(net_sock, "dok");
                            Message targ_msg;
                            targ_msg.type = MSG_OBSTACLE; // Visualize Server as Obstacle
                            targ_msg.id = 0;
                            targ_msg.x = sx; targ_msg.y = screen_y;
                            write(out_map, &targ_msg, sizeof(Message));
                            write(out_dyn, &targ_msg, sizeof(Message));
                        }
                    }
                    else if (strncmp(buffer, "obst", 4) == 0) {
                        sprintf(buffer, "%.2f %.2f", my_x, to_net_y(my_y, p.H));
                        net_send(net_sock, buffer);
                        net_recv_line(net_sock, buffer, 256); // Wait for pok
                    }
                    else if (strncmp(buffer, "q", 1) == 0) {
                        net_send(net_sock, "qok");
                        running = 0;
                    }
                }
            }
        }
    }
    
    if (net_sock != -1) close(net_sock);
    return 0;
}
