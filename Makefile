PLASH_LIB=../plash/lib
CC=musl-gcc
CFLAGS=-static -I$(PLASH_LIB)

all: cbox

cbox: cbox.c
	$(CC) $(CFLAGS) -o cbox cbox.c $(PLASH_LIB)/plash.c

clean:
	rm cbox
