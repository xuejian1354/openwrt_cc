#ifndef XB_SOCKET_H_INCLUDED
#define XB_SOCKET_H_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

struct NetworkPort;

int port_send(struct NetworkPort* port,const char *msg,int len);
int port_recv(struct NetworkPort* port, char *msg,int len);

int create_tcp_client_with_timeout(const char* phost,int nport, int seconds);
int create_tcp_client(const char *phost,unsigned short port); //used 
int tcpclient(char* server, int port); // not useful

int tcp_set_timout(int socket, int second);

/**
* socket超时等待
* return 1表示可读或可写，2表示即可读也可以写，-1表示错误，0表示超时
*/
int SocketSelect(int socket,int bread, int bwrite, int timout );

/**
* 通过wakeup_socket，带有强制select退出功能
*/
int SocketSelect_Wakeup(int socket, int wakeup_socket, int timout);

struct sockaddr_in mygethostbyname(const char* host);

/**
* 向指定连接发送消息
* msg要发送的消息
* len发送数据的长度
* return - 0表示输入的参数不合法，-1发送失败，-2分配空间失败，大于0的数据表示发送的个数
*/
int tcp_send(int socket,const char *msg,int len);

/**
* tcp接收器
* socket 接收的连接，pbuf接收buffer的指针，由调用者释放。
* return - -1表示连接断开，0表示未收到数据，>0表示接收到的数据长度
*/
int tcp_recv(int socket,char **pbuf);


#define PLAIN_PORT 0
#define PACK_PORT 1

struct NetworkPort{
	int (*sendback)(struct NetworkPort *port, const char *msg,int len);
	int socket;
	int packetFlag;  //0, raw data,  1, length + raw data.
};



typedef unsigned char uint8;
typedef unsigned int uint32;

/**
* 将输入的整数转换成大端数据
* memory(OUT) - 接收转换后的大端数值
* v(IN) - 要转换成大端的数据
* return - void
*/
void SetBE32(void* memory, uint32 v);

/**
* 获取memory中的大端数据，转换成整数
* return - 返回转换后的值
*/
uint32 GetBE32(const void* memory);

/**
* 将输入的整数转换成小端数据
* memory(OUT) - 接收转换后的小端数值
* v(IN) - 要转换成小端的数据
* return - void
*/
void SetLE32(void* memory, uint32 v) ;

/**
* 获取memory中的小端数据，转换成整数
* return - 返回转换后的值
*/
uint32 GetLE32(const void* memory);


#ifdef __cplusplus
}
#endif
#endif