CC=		gcc
CFLAGS=	-Wall -ggdb -W -O

serverbench: serverbench.o socket.o
	$(CC) $(CFLAGS) -o serverbench serverbench.o socket.o

serverbench.o:	serverbench.c  socket.h

socket.o: socket.c socket.h

clean:
	-rm -f *.o serverbench

.PHONY: clean 