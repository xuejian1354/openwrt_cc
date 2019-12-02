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
#include <string>
#include <wchar.h>

#include "XbHttp.h"
//#include "NetIp.h"
//#include "klogtracer.h"
//#include <atlstr.h>
#include "Common.h"

CXbHttp::CXbHttp(void)
{
	m_uPort = 80;
	m_sSocket = -1;
	m_iTimeout = 5000;
	m_uHttpCd = 200;
	m_loadTime = 0;
	m_dnsTime = 0;
	m_connTime = 0;
	m_TotalSize = 0;
	m_FlushingMode = false;
}


CXbHttp::~CXbHttp(void)
{
	DisConnect();
}

// ÉèÖÃ³¬Ê±(ms)
void CXbHttp::SetTimeout(int Interval) {
	m_iTimeout = Interval;
}

void CXbHttp::SetFlushingMode() {
	m_FlushingMode = true;
}

void CXbHttp::SetAddrInfo(const wchar_t *pIp, const unsigned short port)
{
	w2c(pIp,m_strSvrIP);
	m_uPort = port;
}

// ¶Ï¿ªÁ¬½Ó
void CXbHttp::DisConnect() {
	if (m_sSocket != -1) {
		close(m_sSocket);
		m_sSocket = -1;
	}
}

// ---------------------------------------------------------------------------

// µÈ´ýSocket¿ÉÐ´, ¿ÉÐ´·µ»Øtrue, ·ñÔò·µ»Øfalse
bool CXbHttp::WaitForWrite() {
	if (m_sSocket == -1)
		return false;
	//printf("%s begin\n",__FUNCTION__);
	
	fd_set FDSET;
	FD_ZERO(&FDSET);
	FD_SET(m_sSocket, &FDSET);

	timeval tm;
	tm.tv_sec = m_iTimeout / 1000;
	tm.tv_usec = m_iTimeout % 1000;
	int nError = select(m_sSocket + 1, NULL, &FDSET, NULL, &tm);
	if (nError > 0 && FD_ISSET(m_sSocket,&FDSET))
	{
		//printf("%s can write\n",__FUNCTION__);
		return true;
	}
	if (nError == -1) {
		printf("%s wait write socket error\n",__FUNCTION__);
	}

	if (nError == 0)
	{
		printf("%s wait write timeout\n",__FUNCTION__);
	}

	printf("%s cannot write\n",__FUNCTION__);
	
	return false;
}
// ---------------------------------------------------------------------------

// µÈ´ýSocket¿É¶Á, ÓÐÊý¾Ý¿É¶Á·µ»Øtrue, ·ñÔò·µ»Øfalse;
bool CXbHttp::WaitForRead() {
	if (m_sSocket == -1)
		return false;

	fd_set FDSET;
	FD_ZERO(&FDSET);
	FD_SET(m_sSocket, &FDSET);

	timeval tm;
	tm.tv_sec = m_iTimeout / 1000;
	tm.tv_usec = m_iTimeout % 1000;
	int nError = select(m_sSocket + 1,&FDSET,NULL,NULL,&tm);
	if (nError > 0 && FD_ISSET(m_sSocket,&FDSET))
	{
		return true;
	}
	
	return false;
}
// ---------------------------------------------------------------------------

int CXbHttp::ParseUrltoDomainName(const wchar_t *pUrl)
{
	int iret = -1;
	m_strUrl = pUrl;

	if (m_strUrl.length() < 4) return iret;

	m_uPort = 80;
	std::wstring temp = L"";
	std::wstring strTemp = m_strUrl;

	size_t indx = strTemp.find(L"://");
	if ((indx != std::wstring::npos) && (indx < 6))
	{
		std::wstring strhttp = strTemp.substr(0,indx + 3);
		strTemp = strTemp.substr(indx + 3);
		if (MyToLower(strhttp).find(L"https://") != std::wstring::npos)
		{
			m_uPort = 443;
		}
	}

	indx = strTemp.find(L"/");
	if (indx != std::wstring::npos)
	{
		temp = strTemp.substr(0,indx);
		m_strUrlObj = strTemp.substr(indx);
	}else
	{
		m_strUrlObj = L"/";
		temp = strTemp;
	}

	indx = temp.find(L":");
	if (indx != std::wstring::npos)
	{
		m_uPort = _wtoi((temp.substr(indx + 1).c_str()));
		m_strSvrDns = temp.substr(0,indx);
	}else
	{
		m_strSvrDns = temp;
	}
	if(m_strSvrDns.empty()) return iret;
	
	char ip[32] = {0}; 
	TElapsed tm;
	tm.Begin();
	std::string strADns;
	w2c(m_strSvrDns.c_str(),strADns);
	Dns(strADns.c_str(),ip,32);
	m_dnsTime = tm.End();

	m_strSvrIP = ip;
	if (!m_strSvrIP.empty())
	{
		iret = 0;
	}
	printf("%s,dns:%s,ip:%s,%d\n",__FUNCTION__,strADns.c_str(),ip,m_uPort);
	return iret;
}
// ---------------------------------------------------------------------------

// ´´½¨Ò»¸öSocketÁ¬½Ó
bool CXbHttp::CreateSocket() {
	bool bret = false;
	// create new socket
	DisConnect();
	m_sSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sSocket == -1)
	{
		TRACE_LOG1("get socket failed!errocode is %s",strerror(errno));
		return bret;
	}

	//ÉèÖÃÎª·Ç×èÈûµÄ
	int flags = 0;
	if((flags = fcntl(m_sSocket, F_GETFL, 0)) < 0 )
	{
		perror("fcntl 1 false\n");
		return bret;
	}
	if(fcntl(m_sSocket, F_SETFL, flags | O_NONBLOCK) < 0 )
	{
		perror("fcntl 2 false\n");
		return bret;
	}
	
	// connect socket
	struct sockaddr_in To;
	To.sin_family = AF_INET;
	To.sin_port = htons(m_uPort);
	To.sin_addr.s_addr = inet_addr(m_strSvrIP.c_str());
	if(connect(m_sSocket, (struct sockaddr*)&To, sizeof(To)) != 0)
	{
		if(errno != EINPROGRESS) { // EINPROGRESS 
			TRACE_LOG1("Failed to connect socket! errNo: %s.\n", strerror(errno));
            perror("connect false\n");
            return bret;
        }
	}else
	{
		TRACE_LOG1("connect return 0! errNo: %s.\n", strerror(errno));
		return true;
	}
	
	fd_set FDSET;
	FD_ZERO(&FDSET);
	FD_SET(m_sSocket, &FDSET);
  
	struct timeval tv;	// ÉèÖÃÁ¬½Ó³¬Ê±Ê±¼ä
	if (m_iTimeout == 0)
	{
		m_iTimeout = 10000;
	}
	tv.tv_sec = m_iTimeout/1000; // ÃëÊý
	tv.tv_usec = 0; // ºÁÃë
	
	TElapsed tm;
	tm.Begin();

	int nError = select(m_sSocket +1, NULL, &FDSET, NULL, &tv);
	
	m_connTime = tm.End();
	if(nError < 0)
	{   
		TRACE_LOG2("select failed ret=%d, errNo: %s.\n", nError,strerror(errno));	
		return bret;
	}else if(nError == 0)
	{
		TRACE_LOG1("%s.select timeout.\n",__FUNCTION__);	
		return bret;
	}else if(nError == 2) {
	 
		TRACE_LOG1("%s.nError == 2;select timeout.\n",__FUNCTION__);	
		return bret;
	}
	
	if(!FD_ISSET(m_sSocket, &FDSET))
	{
		TRACE_LOG1("%s.socket not writable\n",__FUNCTION__);
		return bret;
	}
	bret = true;
	
	TRACE_LOG1("%s.Connected\n",__FUNCTION__);
	
	flags = fcntl(m_sSocket, F_GETFL, 0);
	fcntl(m_sSocket, F_SETFL, flags & ~O_NONBLOCK);
	
	struct timeval send = {m_iTimeout/1000,0};
	struct timeval recv = {m_iTimeout/1000,0};
	setsockopt(m_sSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&send, sizeof(struct timeval));
	setsockopt(m_sSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&recv, sizeof(struct timeval));

	return bret;
}
// ---------------------------------------------------------------------------

int CXbHttp::SendData(const char *pbuf,DWORD dwsize)
{
	if (pbuf == NULL || dwsize == 0)
	{
		return -1;
	}
	
	return ::send(m_sSocket, pbuf, dwsize, 0);
}
// ---------------------------------------------------------------------------
// È¡µÃHTTP»Ø¸´ÐÅÏ¢Í·
bool CXbHttp::GetHTTPRspHeader() {

	bool bRet = false;
	while(true)
	{
		char RecvCh = '\0';

		if(recv(m_sSocket, &RecvCh, 1, 0) <= 0)
		{   
			TRACE_LOG1(_T("GetHTTPrecvHeader recv³ö´í,ErrNo: %s."), strerror(errno));
			break;
		}

		m_strRspHdr.append(1,RecvCh);
		if(m_strRspHdr.find("\r\n\r\n") != std::string::npos)
		{
			bRet = true;
			break;
		}
	}

	return bRet;
}


///¸ù¾ÝÇëÇóµÄÏà¶ÔURLÊä³öHTTPÇëÇóÍ·
DWORD CXbHttp::FormatRequestHeader(const char *pServer,	const char *pObject,DWORD dwcontentlen,int itype) 
{
	///µÚ1ÐÐ:·½·¨,ÇëÇóµÄÂ·¾¶,°æ±¾
	if (itype != 0)
	{
		m_requestheader.append("POST ");
	}
	else
	{
		m_requestheader.append("GET ");
	}
	m_requestheader += std::string(pObject) + std::string(" HTTP/1.1\r\n");

	///µÚ2ÐÐ:Ö÷»ú
	m_requestheader += std::string("Host: ") + std::string(pServer) + std::string("\r\n");

	///µÚ4ÐÐ:½ÓÊÕµÄÊý¾ÝÀàÐÍ
	m_requestheader.append("Accept: */*\r\n");

	///ÄÚÈÝÀàÐÍ
	m_requestheader.append("Content-Type: application/octet-stream\r\n");

	///½ÓÊÕµÄÊý¾ÝÓïÑÔ
	m_requestheader.append("Accept-Language: zh-cn\r\n");

	///µÚ5ÐÐ:ä¯ÀÀÆ÷ÀàÐÍ
	m_requestheader.append("User-Agent: XbHttpCommon\r\n");

	///µÚ6ÐÐ:Á¬½ÓÉèÖÃ,±£³Ö
	m_requestheader.append("Connection: close\r\n"); //Keep-Alive

	///µÚ6ÐÐ:Á¬½ÓÉèÖÃ,±£³Ö
	m_requestheader.append("Content-Length: ");
	char contentbuf[50] = {0};
	sprintf(contentbuf,"%lu",dwcontentlen);
	m_requestheader.append(contentbuf);
	m_requestheader.append("\r\n");

	///×îºóÒ»ÐÐ:¿ÕÐÐ
	m_requestheader.append("\r\n");

	return m_requestheader.length();
}


int CXbHttp::Get(const wchar_t *pwszUrl)
{
	return RequestData(pwszUrl, NULL, 0);
}

int CXbHttp::Post(const wchar_t *pwszUrl,const char *pPostData)
{
	return RequestData(pwszUrl, pPostData, 1);
}
bool CXbHttp::Send(const char *pPostData,int type)
{
	std::string postData;
	if (pPostData != NULL)
	{
		postData = pPostData;
	}
	DWORD dwsendlen = postData.length();
	std::string strdns;
	std::string strobj;
	w2c(m_strSvrDns.c_str(),strdns);
	w2c(m_strUrlObj.c_str(),strobj);
	FormatRequestHeader(strdns.c_str(),strobj.c_str(),dwsendlen,type);

	if (!WaitForWrite())
	{
		TRACE_TEXT(_T("Post WaitForWrite wait for write data\n"));
		return false;
	}
	
	std::string sendData = m_requestheader;
	sendData += postData;

	if (SendData(sendData.c_str(), sendData.length()) <= 0) 
	{		
		TRACE_TEXT(_T("Post send data failed\n"));
		return false;
	}
	TRACE_TEXT(_T("Send data finish\n"));
	return true;
}

int CXbHttp::RequestData(const wchar_t *pwszUrl,const char *pPostData,int type)
{
	m_dnsTime = -1;
	m_loadTime = -1;
	m_connTime = -1;
	m_recvdataTime = -1;
	m_fstpacketTime = -1;
	m_strRspHdr.clear();
	m_StrReqBody.clear();
	m_strUrlObj.clear();
	m_strUrl.clear();
	m_strSvrDns.clear();
	m_strSvrIP.clear();

	int bRet = -1;

	do{
		TElapsed tm;
		tm.Begin();

		if (ParseUrltoDomainName(pwszUrl) != 0)
			break;

		if (!CreateSocket())
			break;

		if (!Send(pPostData,type))
		{
			break;
		}

		if(!RecvData_())
		{
			break;
		}

		m_loadTime = tm.End();
		bRet = 0;
	}while(false);

	DisConnect();

	return bRet;
}

bool CXbHttp::GetHTTPRspCode()
{
	bool bret = false;
	std::string s1 = m_strRspHdr;

	unsigned int len = s1.find("\r\n");
	if(len == std::string::npos) return bret;

	s1 = s1.substr(0, len);
	len = s1.find(" ");
	if (len == std::string::npos) return bret;

	s1 = s1.substr(len);
	m_uHttpCd = atoi(s1.c_str());
	bret = true;

	return bret;
}

bool CXbHttp::RecvData_()
{
	bool bret = false;
	do{
		TElapsed tm2;
		tm2.Begin();
		if (!WaitForRead())
		{
			TRACE_TEXT(_T("WaitForRead failed\n"));
			break;
		}
		m_fstpacketTime = tm2.End();
		
		if (!GetHTTPRspHeader())
		{
			std::string strcov;
			w2c(m_strUrl.c_str(),strcov);
			TRACE_LOG1(_T("GetHTTPrecvHeader<%s> failed\n"),strcov.c_str());
			break;
		}

		if (!GetHTTPRspCode())
		{
			TRACE_LOG1(_T("GetHTTPRspCode<%s>failed\n"),m_strRspHdr.c_str());
			break;
		}

		if (m_uHttpCd != 200)
		{			
			std::string strcov;
			w2c(m_strSvrDns.c_str(),strcov);
			TRACE_LOG2(_T("commit<%s> errcode=%d"),strcov.c_str(),m_uHttpCd);
			break;			
		}
		char *pbuf = NULL;
		DWORD dwSize = 0;
		tm2.Begin();
		if(m_FlushingMode){
			FlushingRecvData(&dwSize);
		}else{
			RecvData((BYTE **)&pbuf, &dwSize);
			if (pbuf)
			{
				m_StrReqBody = pbuf;
				delete []pbuf;
			}
			//ProcessTransferEncodeChunked();
		}
		
		printf("%s,recv:%d\n",__FUNCTION__, dwSize);
		m_recvdataTime = tm2.End();
		
		m_TotalSize = dwSize;
		
		bret = true;
	}while(false);

	return bret;
}

int CXbHttp::RecvData(BYTE **pbuf,DWORD *pDwsize)
{
	DWORD dwRecv = 0;
	DWORD avalible = 0;
	int unitLen = 1024*64;
	TAutoMem mem(unitLen);

	while(true)
	{
		avalible = mem.GetSize() - dwRecv;

		if(avalible == 0){
			mem.ReAlloc(unitLen); //ReAlloc是再扩大。不是扩大到。
			avalible = mem.GetSize() - dwRecv;
		}
		int len = recv(m_sSocket, mem.GetBuffer() + dwRecv, avalible, 0);

		if(len <= 0 )
		{
			break;
		}
		
		if(dwRecv > 1024*1024*10)
			break;

		dwRecv += len;


	}

	*pDwsize = dwRecv;
	
	*pbuf = new BYTE[dwRecv + 1];
	memset(*pbuf, 0, dwRecv + 1);
	memcpy(*pbuf,mem.GetBuffer(), dwRecv);

	return 0;
}

int CXbHttp::FlushingRecvData(DWORD *pDwsize)
{
	DWORD dwRecv = 0;

	char buf[4096];
	while(true)
	{

		int len = recv(m_sSocket, buf, 4096, 0);
		//printf("receive len %d\n", len);
		if(len <= 0 )
		{
			break;
		}
		if(dwRecv > 1024*1024*100)
			break;
			
		dwRecv += len;
	}

	if (pDwsize != NULL)
	{
		*pDwsize = dwRecv;
	}

	return 0;
}


void CXbHttp::ProcessTransferEncodeChunked()
{
	std::string strtmptolower = m_strRspHdr;
	MyToLower(strtmptolower);
	if (strtmptolower.find("transfer-encoding: chunked") != std::string::npos && !m_StrReqBody.empty())
	{
		std::string strTmp = m_StrReqBody;
		std::string strDes;
		do 
		{
			size_t spos = strTmp.find("\r\n");
			if (spos == std::string::npos) break;

			int nsize = 0;         
			std::string strSize = std::string("0x") + strTmp.substr(0,spos);
			sscanf(strSize.c_str(),"%x",&nsize);  
			if (nsize == 0) break;
			
			strDes += strTmp.substr(spos + 2,nsize);
			strTmp = strTmp.substr(spos + 2 + nsize + 2);			

		} while (true);
		m_StrReqBody = strDes;
	}
}

void CXbHttp::GetTestResult(WebPageTestResult *pResult)
{
	if (pResult == NULL)
	{
		TRACE_TEXT(_T("GetTestResult param null\n"));
		return;
	}
	pResult->HttpCd = m_uHttpCd;
	pResult->LoadInSize = m_StrReqBody.length();
	pResult->LoadTime = m_loadTime;
	pResult->DnsTime = m_dnsTime;
	pResult->ConnTime = m_connTime;
	pResult->FstPckTime = m_fstpacketTime;
	pResult->RecvDataTime = m_recvdataTime;
}

DWORD CXbHttp::GetLoadSizeInBytes()
{
	return m_StrReqBody.length();
}

DWORD CXbHttp::GetTotalSize()
{
	return m_TotalSize; //m_StrReqBody.length();
}

double CXbHttp::GetLoadPageTime()
{
	return m_loadTime;
}

double  CXbHttp::GetDnsTime()
{
	return m_dnsTime;
}

double  CXbHttp::GetConnectTime()
{
	return m_connTime;
}

double  CXbHttp::GetFstPkgTime()
{
	return m_fstpacketTime;
}

const char *CXbHttp::GetRecvWebContent()
{
	return m_StrReqBody.c_str();
}

const char *CXbHttp::GetHttpRecvHeader()
{
	return m_strRspHdr.c_str();
}

const char *CXbHttp::GetServerIp()
{
	return m_strSvrIP.c_str();
}
