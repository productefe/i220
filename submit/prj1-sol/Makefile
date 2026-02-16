CC = gcc
CFLAGS = -Wall -Wextra -std=c11

OBJS = morse.o main.o file-utils.o
TARGETS = morse-encode morse-decode

all: $(TARGETS)

morse-encode: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

morse-decode: morse-encode
	ln -s -f $< $@

%.o: %.c morse.h file-utils.h inlines.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGETS) tests8 tests16 *~ *.o

.PHONY: all clean
