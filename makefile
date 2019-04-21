CC=gcc
CFLAGS=-Wall -Wextra
LDFLAGS=
CFLAGS_DEBUG=-O0 -g -fsanitize=address
CFLAGS_RELEASE=-O3
SRC=cenc.c

ifeq ($(BUILD_TYPE),Release)
	CFLAGS += $(CFLAGS_RELEASE)
else
	CFLAGS += $(CFLAGS_DEBUG)
endif

all: cenc


cenc: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 
