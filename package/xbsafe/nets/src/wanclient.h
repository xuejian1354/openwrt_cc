#ifndef WANCLIENT_H_INCLUDED
#define WANCLIENT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "xb_socket.h"

#define MAX_LENGTH (256)

//  event sourcing in notify_event(): 1, for reconnect, 2, just after boot stage. 3, heartbeat, 4, calendar task.

#define EVENT_RECONNECT 1
#define EVENT_POLLING 2

#define RECONNECT_REQUEST 1
#define FIRST_POLLING_REQUEST 2
#define HEARTBEART_REQUEST 3
#define CALENDAR_REQUEST 4

typedef struct {
	char host[64];
	int port;
	char registerHost[64];
	int registerPort;
	char reportHost[64];
	int reportPort;		
	char heartBeatHost[64];    //udp heart beat.
	int heartBeatPort;		
	char pollingEventHost[64];
	int pollingEventPort;		
	int pollingAction;     // 0 close, 1 keepalive.
	int pollingTimeOut;    //(0, 3600 * 24 * 30]  
	int HeartbeatInterval;
	char pollingEventToken[128];

	int retry_interval; //
	int policy;     //0, normal connection. 1, force shutdown, 2, redirect new host.
	int current_connect_state;   //0, starting connect, 1, success connected
	int current_connect_socket;  //main thread socket.
	int current_sleep_count;

	int uptime;
	int register_time;
	char channel[32];
	char sw_version[32];
	
}connect_method;

extern connect_method xb_conn;
int loadconfig_file(const char* file_path);

int route_message(struct NetworkPort* port, const char *msg,int len);
void notify_reconnect();
void notify_event(int event, int source);  // 1, reconnect, 2:polling event.
int start_heart_beat_task(char* host, int port, int interval);

int registerself(int sock);
int bootRegisterSelf(int sock);

char* proxy_request(const char* lanIP, const char* request, int length);

/**
* 启动外网连接服务的接收
* prun运行标志
* return - void
*/
void start_wanclient_recv(struct NetworkPort* port, int wakeup_socket, int *prun);
void start_wanclient_recv_polling(struct NetworkPort* port, int wakeup_socket, int *prun, char* pollingMsg);
int start_local_receive(int port, int *prun, int* socketReturn);

/**
* 向指云平台发送消息
* msg要发送的消息
* len发送数据的长度
* return - 0表示输入的参数不合法，-1发送失败，-2分配空间失败，大于0的数据表示发送的个数
*/
int send_wan_client(int socket, const char *msg,int len);

/**
* 关闭外网连接
* return - 0表示成功，-1表示失败
*/
int close_wan_client();

void retrieveSysConfigure(void);

const char * getLanIP();
const char * getWanIP();
const char * getModel();
const char * getGatewaySWVersion();
const char * getGatewayHWVersion();
const char * getGatewayProvince();
const char * getManufactor(void);
const char * getPluginName(void);

/**
* 获取mac
* return - 返回网关的mac地址，12个16进制的字符
*/
const char * getMac();

/**
* 获取account
* return - 返回网关的用户账号，12个16进制的字符
*/
const char * getAccount();

/**
* 获取上次访问服务器地址
* return - 返回重定向地址
*/
int getLatestAccessAddr(char *addr);

/**
* 获取随机地址
* return - 返回地址
*/
int getRandomAddr(char *addr);

/**
* 等待网络连通性
*/
void waitNetLink(const char *host);


int macNumToStr(char *macAddr, char *str);
int macStrAutoToMacNum(unsigned char *macstr,unsigned char *mac);

#ifdef __cplusplus
}
#endif
#endif
