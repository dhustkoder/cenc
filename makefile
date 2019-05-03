CC=gcc
CFLAGS=-Wall -Wextra
LDFLAGS=
CFLAGS_DEBUG=-O0 -g -fsanitize=address
CFLAGS_RELEASE=-O3
SRC=$(wildcard *.c) \
	$(wildcard tiny-AES-c/aes.c)

ifeq ($(BUILD_TYPE),Debug)
	CFLAGS += $(CFLAGS_DEBUG)
else
	CFLAGS += $(CFLAGS_RELEASE)
endif

all: cenc


cenc: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 

