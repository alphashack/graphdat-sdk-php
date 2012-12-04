#include <sys/socket.h>
//#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include "sockets.h"
#include "zend.h"

// path is used in unix
// port is used in windows
int openSocket(char *path, int port, int debug)
{
	int         sockfd;
	int         servlen;
	struct sockaddr_un serv_addr;
	int         result;

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	if(sockfd == -1)
	{
        if(debug)
        {
            zend_error(E_NOTICE, "Graphdat :: Client could create a socket - error(%d): %s\n", errno, strerror(errno));
        }
		return -1;
	}
    
    bzero((char *) &serv_addr, sizeof(struct sockaddr_un));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, path);
    servlen = SUN_LEN(&serv_addr);
//	servlen = sizeof(serv_addr.sun_family) + strlen(path) + 1;

	result = connect(sockfd, (struct sockaddr*)&serv_addr, servlen);

	if(result == -1)
	{
        if(debug)
        {
            zend_error(E_NOTICE, "Graphdat :: Client could not connect to path `%s` - error(%d): %s\n", path, errno, strerror(errno));
        }
		sockfd = -1;
	}
    else if(debug)
    {
        zend_error(E_NOTICE, "Graphdat :: socket %d opened\n", sockfd);
    }
    
	return sockfd;
}

void closeSocket(int sockfd)
{
	close(sockfd);
    zend_error(E_NOTICE, "Graphdat :: socket %d closed\n", sockfd);
}

int socketWrite(int sockfd, void* buf, int len, int debug)
{
    int sentBytes = 0;
    int bytesToSend = len;
    while (bytesToSend > 0) {
        int tx = send(sockfd, buf + sentBytes, bytesToSend, 0);
        if(tx == -1)
        {
            if(debug)
            {
                zend_error(E_NOTICE, "Graphdat :: Client could send data to socket %d - error(%d): %s\n", sockfd, errno, strerror(errno));
            }
            bytesToSend = 0;
            sentBytes = -1;
        }
        else
        {
            sentBytes += tx;
            bytesToSend -= tx;
        }
    }
    
    return sentBytes;
}
