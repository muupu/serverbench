CC=		gcc
CFLAGS=	-Wall -ggdb -W -O

serverbench: serverbench.o 
	$(CC) $(CFLAGS) -o serverbench serverbench.o

serverbench.o:	serverbench.c socket.c 

clean:
	-rm -f *.o serverbench

.PHONY: clean 