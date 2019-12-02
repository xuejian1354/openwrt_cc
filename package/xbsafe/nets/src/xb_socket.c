
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>

#include "xb_socket.h"

int tcp_set_timout(int sock, int second)
{
	struct timeval send = {second,0};
	struct timeval recv = {second,0};
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&send, sizeof(struct timeval));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&recv, sizeof(struct timeval));
}


int SocketSelect(int socket,int bread, int bwrite, int timout )
{
  int iret = 0;
  struct timeval tv;  // 设置连接超时时间
  fd_set fdsRead;
  fd_set fdsWrite;

  tv.tv_sec = timout; // 秒数
  tv.tv_usec = 0; // 毫秒
  
INT_LOOP: 
  
  if (bread)
  {
    FD_ZERO(&fdsRead);
    FD_SET(socket,&fdsRead);
  }
  if (bwrite)
  {
    FD_ZERO(&fdsWrite);
    FD_SET(socket,&fdsWrite);
  }

  fd_set fdserr;
  FD_ZERO(&fdserr);
  FD_SET(socket,&fdserr);
  int n = select(socket + 1, bread ? &fdsRead : NULL, bwrite? &fdsWrite : NULL, &fdserr ,&tv);
  
  if (n > 0)
  {
    if (bread && FD_ISSET(socket,&fdsRead))
    {
      FD_CLR(socket,&fdsRead);
      iret = 1;
    }
    if (bwrite && FD_ISSET(socket,&fdsWrite))
    {
      FD_CLR(socket,&fdsWrite);
      iret += 1;
    }
    if(FD_ISSET(socket,&fdserr))
    {
      FD_CLR(socket,&fdserr);
      iret = -1;
      printf("%s:%d fdserr exist\n",__FUNCTION__,__LINE__);
    }
  }
  else if(n == 0)
  {
    iret = 0;
    printf("%s:%d timeout\n",__FUNCTION__,__LINE__);
  }
  else
  {   
       if(errno == EINTR){
          goto INT_LOOP;
       }else
            iret = -1;
    printf("%s:%d erro,%s\n",__FUNCTION__,__LINE__,strerror(errno));
  }
  return iret;
  
}


int SocketSelect_Wakeup(int socket, int wakeup_socket, int timout)
{
  int iret = 0;
  struct timeval tv;  // 设置连接超时时间
  fd_set fdsRead;
  int max_fd;

  tv.tv_sec = timout; // 秒数
  tv.tv_usec = 0; // 毫秒
  
INT_LOOP: 
  
  FD_ZERO(&fdsRead);
  FD_SET(socket,&fdsRead);

  if(wakeup_socket >0 ){
    FD_SET(wakeup_socket,&fdsRead);
  }

  max_fd = socket > wakeup_socket ? socket: wakeup_socket;

  fd_set fdserr;
  FD_ZERO(&fdserr);
  FD_SET(socket,&fdserr);
  int n = select(max_fd + 1, &fdsRead, NULL, &fdserr ,&tv);
  
  if (n > 0)
  {
    if (FD_ISSET(socket,&fdsRead))
    {
      FD_CLR(socket,&fdsRead);
      iret = 1;
    }
    if (wakeup_socket >0 && FD_ISSET(wakeup_socket,&fdsRead))
    {
      FD_CLR(wakeup_socket,&fdsRead);
      iret = -2;
    }

    if(FD_ISSET(socket,&fdserr))
    {
      FD_CLR(socket,&fdserr);
      iret = -1;
      printf("%s:%d fdserr exist\n",__FUNCTION__,__LINE__);
    }
  }
  else if(n == 0)
  {
    iret = 0;
    printf("%s:%d timeout\n",__FUNCTION__,__LINE__);
  }
  else
  {   
       if(errno == EINTR){
          goto INT_LOOP;
       }else
            iret = -1;
    printf("%s:%d erro,%s\n",__FUNCTION__,__LINE__,strerror(errno));
  }
  return iret;
  
}


int create_tcp_client(const char *phost,unsigned short port)
{

  int iret = -1;
  int sock = -1;
  int flag;

  struct sockaddr_in addressOSGI;
  struct timeval timeoutSend = {15, 0};
 
  if(phost == NULL || strlen(phost) == 0) return iret;
  

  addressOSGI = mygethostbyname(phost);

  addressOSGI.sin_port = htons(port);

  //create socket
  if (-1 == (sock = socket(AF_INET, SOCK_STREAM, 0))) {
    fprintf(stderr, "socket() Failed,%s!\n",strerror(errno));
    return iret;
  }
  
  flag = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, (flag & (~O_NONBLOCK)));
    
  //connect server
    if (-1 == connect(sock, (const struct sockaddr *)(&addressOSGI), sizeof(addressOSGI))) {
    fprintf(stderr, "connect() Failed,%s!\n",strerror(errno));
    close(sock);
    return iret;
  }
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeoutSend, sizeof(timeoutSend));

  return sock;
}

int create_tcp_client_with_timeout(const char* phost,int nport, int seconds)
{
  struct sockaddr_in addressOSGI;
  addressOSGI = mygethostbyname(phost);
  addressOSGI.sin_port = htons(nport);

  //create new socket
  int aSocket = -1;
  aSocket = -1;
  aSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(aSocket == -1)
  {   
      printf("Failed to create a new socket, errNo: %s.\n", strerror(errno));
      return -1;
  }

  int flags = 0;
  if((flags = fcntl(aSocket, F_GETFL, 0)) < 0 )
  {
    perror("fcntl 1 false\n");
    close(aSocket);
    return -1;
  }

  if(fcntl(aSocket, F_SETFL, flags | O_NONBLOCK) < 0 )
  {
    perror("fcntl 2 false\n");
    close(aSocket);
    return -1;
  }

    if(connect(aSocket, (struct sockaddr*)&addressOSGI, sizeof(addressOSGI)) != 0)
  {
    if(errno != EINPROGRESS) { // EINPROGRESS 
      printf("Failed to connect socket! errNo: %s.\n", strerror(errno));
            perror("connect false\n");
            close(aSocket);
            return -1;
        }
  }else
  {
    printf("connect return 0! errNo: %s.\n", strerror(errno));
    return aSocket;
  }

  fd_set FDSET;
  FD_ZERO(&FDSET);
  FD_SET(aSocket, &FDSET);
  
          
  struct timeval tv;  
  tv.tv_sec = seconds; 
  tv.tv_usec = 0; 

  int nError = select(aSocket +1, NULL, &FDSET, NULL, &tv);
  if(nError < 0)
  {   
    printf("select failed ret=%d, errNo: %s.\n", nError,strerror(errno)); 
    close(aSocket);
    return -1;
  }else if(nError == 0)
  {
    printf("%s.select timeout.\n",__FUNCTION__);  
    close(aSocket);
    return -1;
  }else if(nError == 2) {
    printf("%s.nError == 2;select timeout.\n",__FUNCTION__);  
    close(aSocket);
    return -1;
  }
  
  if(!FD_ISSET(aSocket, &FDSET))
  {
    printf("%s.socket not writable\n",__FUNCTION__);
    close(aSocket);
    return -1;
  }
  //printf("%s.Connected\n",__FUNCTION__);
  
  flags = fcntl(aSocket, F_GETFL, 0);
  fcntl(aSocket, F_SETFL, flags & ~O_NONBLOCK);
  
  struct timeval send = {seconds,0};
  struct timeval recv = {seconds,0};
  setsockopt(aSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&send, sizeof(struct timeval));
  setsockopt(aSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&recv, sizeof(struct timeval));

  return aSocket;
}


int port_send(struct NetworkPort* port,const char *msg,int len)
{
    if(port->packetFlag == 0)
      return send(port->socket,msg,len,0);
    else
      return tcp_send(port->socket, msg, len);
}

int port_recv(struct NetworkPort* port, char *msg,int len)
{
	char *pbuf = NULL;
	int ret = -1;
	int len2 = 0;
    if(port->packetFlag == 0)
      ret = recv(port->socket,msg,len,0);
    else {
      	ret = tcp_recv(port->socket, &pbuf);
      	if(pbuf != NULL){
			if(ret > 0){
				ret = ret > len ? len : ret;
				memcpy(msg, pbuf, ret);
			}
			free(pbuf);		
		}
	}

	return ret;
}


int tcp_send(int socket,const char *msg,int len)
{
	int isend = 0;
	int isum = 0;
	int tmp = 0;
	int ret = -1;
	if(msg == NULL || len == 0) return 0;
	
	isum = len + 4;
	char *pbuf = (char *)malloc(isum + 1);
	if(pbuf == NULL) return -2;
	
	memset(pbuf, 0, isum + 1);
	SetBE32(pbuf,len);
	memcpy(pbuf + 4,msg,len);
	while(isend < isum)
	{
		tmp = send(socket,pbuf + isend,isum - isend,0);
		if(tmp == -1)
		{
			break;
		}
		isend += tmp;
	}
	if(isend == isum)
	{
		ret = isum;
	}

	free(pbuf);
	return ret;
}

//-1表示连接断开，0表示未收到数据，>0表示接收到的数据长度
int tcp_recv(int socket,char **pbuf)
{
	int packLen = 0;
	char recvLen[4];
	int isum = 0;
    int iret = 0;
	int tmp = 0;
	memset(recvLen,0,4);
	errno = 0;
	if ((tmp = recv(socket, recvLen, 4, 0)) <= 0) 
	{
      fprintf(stderr, "recvlen Failed:%d,%s\n",tmp,strerror(errno));
      return -1;
    }
	packLen = (int)GetBE32(recvLen);
	if(packLen <= 0)                  
	{
		return 0;
	}		
	printf("recvlen=%d\n",packLen);
	*pbuf = (char *)malloc(packLen + 1);
	if(*pbuf == NULL)
	{
		printf("recv packlen:%d",packLen);
		return -1;
	}
	memset(*pbuf,0,packLen + 1);
	isum = 0;
	while(isum < packLen)
	{
		iret = recv(socket,*pbuf + isum, packLen - isum,0);
		if (iret <= 0)
		{
			printf("start_wan_client recv err=%s\n",strerror(errno));	
			break;
		}
		isum += iret;
	}
	if(isum != packLen)
	{
		return -1;
	}
	return isum;
}


struct sockaddr_in mygethostbyname(const char* host)
{
    struct sockaddr_in addr;
    struct addrinfo *answer, hint, *curr;
    char ipstr[16];    
    bzero(&hint, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(host, NULL, &hint, &answer);
    if (ret != 0) {
      fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ret));
        return addr;
    }

    for (curr = answer; curr != NULL; curr = curr->ai_next) {
    inet_ntop(AF_INET, 
    &(((struct sockaddr_in *)(curr->ai_addr))->sin_addr), 
    ipstr, 16);
    printf("%s\n", ipstr);
    addr = *((struct sockaddr_in *)(curr->ai_addr));
    }

    freeaddrinfo(answer);

    return addr;
}

int tcpclient(char* server, int port)
{

    /* Variable and structure definitions. */

    int sd, rc;
    socklen_t length = sizeof(int);

    struct sockaddr_in serveraddr;

    struct hostent* hostp;

    char data[100] = "This is a test string from client lol!!! ";

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {

        perror("Client-socket() error");

        return -1;
    }
    else
        printf("Client-socket() OK\n");

    /*If the server hostname is supplied*/

    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));

    serveraddr.sin_family = AF_INET;

    serveraddr.sin_port = htons(port);

    if ((serveraddr.sin_addr.s_addr = inet_addr(server)) == (unsigned long)INADDR_NONE)
    {

        serveraddr = mygethostbyname(server);

    }

    if ((rc = connect(sd, (struct sockaddr*)&serveraddr, sizeof(serveraddr))) < 0)
    {

        perror("Client-connect() error");

        close(sd);

        return -1;
    }
    else
        printf("Connection established...\n");
    return sd;
}

static void Set8(void* memory, int offset, uint8 v) 
{
  uint8* pint8 = (uint8*)memory;
  pint8[offset] = v;
}
static uint8 Get8(const void* memory, int offset) 
{
  const uint8* pint8 = (const uint8*)memory;
  return pint8[offset];
}

//htonl
void SetBE32(void* memory, uint32 v) 
{
  Set8(memory, 0, (uint8)(v >> 24));
  Set8(memory, 1, (uint8)(v >> 16));
  Set8(memory, 2, (uint8)(v >>  8));
  Set8(memory, 3, (uint8)(v >>  0));
}

//ntohl
uint32 GetBE32(const void* memory) 
{
  return ((uint32)(Get8(memory, 0)) << 24)
       | ((uint32)(Get8(memory, 1)) << 16)
       | ((uint32)(Get8(memory, 2)) <<  8)
       | ((uint32)(Get8(memory, 3)) <<  0);
}

void SetLE32(void* memory, uint32 v) {
  Set8(memory, 3, (uint8)(v >> 24));
  Set8(memory, 2, (uint8)(v >> 16));
  Set8(memory, 1, (uint8)(v >>  8));
  Set8(memory, 0, (uint8)(v >>  0));
}

inline uint32 GetLE32(const void* memory) {
  return ((uint32)(Get8(memory, 3)) << 24)
   | ((uint32)(Get8(memory, 2)) << 16)
   | ((uint32)(Get8(memory, 1)) <<  8)
   | ((uint32)(Get8(memory, 0)) <<  0);
}