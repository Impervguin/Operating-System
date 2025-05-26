CC = gcc

CFLAGS = -Wall -Wextra

SRCS = $(wildcard *.c)
EXES = $(SRCS:.c=.out)

all: $(EXES)

%.out: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXES)

.PHONY: all clean