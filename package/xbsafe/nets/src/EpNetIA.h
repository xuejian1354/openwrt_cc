#ifndef EPNETIA_H
#define EPNETIA_H

#include <string>
#include <vector>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include "PhysicalSocket.h"
#include "Common.h"

#define UDP_PACKET           9
#define TCP_PACKET           8
#define SUB_UDP_DELAY        91
#define SUB_UDP_LOST         92
#define SUB_UDP_MSG          93
#define SUB_TCP_SERVER_CMD   80
#define SUB_TCP_DELAY        81
#define SUB_TCP_LOST         82
#define SUB_TCP_MSG          83

#define PACKET_LIMIT_MSG     1376

struct NetTestPacketHeader
{
	BYTE			mark[6];			//包体标示
	BYTE            type;               // 类型 发送的包类型，udp或tcp包
	BYTE            code;               // 代码 子类型代码 当为83或93时说明携带数据，需要解析content或判断ulDatalen
	UINT          id;                 // 标识 当前进程的id
	UINT          number;             // 序列号
	unsigned long	sendstamp;          // 发送的时间戳
	unsigned long	recvstamp;			// 接收的时间戳
	unsigned long	ulDatalen;			// 表示后面还有没有数据跟随，如果为0表示不携带数据包，content根据code判断是否有数据，如果大于零表示后面有有效数据包跟随，content为描述
	BYTE			content[PACKET_LIMIT_MSG];		// 命令字如code为83，则该字段存储的是交互的命令字，否则只是携带的填充数据（不解析）

	NetTestPacketHeader()
	{
		memset(this,0,sizeof(NetTestPacketHeader));
		mark[0] = 0x01;
		mark[1] = 0x02;
		mark[2] = 0x03;
		mark[3] = 0x04;
		mark[4] = 0x05;
		mark[5] = 0x06;
	}
};

//itype：0 Client与Server的命令交互消息，内容是pMsg，
//从服务器接收到的命令消息，上层进行处理，LoginOK，LogoutOK，OnlineUserList，NetSpeedTestInfo，
//1socke连接服务器成功，2socket连接服务器失败，3交互错误
//10为udp的数据
//11为udp的socket创建成功，12为udp socket错误
typedef void ( *FNetNotify)(int itype,const NetTestPacketHeader *pPacketHeader,wchar_t *pExtraMsg);

class EpNetIA
{
public:
	EpNetIA(void);
	~EpNetIA(void);
public:
	BOOL TcpInit(FNetNotify cbnn,const char *pServer,USHORT tcpPort);
	BOOL UdpInit(FNetNotify cbnn,const char *pServer,USHORT udpPort);


	int Recv();
	BOOL SendBye();
	BOOL Start();
	void Stop();
	int GetSocketState(); //0已经关闭，1正在连接，2已连接

	void UpCallBack(int itype,const NetTestPacketHeader *pPacketHeader = NULL,wchar_t *pMsg = NULL)
	{
		if (m_Cb != NULL)
		{
			m_Cb(itype,pPacketHeader,pMsg);
		}
	}
	BOOL m_bRun;
	int m_tcpRecv;
	int m_udpRecv;
	BOOL CreateUdpConn();
	BOOL SendPacket(int type,NetTestPacketHeader &ntph);
	PhysicalSocket m_pys;
	PhysicalSocket m_pysUdp;
private:
	FNetNotify m_Cb;
	struct sockaddr_in sin;
	struct sockaddr_in udpsin;

	std::string m_serverhost;//服务器ip或者是主机名称。
	USHORT m_uport;//服务器连接的端口号tcp
	USHORT m_udpport;//服务器连接的端口号udp
	std::vector<std::string> m_vecMsg;
	std::string m_currentMsg;
	pthread_t m_RecvThread;
	pthread_t m_UdpRecvThread;
private:
	void CleanUp();
	void CloseSocket(PhysicalSocket &pys){ if (pys.GetState() != PhysicalSocket::CS_CLOSED) pys.Close();  }
};

#endif