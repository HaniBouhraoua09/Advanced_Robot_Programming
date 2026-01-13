#include "drone.h"
#include <ncurses.h>
#include <ctype.h>
#include <locale.h>

#define FORCE_STEP 2.0

float fx=0, fy=0, tx=0, ty=0, tvx=0, tvy=0;
char *grid_labels[3][3] = { {"a", "z", "e"}, {"q", "s", "d"}, {"w", "x", "c"} };

void layout_and_draw(WINDOW *win, char last_key) {
    int H, W; getmaxyx(stdscr, H, W);
    wresize(win, H, W); mvwin(win, 0, 0); werase(win);

    int mid_x = W / 2;
    box(win, 0, 0);
    for(int y=1; y<H-1; y++) mvwaddch(win, y, mid_x, ACS_VLINE); 
    mvwaddch(win, 0, mid_x, ACS_TTEE); mvwaddch(win, H-1, mid_x, ACS_BTEE);  

    wattron(win, A_BOLD);
    mvwprintw(win, 1, 2, "INPUT DISPLAY"); mvwprintw(win, 1, mid_x + 2, "DYNAMICS DISPLAY");
    wattroff(win, A_BOLD);

    // Keypad
    int kp_y = (H/2)-6; if(kp_y<3) kp_y=3;
    int kp_x = (mid_x/2)-12; if(kp_x<2) kp_x=2;

    for(int r=0; r<3; r++) {
        for(int c=0; c<3; c++) {
            int y = kp_y + (r*4); int x = kp_x + (c*8);
            char k_char = grid_labels[r][c][0];
            int active = (last_key == k_char);
            if(active) wattron(win, A_REVERSE);
            mvwprintw(win, y, x, "+-----+"); mvwprintw(win, y+1, x, "|  %c  |", k_char); mvwprintw(win, y+2, x, "+-----+");
            if(active) wattroff(win, A_REVERSE);
        }
    }
    mvwprintw(win, H-2, 2, "Press 'p' to close everything");

    // Telemetry
    int dyn_x = mid_x + 4; int dyn_y = (H/2)-5; if(dyn_y<3) dyn_y=3;
    mvwprintw(win, dyn_y++, dyn_x, "force {"); mvwprintw(win, dyn_y++, dyn_x+2, "x: %.2f", fx); mvwprintw(win, dyn_y++, dyn_x+2, "y: %.2f", fy); mvwprintw(win, dyn_y++, dyn_x, "}");
    dyn_y++; mvwprintw(win, dyn_y++, dyn_x, "position {"); mvwprintw(win, dyn_y++, dyn_x+2, "x: %.2f", tx); mvwprintw(win, dyn_y++, dyn_x+2, "y: %.2f", ty); mvwprintw(win, dyn_y++, dyn_x, "}");
    dyn_y++; mvwprintw(win, dyn_y++, dyn_x, "velocity {"); mvwprintw(win, dyn_y++, dyn_x+2, "x: %.2f", tvx); mvwprintw(win, dyn_y++, dyn_x+2, "y: %.2f", tvy); mvwprintw(win, dyn_y++, dyn_x, "}");

    wrefresh(win);
}

int main(int argc, char *argv[]) {

    // This now does Registering AND Listening
    setup_watchdog("CONTROL");
    
    if(argc<3) return 1;
    int fd_out = atoi(argv[1]);
    int fd_in  = atoi(argv[2]);

    setlocale(LC_ALL, "");
    initscr(); cbreak(); noecho(); curs_set(0); nodelay(stdscr, TRUE); 
    WINDOW *win = newwin(0,0,0,0);
    Message msg; int running=1; char last_key=0;

    // --- FIX: Force initial paint ---
    refresh();
    layout_and_draw(win, 0);
    // -------------------------------
    


    while(running) {
        fd_set fds; struct timeval tv={0,0};
        FD_ZERO(&fds); FD_SET(fd_in, &fds);
        if(select(fd_in+1,&fds,NULL,NULL,&tv)>0) {
            while(read(fd_in,&msg,sizeof(Message))>0) {
                if(msg.type==MSG_DRONE_POS) { tx=msg.x; ty=msg.y; tvx=msg.vx; tvy=msg.vy; }
                if(msg.type==MSG_STOP) running=0;
                FD_ZERO(&fds); FD_SET(fd_in, &fds);
                if(select(fd_in+1,&fds,NULL,NULL,&tv)<=0) break;
            }
        }

        int ch = getch();
        if(ch==KEY_RESIZE) { 
            resize_term(0,0); layout_and_draw(win, last_key); 
        }
        else if(ch!=ERR) {
            last_key = (char)tolower(ch);
            if(last_key=='z') fy-=FORCE_STEP;
            if(last_key=='x') fy+=FORCE_STEP;
            if(last_key=='q') fx-=FORCE_STEP;
            if(last_key=='d') fx+=FORCE_STEP;
            if(last_key=='a') {fx-=FORCE_STEP; fy-=FORCE_STEP;}
            if(last_key=='e') {fx+=FORCE_STEP; fy-=FORCE_STEP;}
            if(last_key=='w') {fx-=FORCE_STEP; fy+=FORCE_STEP;}
            if(last_key=='c') {fx+=FORCE_STEP; fy+=FORCE_STEP;}
            if(last_key=='s') {fx=0; fy=0;} 
            if(last_key=='p') running=0;

            msg.type = (running)? MSG_INPUT : MSG_STOP;
            msg.fx=fx; msg.fy=fy; msg.key=last_key;
            write(fd_out, &msg, sizeof(Message));
            layout_and_draw(win, last_key);
        } else {
            msg.type = MSG_INPUT; msg.fx=fx; msg.fy=fy;
            write(fd_out, &msg, sizeof(Message));
        }
        
        // Ensure continuous redraw even without input
        static int frame=0;
        if(frame++ % 5 == 0) layout_and_draw(win, last_key);
        
        usleep(50000);
    }
    endwin(); return 0;
}
