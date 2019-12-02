#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <string>
#include <error.h>
#include <sys/types.h>
#include <unistd.h>

#include "EpNetIA.h"
EpNetIA::EpNetIA(void):
	m_bRun(FALSE)
	,m_tcpRecv(0)
	,m_udpRecv(0)
	,m_Cb(NULL)
{
}


EpNetIA::~EpNetIA(void)
{
	CleanUp();
}
void EpNetIA::CleanUp()
{
	m_bRun = TRUE;
	m_Cb = NULL;
	m_serverhost.clear();
	m_uport = 0;
	m_udpport = 0;

	CloseSocket(m_pys);
	CloseSocket(m_pysUdp);

	if (m_tcpRecv != 0)
	{
		void *statuscode = NULL;
		pthread_cancel(m_RecvThread);
		pthread_join(m_RecvThread,(void **)(&statuscode));
		m_tcpRecv = 0;
	}
	if (m_udpRecv != 0)
	{
		void *statuscode = NULL;
		pthread_cancel(m_UdpRecvThread);
		pthread_join(m_UdpRecvThread,(void **)(&statuscode));
		m_udpRecv = 0;
	}

	memset(&sin,0,sizeof(struct sockaddr_in));
	memset(&udpsin,0,sizeof(struct sockaddr_in));
}

BOOL EpNetIA::UdpInit(FNetNotify cbnn,const char *pServer,USHORT udpPort)
{
	CleanUp();
	m_Cb = cbnn;
	m_serverhost = pServer;
		
	char szServerip[32] = {0};
	udpsin.sin_family = AF_INET;
	sin.sin_family = AF_INET;

	if(!Dns(m_serverhost.c_str(),szServerip,32))
	{
		return FALSE;
	}
	
	unsigned long addr = inet_addr(szServerip);
	udpsin.sin_addr.s_addr = addr;
	sin.sin_addr.s_addr = addr;
		
	m_udpport = udpPort;
	udpsin.sin_port = htons(m_udpport);

	if(!m_pysUdp.Create(SOCK_DGRAM))
	{
		TRACE_LOG1(_T("Udp init create errno = %s"),strerror(m_pysUdp.GetError()));
		return FALSE;
	}
	return TRUE;
}


BOOL EpNetIA::TcpInit(FNetNotify cbnn,const char *pServer,USHORT tcpPort)
{
	CleanUp();
	m_Cb = cbnn;
	m_serverhost = pServer;

	char szServerip[32] = {0};
	udpsin.sin_family = AF_INET;
	sin.sin_family = AF_INET;
	if(!Dns(m_serverhost.c_str(),szServerip,32))
	{
		return FALSE;
	}
	unsigned long addr = inet_addr(szServerip);
	udpsin.sin_addr.s_addr = addr;
	sin.sin_addr.s_addr = addr;

	m_uport = tcpPort;
	sin.sin_port = htons(m_uport);
	
	if(!m_pys.Create(SOCK_STREAM))
	{
		TRACE_LOG1(_T("Tcp RecvThread Create error = %d"),m_pys.GetError());
		return FALSE;
	}

	if(m_pys.Connect(sin) == -1)
	{
		TRACE_LOG1(_T("Tcp RecvThread Connect error = %d"),m_pys.GetError());
		return FALSE;
	}
	return TRUE;
}

int EpNetIA::Recv()
{
	printf("%s begin\n",__FUNCTION__);
	
	NetTestPacketHeader ntph;
	int iret = 0;
	unsigned long pkgSize = sizeof(NetTestPacketHeader);
	unsigned long isum =0;
	while (isum < pkgSize)
	{
		iret = m_pys.Recv(&ntph + isum, pkgSize - isum);
		if (iret <= 0)
		{
			break;
		}
		isum += iret;
	}
	if(isum == pkgSize)                  
	{
		if((int)ntph.type != TCP_PACKET) return -2;
		
		if (ntph.ulDatalen == 0)
		{
			iret = isum;
			UpCallBack(0,&ntph);
		}
		else
		{
			char *pbuf = new char[ntph.ulDatalen+1];
			if (pbuf == NULL)
			{
				return -2;
			}
			memset(pbuf,0,ntph.ulDatalen);
			isum = 0;
			while(isum < ntph.ulDatalen)
			{
				iret = m_pys.Recv(pbuf + isum, ntph.ulDatalen - isum);
				if (iret <= 0)
				{
					break;
				}
				isum += iret;
			}
			if (isum == ntph.ulDatalen)
			{
				iret = isum;
				std::wstring strRecv;
				c2w(pbuf,strRecv);
				UpCallBack(0,&ntph,(wchar_t *)strRecv.c_str());
			}
			
			delete [] pbuf;
			pbuf = NULL;
		}
	}  
	printf("%s finish\n",__FUNCTION__);
	return iret;
}

BOOL EpNetIA::SendBye()
{
	if (m_pys.GetState() != PhysicalSocket::CS_CONNECTED)
	{
		return FALSE;
	}

	std::string strmsg = "bye";

	NetTestPacketHeader ntph;
	ntph.type = TCP_PACKET;
	ntph.code = SUB_TCP_MSG;
	ntph.id = getpid();
	ntph.number = 0;
	ntph.recvstamp = 0;
	ntph.ulDatalen = 0;
	strncpy((char *)ntph.content,strmsg.c_str(),strmsg.length());

	ntph.sendstamp = (long)time(NULL) * 1000;
	int iret = m_pys.Send(&ntph,sizeof(NetTestPacketHeader));
	if (iret <= -1)
	{
		TRACE_LOG1(_T("Tcp send data erro = %d\n"),m_pys.GetError());
		return FALSE;
	}
	return TRUE;
}

BOOL EpNetIA::SendPacket(int type,NetTestPacketHeader &ntph)
{
	int iret = 0;
	int erro = 0;
	if (type == TCP_PACKET)
	{
		iret = m_pys.Send(&ntph,sizeof(NetTestPacketHeader));
		erro = m_pys.GetError();
	}else if (type == UDP_PACKET)
	{
		iret = m_pysUdp.SendTo(&ntph,sizeof(NetTestPacketHeader),udpsin);
		erro = m_pysUdp.GetError();
	}
	if (iret == -1)
	{
		TRACE_LOG2(_T("%s send erro=%s\n"),type == TCP_PACKET? _T("tcp packet"):_T("udp packet"),strerror(erro));
		return FALSE;
	}
	return TRUE;
}


void *RecvThread(void *lpParam)
{
	printf("%s begin\n",__FUNCTION__);
	
	EpNetIA *pthis = (EpNetIA *)lpParam;
	while( pthis->m_bRun)
	{
		errno = 0;
		int iret = pthis->Recv();
		if (iret == -1 || iret == 0)
		{
			TRACE_LOG2(_T("Tcp RecvThread error = %d,%s\n"),pthis->m_pys.GetError(),strerror(pthis->m_pys.GetError()));
			pthis->UpCallBack(3);
			break;	
		}			
	}
	TRACE_TEXT(_T("Tcp RecvThread exit\n"));
	pthis->m_tcpRecv = 0;
	pthread_exit(0);
	return NULL;
}

BOOL EpNetIA::Start()
{
	printf("%s begin\n",__FUNCTION__);
	
	int result = -1;
	m_tcpRecv = 1;
	result = pthread_create(&m_RecvThread, NULL, RecvThread, this);
	if (0 != result) {
		m_tcpRecv = 0;
		perror("tcp Start pthread_create failed\n");
		return FALSE;
	} 
	printf("tcp %s thread boot\n",__FUNCTION__);
	return TRUE;
}

void EpNetIA::Stop()
{
	m_bRun = FALSE;
	m_pys.Close();
	m_pysUdp.Close();
	CleanUp();
}

int EpNetIA::GetSocketState()
{
	return m_pys.GetState();
}

void *UdpRecvThread(void *lpParam)
{
	EpNetIA *pthis = (EpNetIA *)lpParam;
	sockaddr_in fromaddr;
	memset(&fromaddr,0,sizeof(sockaddr_in));
	NetTestPacketHeader ntph;

	while(pthis->m_bRun) 
	{
		memset(&ntph,0,sizeof(NetTestPacketHeader));
		int recvd = pthis->m_pysUdp.RecvFrom(&ntph,sizeof(NetTestPacketHeader),fromaddr);
		if (recvd <= 0)
		{
			printf("UdpRecvThread recv data pkg errno = %d,%s\n",pthis->m_pysUdp.GetError(),strerror(pthis->m_pysUdp.GetError()));
			pthis->UpCallBack(12);
			break;
		}

		if((int)ntph.type == UDP_PACKET)                  
		{  
			if (ntph.ulDatalen == 0)
			{
				pthis->UpCallBack(10,&ntph);
			}
			else
			{
				char *pbuf = new char[ntph.ulDatalen + 1];
				memset(pbuf,0,ntph.ulDatalen + 1);
				pthis->m_pysUdp.RecvFrom(pbuf, ntph.ulDatalen,fromaddr);
				std::wstring strRecv;
				c2w(pbuf,strRecv);
				delete [] pbuf;
				pthis->UpCallBack(10,&ntph,(wchar_t *)strRecv.c_str());
			}
		}		
	} 
	TRACE_TEXT(_T("Udp RecvThread exit\n"));
	pthis->m_udpRecv = 0;
	pthread_exit(0);
	return NULL;
}

BOOL EpNetIA::CreateUdpConn()
{
	printf("CreateUdpConn begin");
	int result = -1;
	m_udpRecv = 1;
	result = pthread_create(&m_UdpRecvThread, NULL, UdpRecvThread, this);
	if (0 != result) {
		m_udpRecv = 0;
		perror("pthread_create failed\n");
		return FALSE;
	}  
	printf("CreateUdpConn finish");
	return TRUE;
}