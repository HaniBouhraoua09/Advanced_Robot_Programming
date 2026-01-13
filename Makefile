CC = gcc
# Assuming your source files are in a folder named 'src_code'. 
# If they are in the current folder, change this to: SRC = .
SRC = src_code

# -I$(SRC) tells the compiler to look for header files in the source folder
CFLAGS = -Wall -g -I$(SRC)
LIBS = -lncurses -lm

# List of all executables to build
TARGETS = master blackboard dynamics obstacles targets map_window control watchdog

# Default rule
all: $(TARGETS)

# 1. Compile logger object (shared by all)
logger.o: $(SRC)/logger.c $(SRC)/logger.h
	$(CC) $(CFLAGS) -c $(SRC)/logger.c

# 2. Build each executable
# Note: They all depend on drone.h, so if you change the header, they all rebuild.

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

watchdog: $(SRC)/watchdog.c $(SRC)/drone.h logger.o
	$(CC) $(CFLAGS) $(SRC)/watchdog.c logger.o -o watchdog $(LIBS)

# 3. Clean up
clean:
	rm -f $(TARGETS) *.o *.log pids.txt
