CC = $(CROSS_COMPILE)gcc
CROSS_COMPILE =
CFFLAGS = -Wall -g -Werror
LDFLAGS = -pthread -lrt

all : aesdsocket.o
	$(CC) $(CFFLAGS) -o aesdsocket aesdsocket.o $(LDFLAGS)

aesdsocket.o  : aesdsocket.c
	$(CC) $(CFFLAGS) -c aesdsocket.c $(LDFLAGS)

clean :
	rm -f *.o aesdsocket *.elf *.map
