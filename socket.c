#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int Socket(const char *addr, int port)
{
    int sock;
    unsigned long inaddr;
    struct sockaddr_in sockin;
    struct hostent *hp;
    
    memset(&sockin, 0, sizeof(sockin));
    sockin.sin_family = AF_INET;

    inaddr = inet_addr(addr);
    if (inaddr != INADDR_NONE)
        memcpy(&sockin.sin_addr, &inaddr, sizeof(inaddr));
    else
    {
        hp = gethostbyname(addr);
        if (hp == NULL)
            return -1;
        memcpy(&sockin.sin_addr, hp->h_addr, hp->h_length);
    }
    sockin.sin_port = htons(port);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return sock;
    if (connect(sock, (struct sockaddr *)&sockin, sizeof(sockin)) < 0)
        return -1;
    return sock;
}

