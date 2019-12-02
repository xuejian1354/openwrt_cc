
#include "Common.h"

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h> /* for fork */
#include <sys/syscall.h>
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include <time.h>
#include <errno.h>
#include <error.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <string.h>
#include <arpa/inet.h>

#include "echoClient.h"


int udp_packets_command(int sock, const char* host, int port, char* message, int len)
{
  struct sockaddr_in si_other;
  int i, slen=sizeof(si_other);

  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(port);
  
  if (inet_aton(host , &si_other.sin_addr) == 0) 
  {
    fprintf(stderr, "inet_aton() failed\n");
    return -1;
  }

  do
  {
    //send the message
    if (sendto(sock, message, len , 0 , (struct sockaddr *) &si_other, slen)==-1)
    {
      fprintf(stderr, "sendto() failed %s\n", strerror(h_errno));
      return -1;
    }
    
    //puts(response);
  }while(0);

  return 0;
}

static char upload_result[256];

static DWORD timeGetTime()
{
  struct timeval now;
  gettimeofday(&now, NULL);
  return (now.tv_sec * 1000 + now.tv_usec/1000);
}


static int start_test(const char *server, int ctrlPort, int method, int* totalTime, int* bps, int* pps)

{
  int retval = 0;
  int s, i, ii;

  char buf[1500];

  int num = 100;
  int sleep_time = 1000*1000;
 
      struct sockaddr_in servaddr;
    struct msghdr msg;
    struct iovec msg1[10], msg2;

    memset(msg1, 0, sizeof(msg1));

    for(i=0; i< 10; i++){
        msg1[i].iov_base = buf;
        msg1[i].iov_len = 1460;
    }

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = msg1;
    msg.msg_iovlen = 10;

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(server);
    servaddr.sin_port=htons(ctrlPort);

    msg.msg_name         = &servaddr;
    msg.msg_namelen      = sizeof (struct sockaddr_in);


   int sleep_internal = 2;
   sleep_time = 0;

    /* Loop based on the packet number */

  int ctl_socket = -1;
  if ( (ctl_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
  {
    return -1;
  }

  int transactionID = udp_upload_test_command(ctl_socket, server, ctrlPort, 5);
  printf("udp_upload_test_command transactionID %d\n", transactionID);
  if(transactionID == -1)
    transactionID = udp_upload_test_command(ctl_socket, server, ctrlPort, 5);
  if(transactionID == -1){
    printf("udp_upload_test_command fail received\n");
  }

  buf[0] = 0x39;
  buf[1] = transactionID;

  time_t begin = time(NULL);
  DWORD begin1 = timeGetTime();

  int total=0;
  int packets=0;
  int total_packets = 8900 * 3;

    //for(offset = 0; offset < ; offset += (sizeof(buf) - sizeof(*ip)))
    for(; packets < total_packets; )
    {

      if(method == 1){
        retval = sendto(ctl_socket, buf, 1460 , 0 , (struct sockaddr *) &servaddr, sizeof (struct sockaddr_in));

        if(retval < 0)
          fprintf(stderr, "h_errno = (%s)\n", strerror(errno));

        total += 1460;
        packets++;

      }else{
        //udp_packets_command(ctl_socket, server, ctrlPort, buf, 1460);
        retval = sendmsg(ctl_socket, &msg, 0);
        if(retval < 0)
          fprintf(stderr, "h_errno = (%s)\n", strerror(errno));

        total += 1460*10;
        packets+= 10;
      }
      
    
    if(ii%sleep_internal == 0)
      if(sleep_time != 0)
        usleep(sleep_time);
    
    if(packets == 8900){
      DWORD begin2 = timeGetTime();
      int s = begin2 - begin1;
      if(s == 0)
        s = 1000;
      total_packets = (8900.0/s) * 15000;
      printf("[estimate] time %d,  will total %d\n", s, total_packets);
      if(total_packets > 89 * 100 * 5 * 15)
        total_packets = 89 * 100 * 5 * 15;   //max 500M througput.
    }

   }

   time_t end = time(NULL);
   int seconds = end - begin;
   if(seconds == 0) seconds = 1;

  /* close socket */
   printf("send %dMbps %dpps\n", total/1024/128/seconds, packets/seconds);
   *totalTime = seconds * 1000;
   *bps = total/1024/seconds;
   *pps = packets/seconds;

  usleep(30000);

  int ret = udp_upload_result_command(ctl_socket, server, ctrlPort, transactionID, 5, upload_result,  sizeof(upload_result));
  if(ret == -1){
    ret = udp_upload_result_command(ctl_socket, server, ctrlPort, transactionID, 5, upload_result,  sizeof(upload_result));
  }

  printf("udp_upload_result_command %s \n", upload_result);

  close(ctl_socket);

  return 0;

}

const char* vendor_name_format();

int detect_specific_method()
{
  const char* vendor = vendor_name_format();
  if(strstr(vendor, "huawei") != NULL){  
    return 1;
  }else if(strstr(vendor, "zhongxing") != NULL){  
    return 1;
  }else if(strstr(vendor, "tianyi") != NULL){  
    return 2;
  }else if(strstr(vendor, "youhua") != NULL){  
    return 2;
  }else if(strstr(vendor, "fenghuo") != NULL){  
    return 2;
  }else if(strstr(vendor, "beier") != NULL){  
    return 2;
  }else if(strstr(vendor, "nbeier") != NULL){  
    return 2;
  }else{
    return 2;
  }
}
int StartUdpUpTest(const char* host, int port, int ntimes, int user_bandwith, int testMethod, int* totalTime, int* bps, int* pps)
{
  printf("StartUdpUpTest Params %s, %d, %ds, %dM, %d \n", host, port, ntimes, user_bandwith, testMethod);
  memset(upload_result, '\0', sizeof(upload_result));

  if(testMethod == 0){
    testMethod = detect_specific_method();
  }

  return start_test(host, port,  1, totalTime, bps, pps);
}

char* getUdpUpTestResult(){
  return upload_result;
}
