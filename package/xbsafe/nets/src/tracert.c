//---------------------------------------------------------------------------
//gcc -o tracert tracert.c -Wall
//路由跟踪的原理：
//路由跟踪的实现就是巧妙地利用了ICMP报文的TTL超时报文。其实现过程如下：源主机先向目的主机发送
//一个回应请求报文（类型8），TTL值设为1，第一个路由器收到后将TTL减1，这样TTL变为0，分组被废除，
//同时路由器向源主机发送一个TTL超时报文（类型为11），报文的IP包头中的源IP地址就是第一个路由器
//的地址，源主机就可以通过对该报文进行分析，得到第一个路由器的地址。接着发送TTL等于2的报文得到
//第二个路由器地址，再发TTL等于3的报文，如此下去直到收到目的主机的回应应答报文（类型为0）或目
//的不可达报文（类型为3），或者到了最大跳数(要检测路由器个数的最大值)

#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include <wchar.h>
#include "tracert.h"
//---------------------------------------------------------------------------

typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef unsigned char byte;
#define ICMP_ECHO       8                 //发送Ping请求时的ICMP报文类型
#define ICMP_ECHOREPLY  0                 //接收Ping回复时的ICMP报文类型
#define ICMP_TIMEOUT    11                //ICMP超时报文类型
#define ICMP_MIN        8                 //Minimum 8-byte ICMP packet (header)
#define MAX_PACKET      512              //Max ICMP packet size
#define DEICMP_PACKSIZE 60               //Defaut ICMP PACKET SIZE

struct timeval   cStartTickCount;         //用来存放发送包的起始时间
struct timeval  cEndTickCount;

static int      icmpcount = 1;
//ICMP头部定义，被封装在IP包中
typedef struct _icmphdr{
	byte   i_type;                        //报文类型
	byte   i_code;                        //代码
	USHORT i_cksum;                       //校验和
	USHORT i_id;                          //标识符
	USHORT i_seq;                         //序号	
}IcmpHeader;

//初始化ICMP头部
void FillICMPData(char *icmpData,int dataSize)
{
	IcmpHeader *icmp_hdr = NULL;
	char *dataPart = NULL;
	icmp_hdr = (IcmpHeader *)icmpData;
	icmp_hdr->i_type = ICMP_ECHO;
	icmp_hdr->i_code = 0;
	icmp_hdr->i_id = getpid();
	icmp_hdr->i_cksum = 0;
	icmp_hdr->i_seq = 0;
	dataPart = icmpData + sizeof(IcmpHeader);
	memset(dataPart,'E',dataSize - sizeof(IcmpHeader));
}

//校验和函数
USHORT checksum(USHORT *buffer,int size)
{
	unsigned long cksum=0;
	while(size>1)
	{
		cksum+=*buffer++;
		size-=sizeof(USHORT);
	}
	if(size)
		cksum+=*(UCHAR *)buffer;
	cksum=(cksum>>16)+(cksum & 0xffff);
	cksum+=(cksum>>16);
	return (USHORT)(~cksum);;
}

/*------------------------------------------------------------------------
 * deltaT - 计算两个时间差值，得一double型的微秒值（ms）
 *------------------------------------------------------------------------
 */
double deltaT(struct timeval *t1p, struct timeval *t2p)
{
	register double dt;

	dt = (double)(t2p->tv_sec - t1p->tv_sec) * 1000.0 +
	     (double)(t2p->tv_usec - t1p->tv_usec) / 1000.0;
	return (dt);
}

int DecodeIPHeader(char *buf,int bytes,struct sockaddr_in *from,tracert_result *result,int *index)
{
	IcmpHeader      *icmphdr=NULL;
	unsigned short  iphdrlen;
	
	//判断接收操作是否超时
	if(!buf)
	{
		printf("%2d:        *	        Request timed out.\n",icmpcount++);
		result[*index].time = 0;
		stpcpy(result[*index].dstIp,"*");
		*index = *index + 1;
		return 3;
	}
	
	iphdrlen=(buf[0] & 0x0f)*4;
	icmphdr=(IcmpHeader *)(buf+iphdrlen);
	if(bytes<iphdrlen+ICMP_MIN)
	{
		printf("Too few bytes from %s\n",inet_ntoa(from->sin_addr));
		return 2;
	}
	//判断接收的ICMP报文是否为超时报文
	if(icmphdr->i_type==ICMP_TIMEOUT&&icmphdr->i_code==0)
	{
		result[*index].time = deltaT(&cStartTickCount,&cEndTickCount);
		stpcpy(result[*index].dstIp,inet_ntoa(from->sin_addr));
		printf("%2d:        %-15s       %.1fms\n",icmpcount++,result[*index].dstIp,result[*index].time);	
		*index = *index + 1;
		return 0;
	}
	//判断接收的ICMP报文是否为回复报文
	else if(icmphdr->i_type==ICMP_ECHOREPLY&&icmphdr->i_id==getpid())
	{
		result[*index].time = deltaT(&cStartTickCount,&cEndTickCount);
		stpcpy(result[*index].dstIp,inet_ntoa(from->sin_addr));
		printf("%2d:        %-15s       %.1fms\n",icmpcount++,result[*index].dstIp,result[*index].time);	
		*index = *index + 1;
		printf("Trace complete!\n");
		return 1;
	}
	//其他类型,表示不可达
	else
	{
		printf("%2d:        Destination host is unreachable!\n",icmpcount++);
		return 4;
	}
}

int tracert(const char *DestHost,tracert_result* pIpReasult,int *number,int *brun)
{
	int sockRaw = -1;
	struct sockaddr_in dest;
	struct sockaddr_in from;
	int i;
	int bread;
	socklen_t fromlen = sizeof(from);
	int ret;
	int count = 0;;
	struct hostent *hp = NULL;
	char icmpPack[MAX_PACKET];
	char recvbuf[MAX_PACKET];
	USHORT seq_no = 0;
	struct timeval timeoutRecv = {5, 0};
	struct timeval timeoutSend = {1, 0};
	int iret = -1;
	icmpcount = 1;
	
	if(DestHost == NULL || *number == 0) return -1;
	
	printf("Destination:%s,pakage size %d,",DestHost,DEICMP_PACKSIZE);
	
	//创建套接字
	sockRaw = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
	if(sockRaw == -1)
	{
		printf("WSASocket() failed:%s\n",strerror(errno));
		return -1;
	}
	//对锁定套接字设置超时
	bread = setsockopt(sockRaw,SOL_SOCKET,SO_RCVTIMEO,&timeoutRecv,sizeof(timeoutRecv));
	if(bread == -1)
	{
		printf("setsockopt(SO_RCVTIMEO) failed:%s\n",strerror(errno));
		return -1;
	}

	bread = setsockopt(sockRaw,SOL_SOCKET,SO_SNDTIMEO,&timeoutSend,sizeof(timeoutSend));
	if(bread == -1)
	{
		printf("setsockopt(SO_SNDTIMEO) failed:%s\n",strerror(errno));
		return -1;
	}
	//解析目标地址，将主机名转化为IP地址
	memset(&dest,0,sizeof(dest));
	dest.sin_family = AF_INET;
	if((dest.sin_addr.s_addr = inet_addr(DestHost)) == INADDR_NONE)
	{
		if((hp = gethostbyname(DestHost))!=NULL)
		{
			memcpy(&(dest.sin_addr),hp->h_addr_list[0],hp->h_length);
			dest.sin_family = hp->h_addrtype;
			printf("dest.sin_addr=%s\n",inet_ntoa(dest.sin_addr));
		}
		else
		{
			printf("gethostbyname() failed:%s\n",strerror(errno));
			return -1;
		}
	}
	//Create the ICMP pakcet
	memset(recvbuf,0,MAX_PACKET);
	memset(icmpPack,0,MAX_PACKET);
	FillICMPData(icmpPack,DEICMP_PACKSIZE);
	printf("Hop          IP Address           Time elapsed\n");
	//开始发送/接收ICMP报文
	for(i=1;i<=30 && *brun == 1;i++)
	{
		if(count >= *number) break;
		
		int bwrote;
		//设置IP包的生存期
		ret=setsockopt(sockRaw,IPPROTO_IP,IP_TTL,(char *)&i,sizeof(int));
		if(ret == -1)
		{
			printf("setsockopt(IP_TTL) failed:(%d),%s\n",errno,strerror(errno));
			break;
		}
		((IcmpHeader *)icmpPack)->i_cksum = 0;		
		((IcmpHeader *)icmpPack)->i_seq = seq_no++;     //Sequence number of ICMP packets
		((IcmpHeader *)icmpPack)->i_cksum = checksum((USHORT *)icmpPack,DEICMP_PACKSIZE);
		//发送ICMP包请求查询
		
		(void)gettimeofday(&cStartTickCount, NULL);
		bwrote = sendto(sockRaw,icmpPack,DEICMP_PACKSIZE,0,(struct sockaddr *)&dest,sizeof(dest));
		if(bwrote <= 0)
		{
			break;
		}
		
		//接收ICMP回复包
		memset(recvbuf,0,MAX_PACKET);
		bread = recvfrom(sockRaw,recvbuf,MAX_PACKET,0,(struct sockaddr *)&from,&fromlen);
		(void)gettimeofday(&cEndTickCount, NULL);
		if(bread == -1)
		{
			if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
			{
				DecodeIPHeader(NULL,0,&from,pIpReasult,&count);
				continue;
			}
			printf("recvfrom() failed:%d,%s\n",errno,strerror(errno));
			break;
		}
		iret = DecodeIPHeader(recvbuf,bread,&from,pIpReasult,&count);
		if(iret == 1 || iret == 4)
			break;
		iret = -1;
		
		sleep(1);
	}

	if(sockRaw != -1)
		close(sockRaw);
	
	*number = count;

	return iret;
}


int main_tracert()
{
	int i = 0;
	tracert_result IpReasult[30];
	memset(IpReasult,0,sizeof(tracert_result) * 30);
	int num = 30;
	int brun = 1;
	tracert("www.baidu.com",IpReasult,&num,&brun);
	printf("num:%d\n",num);
	for(i =0;i<num; ++i)
	{
		printf("%s\n",IpReasult[i].dstIp);
	}
	
	return 0;
}