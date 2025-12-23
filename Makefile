CC = gcc
# -Isrc_code tells the compiler to look for header files in src_code
CFLAGS = -Wall -g -Isrc_code
LIBS = -lncurses -lm

# Source directory
SRC = src_code

# Targets to build (ADDED watchdog HERE)
TARGETS = master blackboard dynamics obstacles targets map_window control watchdog

all: $(TARGETS)

# 1. Compile logger object (shared by all)
logger.o: $(SRC)/logger.c $(SRC)/logger.h
	$(CC) $(CFLAGS) -c $(SRC)/logger.c

# 2. Build each executable
master: $(SRC)/master.c $(SRC)/drone.h logger.o
	$(CC) $(CFLAGS) $(SRC)/master.c logger.o -o master $(LIBS)

blackboard: $(SRC)/blackboard.c $(SRC)/drone.h logger.o
	$(CC) $(CFLAGS) $(SRC)/blackboard.c logger.o -o blackboard $(LIBS)

dynamics: $(SRC)/dynamics.c $(SRC)/drone.h logger.o
	$(CC) $(CFLAGS) $(SRC)/dynamics.c logger.o -o dynamics $(LIBS)

obstacles: $(SRC)/obstacles.c $(SRC)/drone.h logger.o
	$(CC) $(CFLAGS) $(SRC)/obstacles.c logger.o -o obstacles $(LIBS)

targets: $(SRC)/targets.c $(SRC)/drone.h logger.o
	$(CC) $(CFLAGS) $(SRC)/targets.c logger.o -o targets $(LIBS)

map_window: $(SRC)/map_window.c $(SRC)/drone.h logger.o
	$(CC) $(CFLAGS) $(SRC)/map_window.c logger.o -o map_window $(LIBS)

control: $(SRC)/control.c $(SRC)/drone.h logger.o
	$(CC) $(CFLAGS) $(SRC)/control.c logger.o -o control $(LIBS)

# ADDED THIS RULE
watchdog: $(SRC)/watchdog.c $(SRC)/drone.h logger.o
	$(CC) $(CFLAGS) $(SRC)/watchdog.c logger.o -o watchdog $(LIBS)

# 3. Clean up
clean:
	rm -f $(TARGETS) *.o *.log pids.txt
