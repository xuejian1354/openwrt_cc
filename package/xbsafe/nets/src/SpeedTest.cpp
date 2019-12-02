
//测速单元
#include <stdio.h>
#include <string.h>
#include <string>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include "XbHttp.h"
#include "SpeedTest.h"
#include "Common.h"
#include "Downloader.h"
//#include "DES.h"
#include "Flow.h"
#include "echoClient.h"

//---------------------------------------------------------------------------
#define QueryParm "?Strategyqueryrequest="
//---------------------------------------------------------------------------


DWORD timeGetTime()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return (now.tv_sec * 1000 + now.tv_usec/1000);
}

TSpeedTest SpeedCls;
static int MThreadStatus = 0;      //线程状态: 0:空闲，1：下载中，2：下载完成，3：下载失败
static int MThreadCtrl = 0;        //下载测速线程控制: 0:停止，1：下载

static int MUpThreadStatus = 0;    //线程状态: 0:空闲，1：下载中，2：下载完成，3：下载失败
static int MUpThreadCtrl = 0;      //下载测速线程控制: 0:停止，1：下载

static int MWebThreadStatus = 0;    //线程状态: 0:空闲，1：下载中，2：下载完成，3：下载失败
static int NThreadCtrl = 0;        //web测速线程控制: 0:停止，1：测速
//---------------------------------------------------------------------------

TSpeedTest::TSpeedTest()
{         
	NTimes = 10;
	Status = 0;
	CurrSpeed = 0.0;
	MaxSpeed = 0.0;
	AvgSpeed = 0.0;

	UDPTime=UDPPPS=UDPBPS=TotalTime=UserBandWidth=WANBPS=0;
	memset(if_policy, 0, sizeof(if_policy));
}
//---------------------------------------------------------------------------

TSpeedTest::~TSpeedTest()
{
}
//---------------------------------------------------------------------------

void TSpeedTest::ComputeToResult(LINETESTRESULT &result)
{
	if (m_speedList.size() <= 0)
	{
		TRACE_TEXT("speed test fail\n");
		result.ndx = 0;
		result.nmax = 0;
		result.nmin = 0;
		result.navg = 0;
		return ; 
	}
	//这种做法可能溢出，当测试次数非常多时； 可是非溢出的算法精度较低
	double sum = 0;
	double squareSum = 0;
	double mindelay = m_speedList[0]; 
	
	for (size_t i = 0; i < m_speedList.size(); ++i)
	{
		//求方差和平均数
		sum += m_speedList[i];
		squareSum += m_speedList[i] * m_speedList[i];
		//求最大最小值
		if (mindelay > m_speedList[i])
		{
			mindelay = m_speedList[i];
		}
	}
	//求方差和平均数 
	double variance = (double)squareSum / m_speedList.size() - AvgSpeed * AvgSpeed;
	
	result.nmax = MaxSpeed;
	result.nmin = mindelay; 
	result.navg = AvgSpeed;
	result.ndx = variance; 
}

std::vector<LINETESTRESULT>& TSpeedTest::GetResult()
{
	float oldSpeed;
	float adjust_offset;

	printf("%s begin,Status:%d\n",__FUNCTION__,Status);
	LINETESTRESULT testResult;
	switch (m_EnumTest)
	{
		case ENUM_SPEED_DOWN:
			testResult.funcId = ENUM_SPEED_DOWN;
			memcpy(testResult.strip,UInfo.Site[0].url,MAX_PATH - 1);
			testResult.nSend = 0;
			testResult.ninterval = 0;
			ComputeToResult(testResult);
			testResult.nLost = 0;
			testResult.nvalCode = Status;
			m_vecResult.push_back(testResult);
			
			break;
		case ENUM_SPEED_1000M_DOWN:
		case ENUM_SPEED_ROUTER_DOWN:
			testResult.funcId = m_EnumTest;
			memcpy(testResult.strip,UInfo.Site[0].url,MAX_PATH - 1);
			testResult.nSend = TotalSendPackets;
			testResult.ninterval = 0;
			ComputeToResult(testResult);
			testResult.nUDPTime = UDPTime;
			oldSpeed = testResult.navg;
			if(testResult.navg > 930 * 128 && UDPTime > 10000){
				float realSpeed = testResult.navg * 10000.0/UDPTime;
				if(realSpeed < 850 * 128)
					testResult.navg =  realSpeed + 70;
				else if(realSpeed < 870 * 128)
					testResult.navg =  realSpeed + 47;
				else if(realSpeed < 890 * 128)
					testResult.navg =  realSpeed + 33;
				else
					testResult.navg = realSpeed;
			}

			adjust_offset = testResult.navg - oldSpeed;

			if(testResult.nmin > testResult.navg){
				testResult.nmin = testResult.nmin + adjust_offset;
				if(testResult.nmin < 0)
					testResult.nmin = 0.0;
			}

			if(testResult.nmax < testResult.navg){
				testResult.nmax = testResult.nmax + adjust_offset;
			}

			testResult.nUDPPPS = UDPPPS;
			testResult.nUDPBPS = UDPBPS;
			testResult.nTotalTime = TotalTime;

			testResult.nUserBandWidth = UserBandWidth;
			testResult.nWANBPS = WANBPS;
			testResult.nOldAvg = 0;
			if(UserBandWidth > 0 && WANBPS > 0){
				float percent = testResult.navg/(UserBandWidth*128);
				if(UserBandWidth > 50 && percent < 0.9 && WANBPS > testResult.navg){
					testResult.nOldAvg = testResult.navg;
					testResult.navg = WANBPS;
					testResult.nmax = testResult.navg + testResult.navg*0.03;
				}
			}

			strcpy(testResult.if_policy, if_policy);
			testResult.nLost = TotalSendPackets - TotalReceivedPackets;
			testResult.nvalCode = Status;
			m_vecResult.push_back(testResult);
			
			break;
		case ENUM_SPEED_UP:
			testResult.funcId = ENUM_SPEED_UP;
			memcpy(testResult.strip,UInfo.upServer[0].url,MAX_PATH - 1);
			testResult.nSend = 0;
			testResult.ninterval = 0;
			ComputeToResult(testResult);
			testResult.nLost = 0;
			testResult.nvalCode = Status;
			testResult.nmin = testResult.nmin * 1.051;
			testResult.navg = testResult.navg * 1.051;
			testResult.nmax = testResult.nmax * 1.051;			
			m_vecResult.push_back(testResult);
			break;
		case ENUM_SPEED_WEB_SPEED:
			testResult.funcId = ENUM_SPEED_WEB_SPEED;
			
			for(size_t i=0; i<m_webspeedList.size(); ++i )
			{	
				//change order to origional sequence.
				int new_order = 0;
				for(size_t j=0; j<m_webspeedList.size(); ++j){
					if(m_webspeedList[j].Index == i){
						new_order = j;
					}
				}

				printf("ENUM_SPEED_WEB_SPEED %d, result seq %d\n", i, new_order);
				int len = MAX_PATH;
				w2c(PageUrl.at(m_webspeedList[new_order].Index).c_str(),testResult.strip,&len);
				testResult.WebStatus = m_webspeedList[new_order].Status;
				testResult.ResponceTime = m_webspeedList[new_order].ResponceTime;
				testResult.WebPageSize = m_webspeedList[new_order].WebPageSize;
				testResult.LoadTime = m_webspeedList[new_order].LoadTime;
				testResult.DNSTime = m_webspeedList[new_order].DNSTime;
				testResult.FstPkgTime = m_webspeedList[new_order].FstPkgTime;
				m_vecResult.push_back(testResult);
			}
			
			break;
		default:
			m_vecResult.clear();
			break;
	}
	
	return m_vecResult;
}
//---------------------------------------------------------------------------
/*
void TSpeedTest::GetDownSpeedTestStrategy(const std::wstring &strStrategy)
{
	m_EnumTest = ENUM_SPEED_DOWN;
	TRACE_TEXT(_T("GetDownSpeedTestStrategy begin\n"));
	memset(&UInfo,0,sizeof(TCOMMINFO));	
	//TRACE_TEXT(_T("CRMUpWidth finish"));
    std::wstring wstr1 = GetXmlNodeValue(strStrategy.c_str(), L"serverlist");   
	std::string strAurl;
	w2c(wstr1.c_str(),strAurl);
	TRACE_LOG3(_T("downServer %s,%d,%d\n"),strAurl.c_str(),strAurl.length(),wstr1.length());
	//TAutoMem Mem1((wstr1.length() + 1)* 2);
	//处理downserver的信息
	if (!wstr1.empty())
	{
		wchar_t *Mem1 = new wchar_t[(wstr1.length() + 10)];
		wmemset(Mem1,0,wstr1.length() + 10);
		//TRACE_LOG1(_T("TAutoMem mid,%d\n"),wstr1.length());
		//wchar_t* pw1 = wcsncpy((wchar_t*)Mem1.GetBuffer(),wstr1.c_str(),wstr1.length() + 1);
		wchar_t* pw1 = wcsncpy(Mem1,wstr1.c_str(),wstr1.length() + 1);
		TRACE_LOG2(_T("TAutoMem after,%d,%d\n"),wstr1.length(),wcslen(pw1));
		int iSite = 0;
		while(iSite < 5)
		{
			wstr1 = GetXmlAttributeSValue(pw1, L"url");  
			TRACE_TEXT(_T("GetXmlAttributeSValue after\n"));
			if (wstr1.empty())
			{
				wchar_t *p = wcsstr(pw1, L">");
				if(p == NULL) break;

				p = wcsstr(p, L"<");
				if(p == NULL) break;    
				pw1 = p;
				continue;
			}
			TSITE &site = UInfo.Site[iSite]; 			 
			wcsncpy(site.url, wstr1.c_str(), 99);
			TRACE_TEXT(_T("GetXmlAttributeIValue before\n"));
			site.type = GetXmlAttributeIValue(pw1, L"type");   
			site.port = 0;
			TRACE_TEXT(_T("GetXmlAttributeIValue after\n"));
			
			iSite++;
			wchar_t *p = wcsstr(pw1, L">");
			if(p == NULL) break;

			p = wcsstr(p, L"<");
			if(p == NULL) break;    
			pw1 = p;
		}
		delete [] Mem1;
	}
		
	TRACE_TEXT(_T("down info finish\n"));
}

void TSpeedTest::GetUpSpeedTestStrategy(const std::wstring &strStrategy)
{
	m_EnumTest = ENUM_SPEED_UP;
	TRACE_TEXT(_T("GetUpSpeedTestStrategy begin\n"));
	memset(&UInfo,0,sizeof(TCOMMINFO));	
	std::wstring wUpServer = GetXmlNodeValue(strStrategy.c_str(), L"serverlist");
    
	std::string strAurl;
	w2c(wUpServer.c_str(),strAurl);
	TRACE_LOG1(_T("upServer %s\n"),strAurl.c_str());
	//处理upserver的信息
	if(!wUpServer.empty())
	{		
		wchar_t *Mem1 = new wchar_t[(wUpServer.length() + 10)];
		wmemset(Mem1,0,wUpServer.length() + 10);
		wchar_t* pw1 = wcsncpy(Mem1,wUpServer.c_str(),wUpServer.length() + 1);
		TRACE_LOG2(_T("TAutoMem after,%d,%d\n"),wUpServer.length(),wcslen(pw1));
		
		int iSite = 0;
		while(iSite < 5)
		{
			wUpServer = GetXmlAttributeSValue(pw1, L"url");   
			if (wUpServer.empty())
			{
				wchar_t *p = wcsstr(pw1, L">");
				if(p == NULL) break;

				p = wcsstr(p, L"<");
				if(p == NULL) break;    
				pw1 = p;
				continue;
			}
			TSITE &site = UInfo.upServer[iSite]; 			 
			wcsncpy(site.url, wUpServer.c_str(), 99); 
			site.port =  GetXmlAttributeIValue(pw1, L"port");
			
			iSite++;

			wchar_t *p = wcsstr(pw1, L">");
			if(p == NULL) break;

			p = wcsstr(p, L"<");
			if(p == NULL) break;    
			pw1 = p;
		}
		delete [] Mem1;
	}
	TRACE_TEXT(_T("GetUpSpeedTestStrategy info finish\n"));
}
*/
int TSpeedTest::GetDownSpeedTestStrategy(TestAddrInfo *nAddrInfo)
{
	m_EnumTest = ENUM_SPEED_DOWN;
	memset(&UInfo,0,sizeof(TCOMMINFO));	
	std::string strtmp;
	
	int iSite = 0;
	while(iSite < 5)
	{
		strtmp = nAddrInfo[iSite].addr;
		if (strtmp.empty())
		{
			break;
		}
		TSITE &site = UInfo.Site[iSite]; 			 
		strncpy(site.url, strtmp.c_str(), MAX_PATH - 1);
		MyToLower(strtmp);
		site.type = strtmp.find("ftp://") != std::string::npos ? 2: 1;
				
		++iSite;
	}
	printf("%s,finish ret:%d\n",__FUNCTION__,iSite);
	return iSite;
}

int TSpeedTest::GetUpSpeedTestStrategy(TestAddrInfo *nAddrInfo)
{
	m_EnumTest = ENUM_SPEED_UP;
	memset(&UInfo,0,sizeof(TCOMMINFO));	
	std::string strtmp;
	
	int iSite = 0;
	while(iSite < 5)
	{
		strtmp = nAddrInfo[iSite].addr;
		if (strtmp.empty())
		{
			break;
		}
		TSITE &site = UInfo.upServer[iSite]; 			 
		strncpy(site.url, strtmp.c_str(), MAX_PATH - 1);
		site.port = nAddrInfo[iSite].nport;
				
		++iSite;
	}
	printf("%s,finish ret:%d\n",__FUNCTION__,iSite);
	return iSite;
}
//---------------------------------------------------------------------------
BOOL  TSpeedTest::StartDownload(int ntms,int *brun)
{	
	printf("%s begin\n",__FUNCTION__);
	m_brun = brun;
	CurrSpeed = 0.0;
	MaxSpeed = 0.0;
	AvgSpeed = 0.0;
	TotalSize = 0;
	BOOL bret = TRUE;
	
	m_speedList.clear();
	NTimes = ntms;
	Status = 1;
    MThreadCtrl = 1;
    //DloadCallback = ACallback;
    MThreadStatus = 0;
	DWORD dwBase = 0, dwSize0 = 0, dwSize1 = 0,dwdown = 0;
	int Times = 1;
	TElapsed tp;
	
	if (NTimes <= 0 )
	{
		NTimes = 10;
	}
	
	MThreadStatus = 1;
	//DloadCallback(0, 0, 0, 0, 0, 0);

	TDownloader Dloader;
	Dloader.StartDownload(UInfo.Site, 5);
	
    DWORD dwTick = timeGetTime() + 4000;
    while(MThreadCtrl && *m_brun == 1)
    {
        if(timeGetTime() > dwTick) break;
        usleep(200 * 1000);
    }
	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),m_startTm);
	
    if(Dloader.GetStatus() != 1)
    {
        goto END_Exit;
    }
	
	//DloadCallback(1, 0, 0, 0, 0, 0);
	dwBase = Dloader.GetSize();
	dwSize0 = dwBase;
	tp.Begin();

	while(MThreadCtrl && *m_brun == 1)
    {
		usleep(20 * 1000);

        //下载中
		if( tp.End() > Times * 1000)
        {
            dwSize1 = Dloader.GetSize();
			
            CurrSpeed = (dwSize1 - dwSize0) / 1024.0;
            dwSize0 = dwSize1;

			CurrSpeed > MaxSpeed ? MaxSpeed = CurrSpeed : 0;

			dwdown = dwSize1- dwBase;
            AvgSpeed = dwdown / (Times * 1024.0);
			
            if(Times < (NTimes ))
            {
                //DloadCallback(2, CurrSpeed, MaxSpeed, AvgSpeed, dwdown, (Times ) * 100/NTimes);
                TotalSize = Dloader.GetSize();
				printf("%.f,%.f,%.f, %u\n", CurrSpeed, MaxSpeed, AvgSpeed, TotalSize);
				m_speedList.push_back(CurrSpeed);
				
            }
            else
            {
            	TotalSize = Dloader.GetSize();
				printf("%.f,%.f,%.f, %u\n", CurrSpeed, MaxSpeed, AvgSpeed, TotalSize);
				m_speedList.push_back(CurrSpeed);
                //DloadCallback(3, CurrSpeed, MaxSpeed, AvgSpeed, dwdown, (Times ) * 100/NTimes);
                break;
            }  
			++Times;
        }

		if(Dloader.GetStatus() != 1)
		{ 
			break;
		}
    }
	
	Status = 3;
END_Exit:
	
	if((MThreadCtrl && *m_brun == 1) && Dloader.GetStatus() != 1)
	{   //下载失败
		//DloadCallback(Dloader.GetStatus(), 0, 0, 0, 0, 0);
		Status = 4;
	}else if(MThreadCtrl != 1 || *m_brun != 1)
	{
		Status = 5;
		bret = FALSE;
	}
	

	MThreadStatus = 0;  
    Dloader.StopDownload();
	
	printf("%s finish\n",__FUNCTION__);
	
	return bret;
}

int start_data_block(const char* host, int port, int blocking)
{
    FILE* file;
    char buff[128];
    char str_cmd[256];
    char str_arg[256];
    char action = blocking ? 'I' : 'D';
    
    if(action == 'I'){
    	//use -I prerouting 1, not use -A prerouting
    	sprintf(str_arg, "-%c PREROUTING 1 -t mangle -p udp -s %s/32 --sport %d -j DROP", action, host, port);
    }else{
    	sprintf(str_arg, "-%c PREROUTING -t mangle -p udp -s %s/32 --sport %d -j DROP", action, host, port);
    }

#if defined(GATEWAY_1_0)
    sprintf(str_cmd, "iptables %s", str_arg);
#elif defined(GATEWAY_2_0)
    sprintf(str_cmd, "/files/xtables-multi iptables %s", str_arg);
#else
    error "aaa"
#endif

    file = popen(str_cmd, "r");
    //printf("iptables file %s\n", str_cmd);
    if(file == NULL)
    {
    	printf("start test3\n");
    	return -1;
    }
    while(fgets(buff, sizeof(buff),file)!=NULL){

        //printf("%s", buff);
    }
    pclose(file);
    return 0;
}

static int my_debug = 0;

#undef LOG
#define LOG(fmt, arg...)  if(my_debug == 1) printf(fmt, ##arg)

void enable_speed_test_debug()
{
	my_debug = 1;
}

void disable_speed_test_debug()
{
	my_debug = 0;
}

/*
1M bps = 131072 bytes
1M bps = 128KB
理论发包个数  带宽 * 131072 * 时间 / 真实包长度。
*/

static int compute_packets(unsigned long bandwidth_mbps, int time_seconds, int packetBytes, int realPacketBytes)
{
    return bandwidth_mbps*131072.0*time_seconds/realPacketBytes * (packetBytes *1.0/realPacketBytes);
}

int get_interface_flow_by_procfs_1st();
int get_interface_flow_by_procfs_2nd();
unsigned long if_stats_start_compute(const char* vendor, int count, int seconds, int min_packet_count, char** policy);
const char* vendor_name_format();

BOOL  TSpeedTest::StartUDPDownload(int ntms,int *brun, const char * server, int port, int user_bandWidth, int bandWidth, int PacketSize, int realPacketSize)
{	
	printf("%s begin\n",__FUNCTION__);
	m_EnumTest = ENUM_SPEED_1000M_DOWN;

	m_brun = brun;
	CurrSpeed = 0.0;
	MaxSpeed = 0.0;
	AvgSpeed = 0.0;
	TotalSize = 0;
	TotalSendPackets = 0;
	TotalReceivedPackets = 0;
	BOOL bret = TRUE;
	
	m_speedList.clear();
	NTimes = ntms;
	Status = 1;
    MThreadCtrl = 1;
    //DloadCallback = ACallback;
    MThreadStatus = 0;
	DWORD dwBase = 0, dwSize0 = 0, dwSize1 = 0,dwdown = 0;
	int Times = 1;
	
	if (NTimes <= 0 )
	{
		NTimes = 10;
	}
	
	MThreadStatus = 1;
	//DloadCallback(0, 0, 0, 0, 0, 0);

	NetFlow bgFlow;
	NetFlow packetFlow;
	bgFlow.start();   //let sysfile 

	int ctrlPort = port + 1;
	int transactionID = 0;

	int ctl_socket = -1;
	if ( (ctl_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		return -1;
	}

	DWORD time1 = timeGetTime();

	transactionID = udp_packets_command(ctl_socket, server, ctrlPort);
	if(transactionID == 0)
		transactionID = udp_packets_command(ctl_socket, server, ctrlPort);
	if(transactionID == 0){
		printf("udp command fail received\n");
	}
	DWORD time2 = timeGetTime();
	LOG("command udp rtt passed %dms\n", time2 - time1);

	bgFlow.start();  //reset to start.

	DWORD dwBegin = timeGetTime();

	LOG("command sysfile passed %dms\n", dwBegin - time2);

	int if_count1 = get_interface_flow_by_procfs_1st();
	int if_count2 = 0;
	int if_count2_flag = 0;
	int if_min_packet_count = compute_packets(50, 16, PacketSize, realPacketSize);

	int ret = udp_download_command2(server, port, bandWidth*1024, PacketSize, realPacketSize, 16*1000, 30, transactionID, 15); // trigger udp flush
	if(ret < 0)
		ret = udp_download_command2(server, port, bandWidth*1024, PacketSize, realPacketSize, 16*1000, 30, transactionID, 15); 
	if(ret < 0){
		printf("udp trigger command fail received\n");
	}

	DWORD dwDataComming = timeGetTime();

	DWORD passedTime = dwDataComming - dwBegin;
	LOG("command 1 passed %dms\n", passedTime);

	start_data_block(server, port, 1);

    DWORD dwTick = timeGetTime() + 4000;
    while(MThreadCtrl && *m_brun == 1)
    {
        if(timeGetTime() > dwTick) break;
        usleep(200 * 1000);
    }
	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),m_startTm);
	
	//DloadCallback(1, 0, 0, 0, 0, 0);
	bgFlow.checkpoint();

	DWORD packetBegin = timeGetTime();

	//program seems to be blocking by data flushing
	if(ret >= 0 && (packetBegin - dwDataComming) >= 15*1000) {
		if_count2_flag = 1;
		if_count2 = get_interface_flow_by_procfs_2nd();
	}
	packetFlow.start();

	while(MThreadCtrl && *m_brun == 1)
    {
		usleep(1000 * 1000);

        //下载中
		//if( tp.End() > Times * 1000)
        {
            bgFlow.checkpoint();
            CurrSpeed = bgFlow.getWanBytes() / 1024.0;

			CurrSpeed > MaxSpeed ? MaxSpeed = CurrSpeed : 0;
			
			dwSize1 += bgFlow.getWanBytes();

			dwdown = dwSize1;
            AvgSpeed = dwdown / (Times * 1024.0);
			
            if(Times < (NTimes ))
            {
                //DloadCallback(2, CurrSpeed, MaxSpeed, AvgSpeed, dwdown, (Times ) * 100/NTimes);
                
				LOG("%.f,%.f,%.f, %u\n", CurrSpeed, MaxSpeed, AvgSpeed, bgFlow.getWanBytes());
				m_speedList.push_back(CurrSpeed);
				
            }
            else
            {
            
				LOG("%.f,%.f,%.f, %u\n", CurrSpeed, MaxSpeed, AvgSpeed, dwSize1);
				m_speedList.push_back(CurrSpeed);
                //DloadCallback(3, CurrSpeed, MaxSpeed, AvgSpeed, dwdown, (Times ) * 100/NTimes);
                break;
            }  
			++Times;
        }

    }
	
	Status = 3;

	start_data_block(server, port, 0);

	packetFlow.stop();

	DWORD begin2 = timeGetTime();
	DWORD packetTimes = begin2 - packetBegin;

	if(if_count2_flag == 0) {
		if_count2 = get_interface_flow_by_procfs_2nd();
	}

	int sendPackets = 0;
	for(int i=0; i<3; i++){
		int timeout = 5;
		sendPackets = udp_query_packet_count_command(ctl_socket, server, ctrlPort, transactionID, timeout *(i + 1));
		if(sendPackets >= 0)
			break;
	}
	LOG("got %d packets from server \n", sendPackets);

	DWORD endTime = timeGetTime();
	LOG("command 2 passed %dms\n", endTime - begin2);
	LOG("total test passed %dms\n", endTime - dwBegin);
	
	bgFlow.stop();
	
	int recvPackets = bgFlow.getWanRxPackets();
	int dropedPackets = bgFlow.getWanRxDropedPackets();
	LOG("bgflow rx packets %d, droped packets %d\n", recvPackets, dropedPackets);
	LOG("udp result (C,M,A)%.f,%.f,%.f, %u\n", CurrSpeed, MaxSpeed, AvgSpeed, dwSize1);

	if(packetTimes > 0){
		int pps = packetFlow.getWanRxPackets()/(packetTimes/1000.0);
		LOG("total %d packets %dms, pps %d\n", packetFlow.getWanRxPackets(), packetTimes, pps);

		int bps = packetFlow.getWanBytes()/(packetTimes/1000.0);
		LOG("total %d bytes %dms, bps %d\n", packetFlow.getWanBytes(), packetTimes, bps);
		UDPTime = packetTimes;
		UDPPPS = pps;
		UDPBPS =bps;
		TotalTime = endTime - time1;
	}

	if(if_count1 == if_count2 && if_count1 > 0){
		const char* if_vendor = vendor_name_format();
		char* if_policy1 = NULL;
		unsigned long if_bps = if_stats_start_compute(if_vendor, if_count1, 16, if_min_packet_count, &if_policy1);

		if(if_policy1 != NULL){
			strcpy(if_policy, if_policy1);
		}
		WANBPS = if_bps/1024;
	}
	
	TotalReceivedPackets = recvPackets;
	TotalSendPackets = sendPackets;
	UserBandWidth = user_bandWidth;
	
	close(ctl_socket);
END_Exit:
	
	MThreadStatus = 0;  
	
	printf("%s finish\n",__FUNCTION__);
	
	return bret;
}

BOOL  TSpeedTest::StartRouterUDPDownload(int ntms,int *brun, const char * server, int port, int bandWidth, int PacketSize, int realPacketSize)
{	
	printf("%s begin\n",__FUNCTION__);
	m_EnumTest = ENUM_SPEED_ROUTER_DOWN;

	m_brun = brun;
	CurrSpeed = 0.0;
	MaxSpeed = 0.0;
	AvgSpeed = 0.0;
	TotalSize = 0;
	TotalSendPackets = 0;
	TotalReceivedPackets = 0;
	BOOL bret = TRUE;
	
	m_speedList.clear();
	NTimes = ntms;
	Status = 1;
    MThreadCtrl = 1;
    //DloadCallback = ACallback;
    MThreadStatus = 0;
	DWORD dwBase = 0, dwSize0 = 0, dwSize1 = 0,dwdown = 0;
	int Times = 1;
	
	if (NTimes <= 0 )
	{
		NTimes = 10;
	}
	
	MThreadStatus = 1;
	//DloadCallback(0, 0, 0, 0, 0, 0);

	NetFlow bgFlow;
	NetFlow packetFlow;
	bgFlow.start();   //let sysfile 

	int ctrlPort = port + 1;
	int transactionID = 0;


	DWORD time1 = timeGetTime();

	DWORD time2 = timeGetTime();
	LOG("command udp rtt passed %dms\n", time2 - time1);

	bgFlow.start();  //reset to start.

	DWORD dwBegin = timeGetTime();

	LOG("command sysfile passed %dms\n", dwBegin - time2);

	int if_count1 = get_interface_flow_by_procfs_1st();
	int if_count2 = 0;
	int if_count2_flag = 0;
	int if_min_packet_count = compute_packets(50, 16, PacketSize, realPacketSize);

	DWORD dwDataComming = timeGetTime();

	DWORD passedTime = dwDataComming - dwBegin;
	LOG("command 1 passed %dms\n", passedTime);

    DWORD dwTick = timeGetTime() + 4000;
    while(MThreadCtrl && *m_brun == 1)
    {
        if(timeGetTime() > dwTick) break;
        usleep(200 * 1000);
    }
	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),m_startTm);
	
	//DloadCallback(1, 0, 0, 0, 0, 0);
	bgFlow.checkpoint();

	DWORD packetBegin = timeGetTime();

	//program seems to be blocking by data flushing
	if((packetBegin - dwDataComming) >= 15*1000) {
		if_count2_flag = 1;
		if_count2 = get_interface_flow_by_procfs_2nd();
	}
	packetFlow.start();

	while(MThreadCtrl && *m_brun == 1)
    {
		usleep(1000 * 1000);

        //下载中
		//if( tp.End() > Times * 1000)
        {
            bgFlow.checkpoint();
            CurrSpeed = bgFlow.getWanBytes() / 1024.0;

			CurrSpeed > MaxSpeed ? MaxSpeed = CurrSpeed : 0;
			
			dwSize1 += bgFlow.getWanBytes();

			dwdown = dwSize1;
            AvgSpeed = dwdown / (Times * 1024.0);
			
            if(Times < (NTimes ))
            {
                //DloadCallback(2, CurrSpeed, MaxSpeed, AvgSpeed, dwdown, (Times ) * 100/NTimes);
                
				LOG("%.f,%.f,%.f, %u\n", CurrSpeed, MaxSpeed, AvgSpeed, bgFlow.getWanBytes());
				m_speedList.push_back(CurrSpeed);
				
            }
            else
            {
            
				LOG("%.f,%.f,%.f, %u\n", CurrSpeed, MaxSpeed, AvgSpeed, dwSize1);
				m_speedList.push_back(CurrSpeed);
                //DloadCallback(3, CurrSpeed, MaxSpeed, AvgSpeed, dwdown, (Times ) * 100/NTimes);
                break;
            }  
			++Times;
        }

    }
	
	Status = 3;

	packetFlow.stop();

	DWORD begin2 = timeGetTime();
	DWORD packetTimes = begin2 - packetBegin;

	if(if_count2_flag == 0) {
		if_count2 = get_interface_flow_by_procfs_2nd();
	}

	int sendPackets = 0;

	LOG("got %d packets from server \n", sendPackets);

	DWORD endTime = timeGetTime();
	LOG("command 2 passed %dms\n", endTime - begin2);
	LOG("total test passed %dms\n", endTime - dwBegin);
	
	bgFlow.stop();
	
	int recvPackets = bgFlow.getWanRxPackets();
	int dropedPackets = bgFlow.getWanRxDropedPackets();
	LOG("bgflow rx packets %d, droped packets %d\n", recvPackets, dropedPackets);
	LOG("udp result (C,M,A)%.f,%.f,%.f, %u\n", CurrSpeed, MaxSpeed, AvgSpeed, dwSize1);

	if(packetTimes > 0){
		int pps = packetFlow.getWanRxPackets()/(packetTimes/1000.0);
		LOG("total %d packets %dms, pps %d\n", packetFlow.getWanRxPackets(), packetTimes, pps);

		int bps = packetFlow.getWanBytes()/(packetTimes/1000.0);
		LOG("total %d bytes %dms, bps %d\n", packetFlow.getWanBytes(), packetTimes, bps);
		UDPTime = packetTimes;
		UDPPPS = pps;
		UDPBPS =bps;
		TotalTime = endTime - time1;
	}

	if(if_count1 == if_count2 && if_count1 > 0){
		const char* if_vendor = vendor_name_format();
		char* if_policy1 = NULL;
		unsigned long if_bps = if_stats_start_compute(if_vendor, if_count1, 16, if_min_packet_count, &if_policy1);

		if(if_policy1 != NULL){
			strcpy(if_policy, if_policy1);
		}
		WANBPS = if_bps/1024;
	}
	
	TotalReceivedPackets = recvPackets;
	TotalSendPackets = sendPackets;
	UserBandWidth = bandWidth;
	
END_Exit:
	
	MThreadStatus = 0;  
	
	printf("%s finish\n",__FUNCTION__);
	
	return bret;
}

/**************************************************
	auto speed test.
*/

typedef struct {
    char host[64];
    int port;
    int running;
    int sleep;
    int test;
    int bandWidth;
    int PacketSize;
    int realPacketSize;
    int nTimes;
}connect_method;

typedef struct {
	float tx_speed;
	int tx_packets;
	float rx_speed;
	int rx_packets;
	int rx_time;
}SpeedTrace;

BOOL  AutoStartUDPDownload_Impl(SpeedTrace * trace, int ntms,int *brun, const char * server, int port, int bandWidth, int PacketSize, int realPacketSize)
{	
	LOG("%s begin\n",__FUNCTION__);


	int* m_brun = brun;
	float AvgSpeed = 0.0;
	int TotalSize = 0;
	int TotalSendPackets = 0;
	int TotalReceivedPackets = 0;
	BOOL bret = TRUE;

	
	int NTimes = ntms;
	int Status = 1;
    int MThreadCtrl = 1;
    
    int MThreadStatus = 0;
	DWORD dwBase = 0, dwSize0 = 0, dwSize1 = 0,dwdown = 0;
	int Times = 1;
	
	if (NTimes <= 0 )
	{
		NTimes = 10*1000;
	}
	
	MThreadStatus = 1;

	NetFlow bgFlow;
	NetFlow packetFlow;

	int ctrlPort = port + 1;
	int transactionID = 0;

	int ctl_socket = -1;
	if ( (ctl_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		return -1;
	}

	DWORD time1 = timeGetTime();

	transactionID = udp_packets_command2(ctl_socket, server, ctrlPort, 2);
	if(transactionID == 0)
		transactionID = udp_packets_command2(ctl_socket, server, ctrlPort, 2);
	if(transactionID == 0){
		printf("udp command fail received\n");
	}
	DWORD time2 = timeGetTime();
	LOG("command udp rtt passed %dms\n", time2 - time1);

	bgFlow.start();  //reset to start.

	DWORD dwBegin = timeGetTime();

	DWORD delayMS = dwBegin - time2;

	LOG("command sysfile passed %dms\n", dwBegin - time2);


	int ret = udp_download_command2(server, port, bandWidth, PacketSize, realPacketSize, NTimes, delayMS, transactionID, 2); // trigger udp flush
	if(ret < 0)
		ret = udp_download_command2(server, port, bandWidth, PacketSize, realPacketSize, NTimes, delayMS, transactionID, 2); 
	if(ret < 0){
		printf("udp trigger command fail received\n");
	}

	DWORD FlushAfterTime = timeGetTime();
	LOG("command flush port rtt %dms\n", FlushAfterTime - dwBegin);

	start_data_block(server, port, 1);
	bgFlow.start();  //reset to start.

	LOG("command sysfile passed 2nd %dms\n", timeGetTime() - FlushAfterTime);


    std::string m_startTm;
	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),m_startTm);
	
	DWORD packetBegin = timeGetTime();
	//packetFlow.start();

	DWORD SleepBaseTime = packetBegin;

	DWORD SleepBaseTime2 = SleepBaseTime;

	while(MThreadCtrl && *m_brun == 1)
    {
		usleep(1000 * NTimes);
		SleepBaseTime2 = timeGetTime();
		LOG("sleep %d, ", SleepBaseTime2-SleepBaseTime);
		SleepBaseTime = SleepBaseTime2;
		
        bgFlow.stop();
        AvgSpeed = bgFlow.getWanBytes() / (NTimes *1024.0/1000);
        break;
        
    }
	
	Status = 3;

	start_data_block(server, port, 0);

	//packetFlow.stop();

	DWORD begin2 = timeGetTime();
	DWORD packetTimes = begin2 - packetBegin;

	LOG("bgFlow stop %dms\n", begin2 - SleepBaseTime2);

	
	int sendPackets = 0;
	for(int i=0; i<3; i++){
		int timeout = 5;
		sendPackets = udp_query_packet_count_command(ctl_socket, server, ctrlPort, transactionID, timeout *(i + 1));
		if(sendPackets >= 0)
			break;
	}
	//printf("got %d packets from server \n", sendPackets);

	DWORD endTime = timeGetTime();
	LOG("command 2 passed %dms\n", endTime - begin2);
	LOG("total test passed %dms\n", endTime - dwBegin);
	
	//bgFlow.stop();
	
	int recvPackets = bgFlow.getWanRxPackets();
	int dropedPackets = bgFlow.getWanRxDropedPackets();
	LOG("bgflow rx packets %d, droped packets %d\n", recvPackets, dropedPackets);

	if(packetTimes > 0){
		int pps = bgFlow.getWanRxPackets()/(packetTimes/1000.0);
		LOG("total %d packets %dms, pps %d\n", bgFlow.getWanRxPackets(), packetTimes, pps);

		int bps = bgFlow.getWanBytes()/(packetTimes/1000.0);
		LOG("total %d bytes %dms, bps %d\n", bgFlow.getWanBytes(), packetTimes, bps);
		//UDPTime = packetTimes;
		//UDPPPS = pps;
		//UDPBPS =bps;
		//TotalTime = endTime - dwBegin;
	}

	TotalReceivedPackets = recvPackets;
	TotalSendPackets = sendPackets;

	trace->tx_speed = bandWidth;
	trace->tx_packets = sendPackets;
	trace->rx_speed = AvgSpeed*8.0;
	trace->rx_packets = recvPackets;
	trace->rx_time = endTime - dwBegin;


	close(ctl_socket);
END_Exit:
	
	MThreadStatus = 0;  
	
	LOG("%s finish\n",__FUNCTION__);
	
	return bret;
}


void trace_print_header()
{
	printf("tx_speed  tx_packets  rx_speed rx_packets time\n");
}

void trace_print(SpeedTrace * trace)
{
	printf("%.f %16d  %.f %16d %16d\n", trace->tx_speed, trace->tx_packets, trace->rx_speed, trace->rx_packets, trace->rx_time);
}

/* packet_lost  1/1000 
   > 200,  max
   > 50,  just max
   > 10,   perfect.

   speed_lost  1/100
   	> 1,  perfect.
	> 5,  up to max.
	> 20,   125M, 100M.
*/


int bw_list[] = {10, 50, 100, 200, 300, 500, 750, 1000};
//int bw_list[] = {1, 2, 5, 7, 10, 20, 50, 100, 200, 1000*100};

typedef struct {
	int state;    //0, find max
	int cursor;
	int counter;
	int next_bandwidth;
	int prev_bandwidth;
	int stable_bandwidth;
	int top_bandwidth;
	int low_bandwidth;

	float factor;

}SpeedMatcher;

int get_next_upper_bandwidth(SpeedMatcher * matcher)
{
	int MaxStep = sizeof(bw_list)/sizeof(bw_list[0]);
	matcher->cursor++;
	if(matcher->cursor < MaxStep){
		return bw_list[matcher->cursor] * 1024;
	}else{
		int ret = matcher->next_bandwidth * 1.2;
		return ret;
	}

}


void match_speed_algrithm(SpeedMatcher * matcher, SpeedTrace * trace)
{
	
	int packet_lost = (trace->tx_packets - trace->rx_packets)*1000/trace->tx_packets;
	int speed_lost = (trace->tx_speed - trace->rx_speed)*100/trace->tx_speed;
	LOG("cursor %d, bandwidth %d(rx:%6.2f), packet_lost %d, speed_lost %d\n", matcher->cursor, matcher->next_bandwidth, trace->rx_speed/1024, packet_lost, speed_lost);  

	int current = matcher->next_bandwidth;
	int prev = matcher->prev_bandwidth;

	matcher->counter++;

	switch(matcher->state){
	case 0:
		if(speed_lost <=1){
			matcher->next_bandwidth = get_next_upper_bandwidth(matcher);
			matcher->stable_bandwidth = current;
		}else{
			int band_low = matcher->next_bandwidth *(100 - speed_lost)/100;
			
			if(band_low > matcher->stable_bandwidth)
				matcher->top_bandwidth = (current + band_low)/2;
			else
				matcher->top_bandwidth = (current + matcher->stable_bandwidth)/2;

			/*if(band_low > matcher->stable_bandwidth)
				matcher->top_bandwidth = band_low;
			else
				matcher->top_bandwidth = (current + band_low)/2;
			*/

			matcher->next_bandwidth = (matcher->stable_bandwidth + band_low)/2;
			matcher->state = 2;
		}
		break;
	case 2:                      //have top
		if(speed_lost <=1){
			matcher->stable_bandwidth = current;
			matcher->next_bandwidth = (matcher->stable_bandwidth + matcher->top_bandwidth)/2;
		}else {
			int band_low = matcher->next_bandwidth *(100 - speed_lost)/100;
			
			if(band_low > matcher->stable_bandwidth)
				matcher->top_bandwidth = (current + band_low)/2;
			else
				matcher->top_bandwidth = (current + matcher->stable_bandwidth)/2;

			matcher->next_bandwidth = (matcher->stable_bandwidth + band_low)/2;
			//printf("in stable, %6.2f, top %6.2f\n", matcher->stable_bandwidth, matcher->top_bandwidth );
		}
		break;

	}

	matcher->prev_bandwidth = current;

	LOG("current %6.2f, stable%6.2f, top %6.2f, speed_lost %d\n", current/1024.0,  matcher->stable_bandwidth/1024.0, matcher->top_bandwidth/1024.0, speed_lost);
}


BOOL  TSpeedTest::AutoStartUDPDownload(int ntms,int *brun, const char * server, int port, int bandWidth, int PacketSize, int realPacketSize)
{
	printf("%s begin\n",__FUNCTION__);
	m_EnumTest = ENUM_SPEED_1000M_DOWN;

	m_brun = brun;
	CurrSpeed = 0.0;
	MaxSpeed = 0.0;
	AvgSpeed = 0.0;

	UDPTime = 0;
	UDPPPS = 0;
	UDPBPS =0;

	TotalSize = 0;
	TotalSendPackets = 0;
	TotalReceivedPackets = 0;
	BOOL bret = TRUE;
	
	m_speedList.clear();
	NTimes = ntms;
	Status = 1;
    MThreadCtrl = 1;
    //DloadCallback = ACallback;
    MThreadStatus = 0;
	DWORD dwBase = 0, dwSize0 = 0, dwSize1 = 0,dwdown = 0;
	int Times = 1;

	float TotalSpeed = 0.0;
	int TotalCount = 0;

	SpeedTrace  speedTrace;
	SpeedTrace*  trace;
	SpeedMatcher  matcher;

	matcher.state = matcher.cursor = matcher.next_bandwidth = matcher.prev_bandwidth = 
	  matcher.counter = matcher.stable_bandwidth = matcher.top_bandwidth = matcher.low_bandwidth = 0;
	matcher.factor = 2.5;

	matcher.next_bandwidth = bw_list[0]*1024;
	matcher.top_bandwidth = matcher.next_bandwidth;

	//trace_print_header();
	DWORD dwBegin = timeGetTime();
	
	for(int i = 0; i< 15; i++){
		trace = &speedTrace;

		AutoStartUDPDownload_Impl(trace, 1000, brun, server, port, matcher.next_bandwidth, PacketSize, realPacketSize);

		//trace_print(trace);

		if(trace->tx_packets <= 0 || trace->tx_speed <=0)
			continue;

		match_speed_algrithm(&matcher, trace);

		if(i < 5 )
			continue;

		if(i < 7 && matcher.state != 2)
			continue;

		CurrSpeed = trace->rx_speed/8.0;
		TotalSpeed += CurrSpeed;
		TotalCount++;

		CurrSpeed > MaxSpeed ? MaxSpeed = CurrSpeed : 0;
		TotalSendPackets += trace->tx_packets;
		TotalReceivedPackets +=trace->rx_packets;

		UDPTime += trace->rx_time;
		UDPPPS = TotalCount;
		UDPBPS =0;

		m_speedList.push_back(CurrSpeed);	
	}

	if(TotalCount > 0){
		AvgSpeed = TotalSpeed/TotalCount;
	}

	//TotalReceivedPackets = 0;
	//TotalSendPackets = 0;
	TotalTime = timeGetTime() - dwBegin;
	
	//UDPTime = 0;
	//UDPPPS = 0;
	//UDPBPS =0;
	//TotalTime = 0;

	Status = 3;

	MThreadStatus = 0;  
}


//---------------------------------------------------------------------------

void  TSpeedTest::StopDownload()
{
    MThreadCtrl = 0;
    DWORD dwTick = timeGetTime() + 5000;
    while(timeGetTime() < dwTick)
    {
        if(MThreadStatus == 0) break;
        //Sleep(50);
        usleep(200 * 1000);
    }

    Status = 0;
}


//---------------------------------------------------------------------------
BOOL  TSpeedTest::StartUpLoad(int ntms,int *brun)
{
	TRACE_TEXT(_T("StartUpLoad begin\n"));
	m_brun = brun;
	BOOL bret = TRUE;
	Status = 1;
	m_speedList.clear();
	CurrSpeed = 0.0;
	MaxSpeed = 0.0;
	AvgSpeed = 0.0;
	TotalSize = 0;

	MUpThreadCtrl = 1;
	//ULoadCb = BCallBack;
	MUpThreadStatus = 0;
		
	//已正式开始测速
	DWORD dwBase = 0, dwSize0 = 0, dwSize1 = 0,dwUp = 0;

	int Times = 1;
	NTimes = ntms;
	if(NTimes <= 0)
		NTimes = 10;
	
	TElapsed tp;

	//ULoadCb(0,0,0,0,0,0);
	TDownloader Dloader;
	Dloader.StartUpLoad(UInfo.upServer, 5);

	DWORD dwTick = timeGetTime() + 4000;
	while(MUpThreadCtrl == 1 && *m_brun == 1)
	{
		if(timeGetTime() >= dwTick) break;
		//Sleep(50);
		usleep(200 * 1000);
	}
	
	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),m_startTm);
		
	if(Dloader.GetUpStatus() != 1)
	{
		goto End_exit;
	}

	MUpThreadStatus = 1;
	
	//ULoadCb(1,0,0,0,0, 0);
	dwBase = Dloader.GetUpSize();
	dwSize0 = dwBase;
	
	tp.Begin();
	while(MUpThreadCtrl == 1 && *m_brun == 1)
	{
		//Sleep(20);
		usleep(20 * 1000);
		//下载中
		if( tp.End() > Times * 1000)
		{
			dwSize1 = Dloader.GetUpSize();

			CurrSpeed = (dwSize1 - dwSize0) / 1024.0;
			dwSize0 = dwSize1;

			if(CurrSpeed > MaxSpeed) MaxSpeed = CurrSpeed;

			dwUp = dwSize1- dwBase;
			AvgSpeed = dwUp / (Times * 1024.0);

			if(Times < NTimes)
			{
				//ULoadCb(2,CurrSpeed,MaxSpeed,AvgSpeed,dwUp, Times * 10);
				TotalSize = Dloader.GetUpSize();
				printf("%.f,%.f,%.f, %u\n", CurrSpeed, MaxSpeed, AvgSpeed, TotalSize);
				m_speedList.push_back(CurrSpeed);
			}
			else
			{
				TotalSize = Dloader.GetUpSize();
				printf("%.f,%.f,%.f, %u\n", CurrSpeed, MaxSpeed, AvgSpeed, TotalSize);
				//ULoadCb(3,CurrSpeed,MaxSpeed,AvgSpeed,dwUp, Times * 10);
				m_speedList.push_back(CurrSpeed);
				break;
			}  
			++Times;
		}
	}
	Status = 3;
End_exit:
	if((MUpThreadCtrl == 1 && *m_brun == 1) && Dloader.GetUpStatus() != 1)
	{   //下载失败
		//ULoadCb(Dloader.GetUpStatus(), 0, 0, 0, 0, 0);
		Status = 4;
	}else if(MUpThreadCtrl != 1 || *m_brun != 1)
	{
		Status = 5;
		bret = FALSE;
	}

	MUpThreadStatus = 0;  
	Dloader.StopUpLoad();
	return bret;
}
//---------------------------------------------------------------------------

void  TSpeedTest::StopUpLoad()
{
	MUpThreadCtrl = 0;
	DWORD dwTick = timeGetTime() + 6000;
	while(timeGetTime() < dwTick)
	{
		if(MUpThreadStatus == 0) break;
		//Sleep(50);
		usleep(200 * 1000);
	}

	Status = 0;
}

//---------------------------------------------------------------------------

BOOL  TSpeedTest::StartWebTest(TestAddrInfo *pAddrInfo,int *brun)
{
	printf("%s begin\n",__FUNCTION__);
	
	m_EnumTest = ENUM_SPEED_WEB_SPEED;
	m_brun = brun;
	BOOL bret = TRUE;
	MWebThreadStatus = 0;
	UrlCount = 0;
    Status = 1;   
    for(int i=0; i<5; i++)
    {
		if(strlen(pAddrInfo[i].addr) != 0)
		{
			++UrlCount;
			std::wstring tmp;
			c2w(pAddrInfo[i].addr,tmp);
			PageUrl.push_back(tmp);
		}else
		{
			break;
		}
    }
	printf("%s get url:%s,%d\n",__FUNCTION__,pAddrInfo[0].addr,UrlCount);
	
	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),m_startTm);
	
	printf("%s get time:%s\n",__FUNCTION__,m_startTm.c_str());
	
    NThreadCtrl = 1;
	
	TDownloader webTest;
	webTest.StartWebTest(PageUrl,UrlCount);
	TWEBINFO *pWebInfo = NULL;
	int nFinishCnt = 0;
	
	MWebThreadStatus = 1;
	printf("%s begin test\n",__FUNCTION__);
	
    DWORD dwTick = timeGetTime() + 30000;
    while(NThreadCtrl && *m_brun == 1)
    {
        if(timeGetTime() > dwTick)
        {   //10秒超时
            for(int i=0; i<UrlCount; i++)
            {
				pWebInfo = webTest.GetWebTestItem(i);
                if(pWebInfo->Status != 1 && pWebInfo->Status != 2)
                {
					webTest.StopWebThread(i);
                    pWebInfo->Status = 2;
					printf("%s timeout\n",__FUNCTION__);
					m_webspeedList.push_back(*pWebInfo);
                    //WebCallback(pWebInfo);
                }
            }

            break;
        }

        for(int i=0; i<UrlCount; i++)
        {
			pWebInfo = webTest.GetWebTestItem(i);
            if(pWebInfo->Status == 3)
            {   //完成
				++nFinishCnt;
				pWebInfo->Status = 1;
				m_webspeedList.push_back(*pWebInfo);
				printf("%s finish===\n",__FUNCTION__);
				//WebCallback(pWebInfo);
            }else if(pWebInfo->Status == 4)
			{
				++nFinishCnt;
				pWebInfo->Status = 2;
				m_webspeedList.push_back(*pWebInfo);
				printf("%s failed\n",__FUNCTION__);
				//WebCallback(pWebInfo);
			}
        }

        if(nFinishCnt == UrlCount)
		{
			pWebInfo = webTest.GetWebTestItem(0);
			pWebInfo->AllComplated = 1;
			//m_webspeedList.push_back(*pWebInfo);
			//WebCallback(pWebInfo);
			break; 
		}

        usleep(200 * 1000);
		//printf("%s loop\n",__FUNCTION__);
    }
	
	Status = 3;
	if(*m_brun != 1)
	{
		Status = 5;
		bret = FALSE;
	}
	webTest.StopWebTest();
	MWebThreadStatus = 0;  
	
	printf("%s finish,status:%d,list:%d\n",__FUNCTION__,Status,m_webspeedList.size());
	return bret;
}
//---------------------------------------------------------------------------

void  TSpeedTest::StopWebTest()
{    
    NThreadCtrl = 0;
	DWORD dwTick = timeGetTime() + 5000;
	while(timeGetTime() < dwTick)
	{
		if(Status == 0) break;
		//Sleep(50);
		usleep(200 * 1000);
	}
	Status = 0;   
}

unsigned long TSpeedTest::GetTotalSize(){
	return TotalSize;
}

//---------------------------------------------------------------------------
int main_speed()
{
	return 0;
}
