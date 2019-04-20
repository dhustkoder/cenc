CC=gcc
CFLAGS=-Wall -Wextra -O3
LDFLAGS=
SRC=cenc.c

all: cenc


cenc: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 
