CC=gcc
CFLAGS=-Wall -Wextra -O0 -g -fsanitize=address
LDFLAGS=
SRC=cenc.c

all: cenc


cenc: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 
