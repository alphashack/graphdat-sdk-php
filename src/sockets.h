#ifndef GRAPHDAT_SOCKETS_H
#define GRAPHDAT_SOCKETS_H

int openSocket(char *path, int port);
void closeSocket(int sockfd);
int socketWrite(int sockfd, void* buf, int len);

#endif

