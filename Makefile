PLASH_LIB=../plash/lib
CC=musl-gcc
CFLAGS=-static -I$(PLASH_LIB)

all: zvenv

zvenv: zvenv.c
	$(CC) $(CFLAGS) -o zvenv zvenv.c $(PLASH_LIB)/plash.c

clean:
	rm zvenv
