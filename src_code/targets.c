#include "drone.h"
#include <time.h>
#include <math.h>

#define MIN_DIST 5.0

int main(int argc, char *argv[]) {

    // This now does Registering AND Listening
    setup_watchdog("TARGETS");

    if (argc < 2) return 1;
    int fd_out = atoi(argv[1]);

    Params p;
    srand(time(NULL) + 2);
    Message msg; msg.type = MSG_TARGET;
    struct { float x, y; } batch[20];

    // Sync start
    strcpy(global_current_status, "Initializing");
    sleep(1); 
    
    while(1) {
        strcpy(global_current_status, "Reading Params");
        get_params(&p); 
        if (p.W < 10) { sleep(1); continue; }

        strcpy(global_current_status, "Generating Targets");
        for(int i=0; i<9; i++) {
            msg.id = i; 
            
            int valid = 0, attempts = 0;
            while(!valid && attempts < 50) {
                msg.x = 2 + rand() % (p.W - 5);
                msg.y = 3 + rand() % (p.H - 6);
                valid = 1;
                for(int j=0; j<i; j++) {
                    float dist = sqrt(pow(msg.x-batch[j].x,2) + pow(msg.y-batch[j].y,2));
                    if (dist < MIN_DIST) { valid = 0; break; }
                }
                attempts++;
            }
            batch[i].x = msg.x; batch[i].y = msg.y;
            write(fd_out, &msg, sizeof(Message));
        }
        
        strcpy(global_current_status, "Sleeping");
        
        // --- CHANGED: Sleep loop to handle Watchdog interruption ---
        int t = 60; // 60 seconds interval
        while (t > 0) {
            t = sleep(t);
        }
        // ---------------------------------------------------------
    }
    return 0;
}
