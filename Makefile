CC=		gcc
CFLAGS=	-Wall -ggdb -W -O

serverbench: serverbench.o socket.o buildrequest.o bench.o
	$(CC) $(CFLAGS) -o serverbench serverbench.o socket.o buildrequest.o

serverbench.o:	serverbench.c serverbench.h socket.h

socket.o: socket.c socket.h

buildrequest.o: buildrequest.c buildrequest.h serverbench.h 

bench.o: bench.c bench.h

clean:
	-rm -f *.o serverbench

.PHONY: clean 