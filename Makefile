CC=		gcc
CFLAGS=	-Wall -ggdb -W -O
TMPDIR=/tmp/serverbench

serverbench: serverbench.o Makefile
	$(CC) $(CFLAGS) -o serverbench serverbench.o

serverbench.o:	serverbench.c socket.c Makefile