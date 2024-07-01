CC = gcc
CFLAGS = -std=c11 -Wall -Werror
TARGET = mysync

# List of source files
SRCS = mysync.c options.c utility.c glob2regex.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f mysync