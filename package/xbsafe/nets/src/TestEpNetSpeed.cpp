#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <time.h>

#include "TestEpNetSpeed.h"

using namespace  std;

TestEpNetSpeed *pthis = NULL;

TestEpNetSpeed::TestEpNetSpeed():
	m_SendPackage(0),
	m_UdpLostSendPackets(0),
	m_UdpLostRecvPackages(0),
	pinetIaSet(NULL),
	m_TcpPort(0),m_UdpPort(0)
{
	pthis = this;
	m_status = FALSE;
}

TestEpNetSpeed::~TestEpNetSpeed()
{
	if (pinetIaSet != NULL)
	{
		StopConnect();
		delete pinetIaSet;
		pinetIaSet = NULL;
	}
}

BOOL TestEpNetSpeed::Init1(std::string serverHost, int port, EnumFuncTest itest,int *nrun)
{
	printf("%s begin\n",__FUNCTION__);
	m_brun = nrun;
	m_ServerHost = serverHost;
	m_EnumTest = itest;
	if (itest == ENUM_TCP_DELAY)
	{
		m_TcpPort = port;
	}
	else if (itest == ENUM_UDP_LOST || itest == ENUM_UDP_DELAY)
	{
		m_UdpPort = port;
	}
	
	pinetIaSet = new EpNetIA;
	printf("%s,port:%d,host:%s finish\n",__FUNCTION__,port,serverHost.c_str());
	
	return pinetIaSet != NULL;
}

void TestEpNetSpeed::ComputeToResult(const vector<double>& delayTime, LINETESTRESULT &testResult)
{
	if (delayTime.size() <= 0)
	{
		TRACE_TEXT("all pkg lost\n");
		testResult.ndx = 0;
		testResult.nmax = 0;
		testResult.nmin = 0;
		testResult.navg = 0;
		return ; 
	}

	//这种做法可能溢出，当测试次数非常多时； 可是非溢出的算法精度较低
	DWORD sum = 0;
	DWORD squareSum = 0;
	DWORD maxdelay = LINETESTRESULT::INT_LR_MAX_VALUE;
	DWORD mindelay = LINETESTRESULT::INT_LR_VERYLARGE_VALUE; 
	
	for (size_t i = 0; i < delayTime.size(); ++i)
	{
		//求方差和平均数
		sum += delayTime[i];
		squareSum += delayTime[i] * delayTime[i];
		//求最大最小值
		if (maxdelay < delayTime[i])
			maxdelay = delayTime[i];
		if (mindelay > delayTime[i])
		{
			mindelay = delayTime[i];
			if (mindelay < 1)
			{
				mindelay = 0;
			}
		}
	}
	//求方差和平均数 
	double avg = (double)sum / delayTime.size();
	double variance = (double)squareSum / delayTime.size() - avg * avg;
	
	testResult.nmax = maxdelay;
	testResult.nmin = mindelay; 
	testResult.navg = avg;
	testResult.ndx = variance; 
}

std::vector<LINETESTRESULT>& TestEpNetSpeed::GetResult()
{
	m_vecResult.clear();
	
	if(m_status == FALSE) return m_vecResult;
	
	LINETESTRESULT testResult;
	switch (m_EnumTest)
	{
		case ENUM_TCP_DELAY:
			testResult.funcId = ENUM_TCP_DELAY;
			memcpy(testResult.strip,m_ServerHost.c_str(),MAX_PATH - 1);
			testResult.nSend = m_SendPackage;
			testResult.ninterval = m_TcpDelaySendInterval;
			ComputeToResult(m_TcpDelayed, testResult);
			testResult.nLost = (m_SendPackage - m_TcpDelayed.size() > 0) ?
				(m_SendPackage - m_TcpDelayed.size()) : 0;
			break;
		case ENUM_UDP_DELAY:
			testResult.funcId = ENUM_UDP_DELAY;
			memcpy(testResult.strip,m_ServerHost.c_str(),MAX_PATH - 1);
			testResult.nSend = m_SendPackage;
			testResult.ninterval = m_UdpDelaySendInterval;
			ComputeToResult(m_UdpDelayed, testResult);
			testResult.nLost = (m_SendPackage - m_UdpDelayed.size() > 0) ?
				(m_SendPackage - m_UdpDelayed.size()) : 0;
			break;
		case ENUM_UDP_LOST:
			testResult.funcId = ENUM_UDP_LOST;
			memcpy(testResult.strip,m_ServerHost.c_str(),MAX_PATH - 1);
			testResult.nSend = m_SendPackage;
			testResult.ninterval = m_UdpLostSendInterval;
			testResult.nLost = m_SendPackage - m_UdpLostRecvPackages;
			testResult.navg = 0;
			testResult.nmax = 0;
			testResult.nmin = 0;
			testResult.ndx = 0;
			break;
		default:
			m_vecResult.clear();
			break;
	}
	m_vecResult.push_back(testResult);
	return m_vecResult;
}

void TestEpNetSpeed::StopConnect()
{
	if (pinetIaSet == NULL)
	{
		return;
	}
	
	pinetIaSet->SendBye();
	pinetIaSet->Stop();
}

BOOL TestEpNetSpeed::StartConnect()
{
	printf("%s begin\n",__FUNCTION__);
	BOOL bret = FALSE;

	if (m_EnumTest == ENUM_TCP_DELAY)
	{
		if(pinetIaSet->TcpInit(NetSpeedNotify,m_ServerHost.c_str(),m_TcpPort))
		{
			if(pinetIaSet->Start())
			{
				bret = TRUE;
			}
		}			
	}
	else if (m_EnumTest == ENUM_UDP_DELAY || m_EnumTest == ENUM_UDP_LOST)
	{
		if(pinetIaSet->UdpInit(NetSpeedNotify,m_ServerHost.c_str(),m_UdpPort))
		{
			if(pinetIaSet->CreateUdpConn())
			{
				bret = TRUE;
			}
		}
	}
	
	printf("%s finish\n",__FUNCTION__);
	
	return bret;
}

double deltaT(const struct timeval *t1p, const struct timeval *t2p)
{
	register double dt;

	dt = (double)(t2p->tv_sec - t1p->tv_sec) * 1000.0 +
		(double)(t2p->tv_usec - t1p->tv_usec) / 1000.0;
	printf("%.3f\n",dt);
	return (dt);
}

void  NetSpeedNotify(int itype,const NetTestPacketHeader *pPacketHeader,wchar_t *pMsg)
{
	//printf("%s begin\n",__FUNCTION__);
	struct timeval t2;
	(void)gettimeofday(&t2, NULL);
	if(pPacketHeader == NULL)
	{
		pthis->m_bconnect = FALSE;
		return;
	}
	
	if ((int)pPacketHeader->code == SUB_TCP_DELAY)
	{
		pthis->m_TcpDelayed.push_back(deltaT((struct timeval *)pPacketHeader->content,&t2));
	}else if ((int)pPacketHeader->code == SUB_UDP_LOST)
	{
		++pthis->m_UdpLostRecvPackages;	
	}else if ((int)pPacketHeader->code == SUB_UDP_DELAY)
	{
		pthis->m_UdpDelayed.push_back(deltaT((struct timeval *)pPacketHeader->content,&t2));
	}else if ((int)pPacketHeader->code == SUB_TCP_MSG)
	{
		//-----------------------
	}else if ((int)pPacketHeader->code == SUB_UDP_MSG)
	{
		//-----------------------------------
	}
}

BOOL TestEpNetSpeed::TcpPackageDelay(int times, int intervals)
{
	m_status = FALSE;
	printf("%s begin\n",__FUNCTION__);
	if (!pinetIaSet )
	{
		return FALSE;
	}

	if(!StartConnect())
		return FALSE;
	
	printf("%s StartConnect true\n",__FUNCTION__);
	
	m_bconnect = TRUE;

	m_TcpDelaySendPackets = times;
	m_TcpDelaySendInterval = intervals;
	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),m_startTm);
	
	NetTestPacketHeader ntph;
	ntph.type = TCP_PACKET;
	ntph.code = SUB_TCP_DELAY;
	ntph.id = getpid(); 
	UINT i=0;
	for (i=0;i < m_TcpDelaySendPackets && m_bconnect && (*m_brun == 1); ++i)
	{
		printf("%s send:%d\n",__FUNCTION__,i);
		gettimeofday((struct timeval *)ntph.content, NULL);
		ntph.sendstamp = (long)time(NULL) * 1000;
		pinetIaSet->SendPacket(TCP_PACKET, ntph);
		usleep(m_TcpDelaySendInterval * 1000);
	}
	m_SendPackage = i;
	
	printf("%s finish\n",__FUNCTION__);
	if(*m_brun != 1)
		return FALSE;
	
	m_status = TRUE;
	
	return TRUE;
}

BOOL TestEpNetSpeed::UdpPackageDelay(int times, int intervals)
{
	m_status = FALSE;
	if (!pinetIaSet )
	{
		return FALSE;
	}
	if(!StartConnect())
		return FALSE;
	
	m_bconnect = TRUE;

	m_UdpDelaySendPackets =times; 
	m_UdpDelaySendInterval = intervals;
	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),m_startTm);
	
	NetTestPacketHeader ntph;
	ntph.type = UDP_PACKET;
	ntph.code = SUB_UDP_DELAY;
	ntph.id = getpid(); 

	TRACE_TEXT("test udpdelay,Udp data pkg sending\n");
	
	UINT i=0;
	for (i=0;i < m_UdpDelaySendPackets && m_bconnect && (*m_brun == 1); ++i)
	{
		gettimeofday((struct timeval *)ntph.content, NULL);
		ntph.sendstamp = (long)time(NULL) * 1000;
		pinetIaSet->SendPacket(UDP_PACKET,ntph);
	
		usleep(m_UdpDelaySendInterval * 1000);
	}
	usleep(1000);
	m_SendPackage = i;
	if(*m_brun != 1)
		return FALSE;
	m_status = TRUE;
	
	return TRUE;
}

BOOL TestEpNetSpeed::UdpPackageLost(int times,int intervals)
{
	m_status = FALSE;
	if (!pinetIaSet )
	{
		return FALSE;
	}
	
	if(!StartConnect())
		return FALSE;
	
	m_bconnect = TRUE;

	m_UdpLostRecvPackages = 0;
	m_UdpLostSendPackets = times;
	m_UdpLostSendInterval = intervals;
	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),m_startTm);
	
	NetTestPacketHeader ntph;
	ntph.type = UDP_PACKET;
	ntph.code = SUB_UDP_LOST;
	ntph.id = getpid(); 
	UINT i=0;
	for (i=0;i < m_UdpLostSendPackets && m_bconnect && (*m_brun == 1); ++i)
	{
		gettimeofday((struct timeval *)ntph.content, NULL);
		ntph.sendstamp = (long)time(NULL) * 1000;
		pinetIaSet->SendPacket(UDP_PACKET,ntph);
		usleep(m_UdpLostSendInterval * 1000);
	}
	usleep(1000);
	m_SendPackage = i;

	if(*m_brun != 1)
		return FALSE;
	m_status = TRUE;
	
	return TRUE;
}
