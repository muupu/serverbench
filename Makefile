CC=		gcc
CFLAGS=	-Wall -ggdb -W -O

serverbench: serverbench.o socket.o buildrequest.o
	$(CC) $(CFLAGS) -o serverbench serverbench.o socket.o buildrequest.o

serverbench.o:	serverbench.c  socket.h

socket.o: socket.c socket.h

buildrequest.o: buildrequest.c buildrequest.h

clean:
	-rm -f *.o serverbench

.PHONY: clean 