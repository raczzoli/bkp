# Compiler and flags
CC = gcc
CFLAGS = -std=gnu99 -Wall -O2 -Wextra -g
LDFLAGS = -lcrypto -lz#-lpthread -lcrypto -lssl -ljansson

# Target executable
PROG = bkp

# Source files
SRCS = main.c snapshot.c cache.c tree.c file.c restore.c sha1-file.c print-file.c worker.c
OBJS = $(SRCS:.c=.o)

# Default target
all: $(PROG)

# Link object files into the final executable
$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(PROG)


# Phony targets
.PHONY: all clean
