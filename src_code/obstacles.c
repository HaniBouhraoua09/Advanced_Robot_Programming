#include "drone.h"
#include <time.h>
#include <math.h>

#define MIN_DIST 6.0 

int main(int argc, char *argv[]) {

    // This now does Registering AND Listening
    setup_watchdog("OBSTACLES"); 
    
    if (argc < 2) return 1;
    int fd_out = atoi(argv[1]);

    Params p;
    // Seed with +1 to ensure different random sequence from targets
    srand(time(NULL) + 1);
    
    Message msg; 
    msg.type = MSG_OBSTACLE;
    struct { float x, y; } batch[15];

    // Wait 1 second to sync start with Targets
    strcpy(global_current_status, "Initializing");
    sleep(1); 
    
    while(1) {
        strcpy(global_current_status, "Reading Params");
        get_params(&p); 
        if (p.W < 10) { sleep(1); continue; }

        // Generate 10 Obstacles (matching the 10 Targets)
        strcpy(global_current_status, "Generating Obstacles");
        for(int i=0; i<10; i++) {
            msg.id = i; 
            int valid = 0, attempts = 0;
            
            // Try to find a valid spot
            while(!valid && attempts < 50) {
                msg.x = 2 + rand() % (p.W - 5); 
                msg.y = 2 + rand() % (p.H - 5);
                valid = 1;
                
                // Check distance against previous obstacles in this batch
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
        
        // --- CHANGED: Sleep loop to handle Watchdog interruptions ---
        int t = 60; // Wait 60 seconds (or read from params if you prefer)
        while (t > 0) {
            t = sleep(t);
        }
        // ----------------------------------------------------------
    }
    return 0;
}
