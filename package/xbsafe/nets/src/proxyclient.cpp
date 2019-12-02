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

#include "base64.h"
#include "proxyclient.h"

int connect_w_to(int soc, struct sockaddr_in* addrPtr) { 
  int res, valopt; 
  long arg; 
  fd_set myset; 
  struct timeval tv; 
  socklen_t lon; 

  // Set non-blocking 
  arg = fcntl(soc, F_GETFL, NULL); 
  arg |= O_NONBLOCK; 
  fcntl(soc, F_SETFL, arg); 

  res = connect(soc, (struct sockaddr *)addrPtr, sizeof(struct sockaddr_in)); 

  if (res < 0) { 
     if (errno == EINPROGRESS) { 
        tv.tv_sec = 15; 
        tv.tv_usec = 0; 
        FD_ZERO(&myset); 
        FD_SET(soc, &myset); 
        if (select(soc+1, NULL, &myset, NULL, &tv) > 0) { 
           lon = sizeof(int); 
           getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon); 
           if (valopt) { 
              fprintf(stderr, "Error in connection() %d - %s\n", valopt, strerror(valopt)); 
              return -1; 
           } 
        } 
        else { 
           fprintf(stderr, "Timeout or error() %d - %s\n", valopt, strerror(valopt)); 
           return -1; 
        } 
     } 
     else { 
        fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
        return -1; 
     } 
  } 
  // Set to blocking mode again... 
  arg = fcntl(soc, F_GETFL, NULL); 
  arg &= (~O_NONBLOCK); 
  fcntl(soc, F_SETFL, arg); 
  
  return 0;
}

#define LOCAL_PORT 17998

#define BUFFER_SIZE 1024

char* proxy_request(const char* lanIP, const char* request, int length)
{
	char *response = NULL;

    struct sockaddr_in client_addr;
    bzero(&client_addr,sizeof(client_addr)); 
    client_addr.sin_family = AF_INET;   
    //client_addr.sin_addr.s_addr = htons(INADDR_ANY);
    inet_pton(AF_INET, lanIP, &(client_addr.sin_addr));
    client_addr.sin_port = htons(0); 

    int client_socket = socket(AF_INET,SOCK_STREAM,0);
    if( client_socket < 0)
    {
        printf("Create Socket Failed!\n");
        return NULL;
    }

/*    if( bind(client_socket,(struct sockaddr*)&client_addr,sizeof(client_addr)))
    {
        printf("Client Bind Port Failed!\n"); 
        close(client_socket);
        return NULL;
    }
*/

    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if(inet_aton(lanIP,&server_addr.sin_addr) == 0)
    {
        printf("Server IP Address Error!\n");
        close(client_socket);
        return NULL;
    }
    server_addr.sin_port = htons(LOCAL_PORT);
    socklen_t server_addr_length = sizeof(server_addr);

    //if(connect(client_socket,(struct sockaddr*)&server_addr, server_addr_length) < 0)
    if(connect_w_to(client_socket, &server_addr) < 0)
    {
        printf("Can Not Connect To local port!\n");
        close(client_socket);
        return NULL;
    }
    
  struct timeval sendTimeout = {10,0};
  struct timeval recvTimeout = {10,0};
  setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&sendTimeout, sizeof(struct timeval));
  setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&recvTimeout, sizeof(struct timeval));

    tcp_send(client_socket,request,length);
 
 	  length = tcp_recv(client_socket, &response);

    printf("Recieve :\t %d From Server Finished\n",length);
     
    close(client_socket);
    return response;
}


std::string tryExtractModel(const char* msg, std::string &swVersion)
{
  duk_context *ctx = NULL;
  duk_idx_t indx0;
  long i = 0;
  std::string strResp;
  std::string model;
  std::string decodestr;

  if(msg == NULL || strlen(msg) == 0)
    return model;

  decodestr = msg;
    
  jsonInitialize(&ctx,&indx0);
  
  i = getStringLen(ctx, decodestr.c_str(), "return_Parameter");
  if(i <= 0 || i > 65536)
  {
    jsonDestroy(ctx); 
    return model;
  }
  
  char* pMem = (char*)malloc(i + 128);
  if(pMem == NULL){
    jsonDestroy(ctx); 
    return model;
  }

  getString(ctx, decodestr.c_str(), "return_Parameter", pMem);

  strResp = Base64::decode64(pMem);


  const char* str = getString(ctx, strResp.c_str(), "ProductCLass", pMem); //1.0 gateway
  if(str != NULL && strlen(str) < 64){
    model = str;
  }

  const char* str2 = getString(ctx, strResp.c_str(), "ProductClass", pMem);  //2.0 gateway
  if(str2 != NULL && strlen(str2) < 64){
    model = str2;
  }

  const char* str1 = getString(ctx, strResp.c_str(), "SWVersion", pMem);

  if(str1 != NULL && strlen(str1) < 64){
    swVersion = str1;
  }

  free(pMem);
  jsonDestroy(ctx); 
  return model;
}


std::string  make_model_request(void)
{
  int iret = 0;
  std::string model;
  const char *pdata = NULL;
  duk_size_t datalen = 0;
  duk_context *ctx = NULL;
  duk_idx_t indx0;
  jsonInitialize(&ctx,&indx0);
  putString(ctx, indx0, "CmdType", "GET_DEVICE_INFO_ALL");
  putString(ctx, indx0, "SequenceId", "12345678");
  putString(ctx, indx0, "USER", "telecomadmin");
  putString(ctx, indx0, "PASSWORD", "nE7jA%5m");
  
  duk_json_encode(ctx,indx0);
  
  pdata = duk_get_lstring(ctx,indx0,&datalen);
  model = pdata;
  jsonDestroy(ctx);
  return model;
}

std::string tryGetModel(std::string &swVersion)
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

  std::string modelReturn;
  std::string modelRequest = make_model_request();
  std::string base64Str = Base64::encode64(modelRequest);

  jsonInitialize(&ctx,&indx0);
  putString(ctx, indx0, "RPCMethod", "Post1");
  putInt(ctx,indx0,"ID",0);
  putString(ctx, indx0, "SequenceId", "12345678");
  putString(ctx, indx0, "Plugin_Name", "DeviceInfo");
  putString(ctx, indx0, "Version", "");
  putString(ctx, indx0, "Parameter", base64Str.c_str());
  
  duk_json_encode(ctx,indx0);
  
  pdata = duk_get_lstring(ctx,indx0,&datalen);
  char* response = proxy_request(getLanIP(), pdata, datalen);
  
  if(response != NULL){
    modelReturn = tryExtractModel(response, swVersion);
    free(response);
    printf("local api got %s\n", modelReturn.c_str());
  
  }

  jsonDestroy(ctx);
  return modelReturn;
}
