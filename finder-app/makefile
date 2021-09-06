CC = $(CROSS_COMPILE)gcc

CFLAGS = -Wall -Werror

all: writerObj writerC

writerObj: writer.o
	$(CC) $(CFLAGS) -o writer writer.o
	 
writerC: writer.c
	$(CC) $(CFLAGS) -c writer.c

clean: 
	-rm -f *.o $(TARGET) *.elf *.map
