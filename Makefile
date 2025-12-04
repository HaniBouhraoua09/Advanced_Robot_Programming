CC = gcc
CFLAGS = -Wall -Isrc_code
LIBS = -lncurses -lm
SRC = src_code

all: master blackboard dynamics obstacles targets map_window control

master: $(SRC)/master.c $(SRC)/drone.h
	$(CC) $(CFLAGS) $(SRC)/master.c -o master

blackboard: $(SRC)/blackboard.c $(SRC)/drone.h
	$(CC) $(CFLAGS) $(SRC)/blackboard.c -o blackboard

dynamics: $(SRC)/dynamics.c $(SRC)/drone.h
	$(CC) $(CFLAGS) $(SRC)/dynamics.c -o dynamics $(LIBS)

obstacles: $(SRC)/obstacles.c $(SRC)/drone.h
	$(CC) $(CFLAGS) $(SRC)/obstacles.c -o obstacles $(LIBS)

targets: $(SRC)/targets.c $(SRC)/drone.h
	$(CC) $(CFLAGS) $(SRC)/targets.c -o targets $(LIBS)

map_window: $(SRC)/map_window.c $(SRC)/drone.h
	$(CC) $(CFLAGS) $(SRC)/map_window.c -o map_window $(LIBS)

control: $(SRC)/control.c $(SRC)/drone.h
	$(CC) $(CFLAGS) $(SRC)/control.c -o control $(LIBS)

clean:
	rm -f master blackboard dynamics obstacles targets map_window control
