# Compiler and flags
CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g
LDFLAGS = -lcrypto #-lpthread -lcrypto -lssl -ljansson

# Target executable
PROG = bkp

# Source files
SRCS = main.c cache.c tree.c sha1.c
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
