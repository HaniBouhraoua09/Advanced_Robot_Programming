#include "drone.h"
#include <ncurses.h>
#include <time.h>
#include <locale.h>
#include <signal.h>

struct { int active; float x, y; } obstacles[40], targets[40];
float drone_x = 10, drone_y = 10;

// Simple Score
int score = 0;

void init_colors() {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);   // Drone White
    init_pair(2, COLOR_YELLOW, COLOR_BLACK); 
    init_pair(3, COLOR_GREEN, COLOR_BLACK);  
    init_pair(4, COLOR_WHITE, COLOR_BLACK);  
}

// FIX: Preserves Physics Params on Resize
void update_params_file(int w, int h) {
    float m = 1.0, k = 0.5, t = 0.1;
    FILE *f_in = fopen("params.txt", "r");
    if (f_in) {
        char key[32];
        while(fscanf(f_in, "%s", key) != EOF) {
            if (strcmp(key, "M") == 0) fscanf(f_in, "%f", &m);
            else if (strcmp(key, "K") == 0) fscanf(f_in, "%f", &k);
            else if (strcmp(key, "T") == 0) fscanf(f_in, "%f", &t);
        }
        fclose(f_in);
    }
    FILE *f_out = fopen("params.txt", "w");
    if (f_out) {
        fprintf(f_out, "M %.2f\nK %.2f\nT %.2f\nWidth %d\nHeight %d\n", m, k, t, w, h);
        fclose(f_out);
    }
}

void layout_and_draw(WINDOW *win) {
    int H, W;
    getmaxyx(stdscr, H, W);
    wresize(win, H, W);
    mvwin(win, 0, 0);
    werase(win);
    
    // Simple Score Display
    wattron(win, A_BOLD | COLOR_PAIR(4));
    mvwprintw(win, 0, 1, "MAP DISPLAY                    [Score: %d]", score); 
    wattroff(win, A_BOLD | COLOR_PAIR(4));

    // Border
    int top_y = 2; 
    wattron(win, COLOR_PAIR(4));
    mvwaddch(win, top_y, 0, ACS_ULCORNER); mvwaddch(win, top_y, W-1, ACS_URCORNER);
    mvwaddch(win, H-1, 0, ACS_LLCORNER); mvwaddch(win, H-1, W-1, ACS_LRCORNER);
    mvwhline(win, top_y, 1, ACS_HLINE, W-2); mvwhline(win, H-1, 1, ACS_HLINE, W-2);
    mvwvline(win, top_y + 1, 0, ACS_VLINE, H - top_y - 2);
    mvwvline(win, top_y + 1, W-1, ACS_VLINE, H - top_y - 2);
    wattroff(win, COLOR_PAIR(4));

    // Obstacles
    wattron(win, COLOR_PAIR(2));
    for(int i=0; i<40; i++) {
        if(obstacles[i].active && obstacles[i].y > top_y && obstacles[i].y < H-1) 
            mvwaddch(win, (int)obstacles[i].y, (int)obstacles[i].x, 'O');
    }
    wattroff(win, COLOR_PAIR(2));

    // Targets
    wattron(win, COLOR_PAIR(3));
    for(int i=0; i<40; i++) {
        if(targets[i].active && targets[i].y > top_y && targets[i].y < H-1) {
            if (i < 9) {
                char label = i + '1'; 
                mvwaddch(win, (int)targets[i].y, (int)targets[i].x, label);
            }
        }
    }
    wattroff(win, COLOR_PAIR(3));

    // Drone
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    if(drone_y > top_y && drone_y < H-1)
        mvwaddch(win, (int)drone_y, (int)drone_x, '+');
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
    
    wrefresh(win);
}

int main(int argc, char *argv[]) {

    // This now does Registering AND Listening
    setup_watchdog("MAP_WIN");
    
    if(argc < 2) return 1;
    int fd_in = atoi(argv[1]);

    signal(SIGPIPE, SIG_IGN); 
    setlocale(LC_ALL, ""); 
    initscr(); cbreak(); noecho(); curs_set(0); 
    keypad(stdscr, TRUE); nodelay(stdscr, TRUE); 
    init_colors();

    WINDOW *win = newwin(0, 0, 0, 0);
    int H, W;
    getmaxyx(stdscr, H, W);
    
    update_params_file(W, H); 
    int prev_W = W, prev_H = H;

    refresh();
    layout_and_draw(win);

    Message msg;


    while(1) {
    
        strcpy(global_current_status, "Updating Display");
        
        int ch = getch();
        if (ch == KEY_RESIZE) { 
            resize_term(0, 0); 
            getmaxyx(stdscr, H, W);
            if (prev_W > 0 && prev_H > 0) {
                float sx = (float)W / prev_W;
                float sy = (float)H / prev_H;
                for(int i=0; i<40; i++) {
                    if(obstacles[i].active) { obstacles[i].x *= sx; obstacles[i].y *= sy; }
                    if(targets[i].active) { targets[i].x *= sx; targets[i].y *= sy; }
                }
            }
            prev_W = W; prev_H = H;
            
            update_params_file(W, H);
            layout_and_draw(win); 
        }
        if (ch == 27) break;

        fd_set fds; struct timeval tv={0,0};
        FD_ZERO(&fds); FD_SET(fd_in, &fds);
        if(select(fd_in+1, &fds, NULL, NULL, &tv) > 0) {
            while (read(fd_in, &msg, sizeof(Message)) > 0) {
                if (msg.type == MSG_DRONE_POS) { 
                    drone_x = msg.x; drone_y = msg.y; 
                }
                else if (msg.type == MSG_OBSTACLE) {
                    if(msg.id >= 0 && msg.id < 40) {
                        obstacles[msg.id].x = msg.x; obstacles[msg.id].y = msg.y; 
                        obstacles[msg.id].active = 1;
                    }
                }
                else if (msg.type == MSG_TARGET) {
                    if(msg.id >= 0 && msg.id < 40) {
                        targets[msg.id].x = msg.x; targets[msg.id].y = msg.y; 
                        targets[msg.id].active = 1; 
                    }
                }
                else if (msg.type == MSG_HIT) {
                    if(msg.id >= 0 && msg.id < 40 && targets[msg.id].active) {
                        targets[msg.id].active = 0; 
                        score += 1;                 
                    }
                }
                else if (msg.type == MSG_STOP) goto cleanup;
                
                FD_ZERO(&fds); FD_SET(fd_in, &fds);
                if(select(fd_in+1, &fds, NULL, NULL, &tv) <= 0) break;
            }
        }

        layout_and_draw(win);
        usleep(30000); 
    }

cleanup:
    endwin();
    return 0;
}
