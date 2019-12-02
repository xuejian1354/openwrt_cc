#ifndef TESTEPNETSPEED_H
#define TESTEPNETSPEED_H
#include <string.h>
#include <string>
#include "EpNetIA.h"
#include "Common.h"

using namespace std; 

typedef void (*FgetDubaUUID)(char *md5hdid);

// const static wstring SERVER_HOST = L"58.53.188.30"; //"220.181.85.226";
//const static int TCP_PORT = 62553; 
//const static int UDP_PORT = 62556;
// const static wstring QUERY_SERVER = L"http://58.53.188.29:8090/CompletionReceipt/MaintQuery.action"; 

class TestEpNetSpeed
{
	/*
		对外提供的接口
	*/
public:
	TestEpNetSpeed();
	~TestEpNetSpeed();
	BOOL Init1(std::string serverHost, int port, EnumFuncTest itest,int *nrun);
	BOOL StartConnect();
	BOOL UdpPackageLost(int times, int intervals);
	BOOL TcpPackageDelay(int times, int intervals);
	BOOL UdpPackageDelay(int times, int intervals);

private:
	//求平均值和方差，并放入LINETESTRESULT 结构中
	void ComputeToResult(const vector<double>& delayTime, LINETESTRESULT &testResult);
	void StopConnect();

public:
	DWORD m_SendPackage;
	//udp和tcp的延时及丢包率测试
	DWORD m_UdpLostSendPackets;
	DWORD m_UdpLostSendInterval;
	DWORD m_UdpLostRecvPackages;
	 
	DWORD m_UdpDelaySendPackets;
	DWORD m_UdpDelaySendInterval;
	vector<double> m_UdpDelayed; 

	DWORD m_TcpDelaySendPackets;
	DWORD m_TcpDelaySendInterval;
	vector<double> m_TcpDelayed;
private:
	//程序基本的私有属性	
	EpNetIA *pinetIaSet; 
	EnumFuncTest m_EnumTest;
	std::string m_ServerHost; 
	int m_TcpPort;
	int m_UdpPort;
	int *m_brun;
public:	
	BOOL m_status;
	BOOL m_bconnect;
	std::string m_startTm;
	std::vector<LINETESTRESULT> m_vecResult;
	const std::string& GetStartTime(){ return m_startTm; }
	std::vector<LINETESTRESULT>& GetResult();
};
void  NetSpeedNotify(int itype,const NetTestPacketHeader *pPacketHeader,wchar_t *pMsg);

#endif