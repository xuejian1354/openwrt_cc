#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>

#include "Downloader.h"  
#include "Common.h"
#include "XbHttp.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <string.h>
#include <arpa/inet.h>

//---------------------------------------------------------------------------
static pthread_mutex_t mutexDown = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutexUp = PTHREAD_MUTEX_INITIALIZER;
#define SOCKET_BANDWIDTH_STD 2560
//---------------------------------------------------------------------------
inline int _wtoi(const wchar_t *str)
{
  return (int)wcstol(str, 0, 10);
}

TRecvThread::TRecvThread(bool CreateSuspended, THREADPARAM *para)
    : TCThread(), FSocket(-1), pFStatus(NULL), pFSize(NULL)
{   
	wmemset(DomainName, 0, 80);
	wmemset(FObject, 0, 100);
	wmemset(UserName, 0, 40);
	wmemset(Password, 0, 40);
	wmemset(FServerIP, 0, 40);

    Site1 = *(para->pSite1);
    Site2 = *(para->pSite2);
	pFStatus = para->pStatus;
	pFSize = para->pdwSize;
    pFControl = para->pControl;
	pDLThreadCount = para->pThreadCnt;
}
//---------------------------------------------------------------------------
void TRecvThread::Run()
{
	if(start() == 0)
	{
		pthread_mutex_lock(&mutexDown);
		*pDLThreadCount = *pDLThreadCount + 1;
		pthread_mutex_unlock(&mutexDown);
	}
}
//---------------------------------------------------------------------------

TRecvThread::~TRecvThread()
{
    if(FSocket != -1)                //¹Ø±Õsocket
    {
        //closesocket(FSocket);
        close(FSocket);
        FSocket = -1;
    }
}
//---------------------------------------------------------------------------
      
void TRecvThread::AnalyseUrl(int idx)
{
    TSITE *pSite = &Site1;
    if(idx == 2) 
	{
		pSite = &Site2;
	}
	std::wstring strwUrl;
	c2w(pSite->url,strwUrl);
	if(pSite->type == 2)
	{   //ftp
		FType = 2;
		//DownloadFtpFile(pSite->url);
		DownloadFtpFile_Socket(strwUrl.c_str());
	}
	else
	{   //http
		FType = 1;
		DownloadHttpFile(strwUrl.c_str());
	}
}
//---------------------------------------------------------------------------

BOOL TRecvThread::AnalyseHttp(const wchar_t *sURL)
{
	printf("%s begin\n",__FUNCTION__);
	BOOL bret = FALSE;
	if(sURL == NULL || wcslen(sURL) == 0)
	{
		TRACE_TEXT("AnalyseHttp url empty\n");
		return bret;
	}
	wmemset(DomainName, 0, 80);
	wmemset(FServerIP, 0, 40);
	wmemset(FObject, 0, 100);

	std::wstring strUrl = sURL;
	size_t idx = strUrl.find(L"http://");
	if (idx != std::wstring::npos)
	{
		strUrl = strUrl.substr(idx + 7);
	}
	
	std::wstring strDomain;
    idx = strUrl.find(L"/");
	if(idx != std::wstring::npos)
    {
		strDomain = strUrl.substr(0,idx);
        //wcsncpy_s(FObject, 100, strUrl.substr(idx).c_str(),99);
        wcsncpy(FObject, strUrl.substr(idx).c_str(),99);
    }else
	{
		strDomain = strUrl;
	}

	idx = strDomain.find(L":");
	if(idx != std::wstring::npos)
	{
	//	wcsncpy_s(DomainName, 80, strDomain.substr(0,idx).c_str(),79);
		wcsncpy(DomainName, strDomain.substr(0,idx).c_str(),79);
		FPort = _wtoi(strDomain.substr(idx + 1).c_str());
	}
	else 
	{
	//	wcsncpy_s(DomainName, 80, strDomain.c_str(),79);
		wcsncpy(DomainName, strDomain.c_str(),79);
		FPort = 80;
	}
	std::string strHost;
	w2c(DomainName,strHost);
	
	char sip[32];
	memset(sip,0,32);
	Dns(strHost.c_str(),sip,32);
	int iplen = 40;
	c2w(sip,FServerIP,&iplen);

	if((FServerIP[0] != 0) && (FObject[0] != 0)) 
	{
		bret = TRUE;
	}
	printf("%s,dns:%s,ip:%s,%d\n",__FUNCTION__,strHost.c_str(),sip,FPort);
	return bret;
}
//---------------------------------------------------------------------------
//·ÖÎöURL
//ftp://ceshu:gxdxceshu@202.103.226.187:21/home/ceshu/download/30M.TXT
//p1 = L"ftp://aqzj:aqzj@192.168.1.37/Package/V1.0/2011-package/anhui-100/ah0625.exe";
BOOL TRecvThread::AnalyseFtp(const wchar_t *sURL)
{
	BOOL bret = FALSE;
    wmemset(DomainName, 0, 80);
    wmemset(FObject, 0, 100);
    wmemset(UserName, 0, 40);
    wmemset(Password, 0, 40);
	wmemset(FServerIP, 0, 40);

	std::wstring strUrl = sURL;
	std::wstring strTmp = strUrl;
	MyToLower(strTmp);
	std::string strAurl;
	w2c(sURL,strAurl);
	TRACE_LOG1(_T("ftp AnalyseFtp url:%s\n"),strAurl.c_str());
	
	size_t idx = strTmp.find(L"ftp://");
	if(idx != std::wstring::npos)
    {  
		strUrl = strUrl.substr(idx + 6);
    }

	idx = strUrl.find(L"@");
	if(idx != std::wstring::npos)
    {
		strTmp = strUrl.substr(0,idx);
		strUrl = strUrl.substr(idx + 1);
		idx = strTmp.find(L":");
		if (idx != std::wstring::npos)
		{
			//wcsncpy_s(UserName, 40, strTmp.substr(0,idx).c_str(), 39);
			wcsncpy(UserName, strTmp.substr(0,idx).c_str(), 39);
			//wcsncpy_s(Password, 40, strTmp.substr(idx + 1).c_str(), 39);
			wcsncpy(Password, strTmp.substr(idx + 1).c_str(), 39);
		}
    }

    if(wcslen(UserName) == 0 || wcslen(Password) == 0)
    {   //Ã»ÓÐÖ¸¶¨ÓÃ»§ÃûÃÜÂë
        //wcscpy_s(UserName, L"anonymous");
        wcscpy(UserName, L"anonymous");
        //wcscpy_s(Password, L"zh@163.com");
        wcscpy(Password, L"zh@163.com");
    }

    FPort = 21;                             //ftpÄ¬ÈÏ¶Ë¿Ú
	idx = strUrl.find(L"/");
	if (idx != std::wstring::npos)
	{
		//wcsncpy_s(FObject, 100, strUrl.substr(idx).c_str(), 99);  
		wcsncpy(FObject, strUrl.substr(idx).c_str(), 99);  
		strTmp = strUrl.substr(0,idx);
		idx = strTmp.find(L":");
		if (idx != std::wstring::npos)
		{
			//wcsncpy_s(DomainName, 80, strTmp.substr(0,idx).c_str(), 79);
			wcsncpy(DomainName, strTmp.substr(0,idx).c_str(), 79);
			FPort = _wtoi(strTmp.substr(idx + 1).c_str());
		}else
		{
			//wcsncpy_s(DomainName, 80, strTmp.c_str(), 79);
			wcsncpy(DomainName, strTmp.c_str(), 79);
		}		
	}

	if (wcslen(DomainName) > 0 && wcslen(FObject) > 0)
	{
		std::string strHost;
		w2c(DomainName,strHost);
		char sip[32];
		memset(sip,0,32);
		Dns(strHost.c_str(),sip,32);
		int nSize = 40;
		c2w(sip,FServerIP, &nSize);
		if (nSize > 40)
		{			
			TRACE_LOG1(_T("ftp Dns failed,FServerIP is to small.%s\n"),(sip));
		}else
		{
			bret = TRUE;
		}		
	}

	return bret;
}

//---------------------------------------------------------------------------
int  TRecvThread::SendCommandToServer(SOCKET &st,const char * pcmd,std::string *strRecv)
{
	int nret = 0;
	
	if (pcmd != NULL)
	{
		if(send(st, pcmd, strlen(pcmd), 0) == -1)
		{
			TRACE_LOG1(_T("ftp send error err=%s\n"),strerror(errno));
			return nret;
		}
	}
	
	std::string strHeader;
	for(int i=0; i<1024; i++)
	{
		char RecvCh = '\0';

		if(recv(st, &RecvCh, 1, 0) <= 0)
		{   
			TRACE_LOG1("SendCommandToServer recv³ö´í, ErrNo: %s.\n", strerror(errno));
			break;
		}

		strHeader.append(1,RecvCh);
		if(strHeader.find("\r\n") != std::string::npos)
		{
			break;
		}
	}
	MyTrim(strHeader);
	std::string strCode;
	for (unsigned int i=0; i < strHeader.length() && isdigit((int)strHeader[i]); ++i)
	{
			strCode.append(1,strHeader[i]);
	}
	nret = atoi(strCode.c_str());
	if (strRecv != NULL)
	{
		*strRecv = strHeader;
	}

	return nret;
}
//---------------------------------------------------------------------------
BOOL TRecvThread::ParsePasvData(std::string &strRecv,std::string &strIp,int &nPort)
{
	BOOL bret = FALSE;
	
	const char *p = strchr ( strRecv.c_str(), '(');
	if ( !p ) return bret;

	std::string csPasvStr = p+1, csTemp;
	unsigned int nPosStart = 0, nPosEnd = 0;
	int nMultiple = 0, nMantissa = 0;
	for ( int i=0; ; i++ )
	{
		nPosEnd = csPasvStr.find(",", nPosStart);
		if ( nPosEnd == std::string::npos )
		{
			if ( i == 5 )
			{
				nPosEnd = csPasvStr.find(")", nPosStart );
				csTemp = csPasvStr.substr(nPosStart, nPosEnd - nPosStart );
				nMantissa = atoi(csTemp.c_str());
				break;
			}
			else return bret;
		}

		csTemp = csPasvStr.substr(nPosStart, nPosEnd - nPosStart);
		MyTrim(csTemp);
		if ( i < 4 )
		{
			if ( !strIp.empty() ) strIp += ".";
			strIp += csTemp;
		}
		else if ( i == 4 )
		{
			nMultiple = atoi ( csTemp.c_str());
		}
		else return bret;

		nPosStart = nPosEnd + 1;
	}
	nPort = nMultiple * 256 + nMantissa;

	if(!strIp.empty() && nPort != 0) bret = TRUE;

	return bret;
}

//---------------------------------------------------------------------------
BOOL TRecvThread::DownloadFtpFile_Socket(const wchar_t *sURL)
{
	BOOL bret = FALSE;
	SOCKET DataSocket = -1;

	do{	
		if(!AnalyseFtp(sURL)) 
		{
			break;
		}
		if(!CreateSocket(&FSocket,FServerIP,(USHORT)FPort,1))
		{
			break;
		}
		//½ÓÊÕ½¨Á¢Á¬½Óºó·¢¹ýÀ´µÄÈ·ÈÏ
		if(SendCommandToServer(FSocket,NULL,NULL) != 220)
		{
			TRACE_TEXT(_T("connect failed."));
			break;
		}
		std::string strUser;
		w2c(UserName,strUser);
		strUser = "USER " + strUser + "\r\n";
		if(SendCommandToServer(FSocket,strUser.c_str(),NULL) != 331)
		{
			TRACE_TEXT(_T("set user failed."));
			break;
		}
		
		std::string strpwd;
		w2c(Password,strpwd);
		strpwd = "PASS " + strpwd + "\r\n";
		if(SendCommandToServer(FSocket,strpwd.c_str(),NULL) != 230)
		{
			TRACE_TEXT(_T("set user failed."));
			break;
		}

		if(SendCommandToServer(FSocket,"TYPE I\r\n",NULL) != 200)
		{
			TRACE_TEXT(_T("set TYPE I failed."));
			break;
		}
		
		std::string strRecv;
		if(SendCommandToServer(FSocket,"PASV\r\n",&strRecv) != 227)
		{
			TRACE_TEXT(_T("set TYPE I failed."));
			return bret;
		}
		std::string strIp;
		int nPort = 0;
		if(!ParsePasvData(strRecv,strIp,nPort))
		{
			TRACE_TEXT(_T("ParsePasvData failed."));
			break;
		}

		//½¨Á¢Êý¾ÝÍ¨µÀ
		std::wstring strwIp;
		c2w(strIp.c_str(),strwIp);
		if(!CreateSocket(&DataSocket,strwIp.c_str(),(USHORT)nPort,2))
		{
			break;
		}

		std::string strObj;
		w2c(FObject,strObj);
		strObj = "RETR " + strObj + "\r\n";
		if(SendCommandToServer(FSocket,strObj.c_str(),NULL) != 150)
		{
			TRACE_LOG1(_T("commit obj %s failed."), strObj.c_str());
			break;
		}

		bool berr = false;
		*pFStatus = 1;
		char RecvBuf[1460];  
		while(!this->IsTerminated() && (*pFControl != 0) )
		{       
			int len = recv(DataSocket, RecvBuf, 1460, 0);
			if(len <= -1)
			{     
				berr = true;
				TRACE_LOG1("Ftp Socket recv failed, ErrNo: %s.", strerror(errno));
				break;
			}
			//m_Mutex.Lock();
			*pFSize += len; 
			//m_Mutex.UnLock();
		}
		if (berr)
		{
			break;
		}
		*pFStatus = 5;
		bret = TRUE;

	}while(FALSE);

	if(DataSocket != -1)
	{
		//closesocket(DataSocket);
		close(DataSocket);
		DataSocket = -1;
	}

	if (FSocket != -1)
	{
		//closesocket(FSocket);
		close(FSocket);
		FSocket = -1;
	}

	if (!bret)
	{
		*pFStatus = 4;
	}
	return bret;
}
//---------------------------------------------------------------------------

BOOL TRecvThread::CreateSocket(SOCKET *pSocket,const wchar_t *pIp,USHORT nport,int ntype)
{
	printf("%s\n",__FUNCTION__);
	if(*pSocket != -1)
	{
		//closesocket(*pSocket);
		close(*pSocket);
		*pSocket = -1;
	}

    if(pIp[0] == 0) return FALSE;

    char ip[40] = {0};
	memset(ip,0,40);
    int len = (int)wcslen(pIp);
    for(int i=0; i<len; i++) 
        ip[i] = (char)pIp[i];

	printf("%s,serip:%s\n",__FUNCTION__,ip);
      
    //create new socket
    SOCKET &aSocket = *pSocket;
    aSocket = -1;
    aSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(aSocket == -1)
    {   
        TRACE_LOG1("Failed to create a new socket, errNo: %s.\n", strerror(errno));
        return FALSE;
    }

	//ÉèÖÃÎª·Ç×èÈûµÄ
	int flags = 0;
	if((flags = fcntl(aSocket, F_GETFL, 0)) < 0 )
	{
		perror("fcntl 1 false\n");
		return FALSE;
	}
	if(fcntl(aSocket, F_SETFL, flags | O_NONBLOCK) < 0 )
	{
		perror("fcntl 2 false\n");
		return FALSE;
	}

	//connect socket
	//SOCKADDR_IN To;
	sockaddr_in To;
	To.sin_family = AF_INET;
	To.sin_port = htons(nport);
	To.sin_addr.s_addr = inet_addr(ip);

    if(connect(aSocket, (struct sockaddr*)&To, sizeof(To)) != 0)
	{
		if(errno != EINPROGRESS) { // EINPROGRESS 
			TRACE_LOG1("Failed to connect socket! errNo: %s.\n", strerror(errno));
            perror("connect false\n");
            return FALSE;
        }
	}else
	{
		TRACE_LOG1("connect return 0! errNo: %s.\n", strerror(errno));
		return TRUE;
	}
	fd_set FDSET;
	FD_ZERO(&FDSET);
	FD_SET(aSocket, &FDSET);
	
          
	struct timeval tv;	// ÉèÖÃÁ¬½Ó³¬Ê±Ê±¼ä
	tv.tv_sec = 6; // ÃëÊý
	tv.tv_usec = 0; // ºÁÃë

	int nError = select(aSocket +1, NULL, &FDSET, NULL, &tv);
	if(nError < 0)
	{   
		TRACE_LOG2("select failed ret=%d, errNo: %s.\n", nError,strerror(errno));	
		return FALSE;
	}else if(nError == 0)
	{
		TRACE_LOG1("%s.select timeout.\n",__FUNCTION__);	
		return FALSE;
	}else if(nError == 2) {
     
		TRACE_LOG1("%s.nError == 2;select timeout.\n",__FUNCTION__);	
        return FALSE;
    }
	
	if(!FD_ISSET(aSocket, &FDSET))
	{
		TRACE_LOG1("%s.socket not writable\n",__FUNCTION__);
		return FALSE;
	}
	TRACE_LOG1("%s.Connected\n",__FUNCTION__);
	
	flags = fcntl(aSocket, F_GETFL, 0);
	fcntl(aSocket, F_SETFL, flags & ~O_NONBLOCK);
	
	struct timeval send = {6,0};
	struct timeval recv = {6,0};
	setsockopt(aSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&send, sizeof(struct timeval));
	setsockopt(aSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&recv, sizeof(struct timeval));

	//int nZero = 128 * 1024;
	//setsockopt(aSocket, SOL_SOCKET, SO_RCVBUF, (char *)&nZero,sizeof(int));

    return TRUE;
}
//---------------------------------------------------------------------------

//È¡µÃHTTP»Ø¸´ÐÅÏ¢Í·
BOOL TRecvThread::GetHTTPrecvHeader(std::string &strHeader)
{
    BOOL bRet = FALSE;
	printf("%s\n",__FUNCTION__);
    for(int i=0; i<2048; i++)
    {
		char RecvCh = '\0';
  
        if(recv(FSocket, &RecvCh, 1, 0) <= 0)
        {   
			fprintf(stderr, "GetHTTPrecvHeader recv fals,ErrNo: %s.\n", strerror(errno));
            break;
        }
        
		strHeader.append(1,RecvCh);
		if(strHeader.find("\r\n\r\n") != std::string::npos)
		{
			bRet = TRUE;
			break;
		}
    }
	printf("%s finish,%s\n",__FUNCTION__,strHeader.c_str());
    return bRet;
}
//---------------------------------------------------------------------------
BOOL TRecvThread::MakeRequestParam(std::string &strReq)
{
    std::string strobj;
	std::string strDomainName;
	if(!w2c(FObject,strobj)) return FALSE;
	if(!w2c(DomainName,strDomainName)) return FALSE;

	strobj = std::string("GET ") + strobj + std::string(" HTTP/1.1\r\n");
	strDomainName = std::string("Host: ") + strDomainName + std::string("\r\n");
	strReq = strobj + strDomainName;
	strReq.append("Accept: */*\r\n");
	strReq.append("Content-Type: application/octet-stream\r\n");
	//strReq.append("User-Agent: Mozilla/4.0(compatible; MSIE 6.0; Windows NT 5.1)\r\n");
	strReq.append("Connection: Keep-Alive\r\n\r\n");

	return TRUE;
}
//---------------------------------------------------------------------------
BOOL TRecvThread::SendRequest(std::string &strReq)
{
	printf("%s,param:%s\n",__FUNCTION__,strReq.c_str());
	
	BOOL bret = FALSE;
	if(send(FSocket, strReq.c_str(), strReq.length(), 0) <= 0)
	{
		//OnError(L"send error File: %s, LineNo: %d.", __LINE__);
		fprintf(stderr, "send error File: %s, LineNo: %d.\n", __FILE__,  __LINE__);
		return bret;
	}

	//µÃµ½»Ø¸´µÄÐÅÏ¢Í·
	std::string strRecvHeader;
	if(GetHTTPrecvHeader(strRecvHeader))
	{
		std::string strRsp;
		unsigned int idx = strRecvHeader.find("\r\n");
		if (idx == std::string::npos)
		{
			TRACE_TEXT(strRecvHeader.c_str());
			TRACE_TEXT("\n");
			return bret;
		}
		
		strRsp = strRecvHeader.substr(0,idx);
		idx = strRsp.find(" 200");
		if(idx == std::string::npos) 
			idx = strRsp.find(" 206");
	
		if(idx != std::string::npos)
		{
			bret = TRUE;
			printf("%s,true\n",__FUNCTION__);
		}
		else
		{
			TRACE_TEXT(strRecvHeader.c_str());
			TRACE_TEXT("\n");
		}
	}

	return bret;
}
//---------------------------------------------------------------------------

BOOL TRecvThread::DownloadHttpFile(const wchar_t *sURL)
{
	std::string straurl;
	w2c(sURL,straurl);
	printf("DownloadHttpFile,%s\n",straurl.c_str());
	
	BOOL bret = FALSE;
	std::string strReqParam;
	char buf[1460] = {0};
	int len;

    if(!AnalyseHttp(sURL)) 
	{
		//OnError(L"DownloadHttpFile AnalyseHttp,%s,%d",__LINE__);
		fprintf(stderr, "DownloadHttpFile AnalyseHttp,%s,%d\n", __FILE__,  __LINE__);
		goto Exit0;
	}


    if(!CreateSocket(&FSocket,FServerIP,(USHORT)FPort)) 
    {
		//OnError(L"DownloadHttpFile CreateSocket,%s,%d",__LINE__);
		fprintf(stderr, "DownloadHttpFile CreateSocket,%s,%d\n",__FILE__, __LINE__);
        goto Exit0;
    }

	if(!MakeRequestParam(strReqParam))
	{
		//OnError(L"DownloadHttpFile MakeRequestParam,%s,%d",__LINE__);
		fprintf(stderr, "DownloadHttpFile MakeRequestParam,%s,%d\n",__FILE__, __LINE__);
		goto Exit0;
	}

	if(!SendRequest(strReqParam))
	{
		//OnError(L"DownloadHttpFile SendRequest,%s,%d",__LINE__);
		fprintf(stderr, "DownloadHttpFile SendRequest,%s,%d\n",__FILE__, __LINE__);
		goto Exit0;
	}
	printf("%s,begin recv data\n",__FUNCTION__);
	
    *pFStatus = 2;
    while(!this->IsTerminated() && (*pFControl != 0))
    {       
        len = recv(FSocket, buf, 1460, 0);
        
        if(len == 0)
        {
            
            if(errno == EAGAIN  || errno == EWOULDBLOCK || errno == EINTR)
                continue;
            else{
                goto Exit0;
                TRACE_LOG1("DownloadHttpFile recv err 0, ErrNo: %s.\n", strerror(errno));
            }
                 
        }
        if(len == -1)
        {  
        	if(errno == EINTR)
        		continue;
            else{
            	TRACE_LOG1("DownloadHttpFile recv err2, ErrNo: %d.\n", errno);
				TRACE_LOG1("DownloadHttpFile recv err, ErrNo: %s.\n", strerror(errno));
            	goto Exit0;
            }

        }

		//m_Mutex.Lock();
		*pFSize += len; 
		//m_Mutex.UnLock();
    }
	bret = TRUE;
	*pFStatus = 5;
	
Exit0:
	printf("DownloadHttpFile finish\n");
	if (FSocket != -1)
	{
		//closesocket(FSocket);
		close(FSocket);
		FSocket = -1;
	}
	if (!bret)
	{
		*pFStatus = 4;
	}

    return bret;
}

//---------------------------------------------------------------------------

void TRecvThread::Execute()
{   
	if (IsTerminated())
	{
		*pFStatus = 3;
		goto ExitEntry;
	}
	*pFStatus = 1;                                        //»ñÈ¡ÏÂÔØÐÅÏ¢
	AnalyseUrl(1);
     
	 /*
	if(*pFStatus == 4)
	{   //Ê§°Ü
		*pFStatus = 1;
		
		if (IsTerminated())
		{
			*pFStatus = 3;
			goto ExitEntry;
		}

		AnalyseUrl(2);
		if (*pFStatus == 4)
		{
			std::string strtmp;
			w2c(FServerIP,strtmp);
			TRACE_LOG2("second<ip=%s> faild,type=%d\n",strtmp.c_str(),FType);
		}
	}
	*/
ExitEntry:	
	pthread_mutex_lock(&mutexDown);
	*pDLThreadCount = *pDLThreadCount - 1;
	pthread_mutex_unlock(&mutexDown);
}
//---------------------------------------------------------------------------

TSendThread::TSendThread(bool CreateSuspended, UPTHREADPARAM *para):TCThread(),pFStatus(NULL),pFSize(NULL)
{
	//memcpy_s(&UpPara,sizeof(UPTHREADPARAM),para,sizeof(UPTHREADPARAM));
	memcpy(&UpPara,para,sizeof(UPTHREADPARAM));
	pFSize = UpPara.pdwSize;
	pFStatus = UpPara.pStatus;
	pULThreadCount = UpPara.pThreadCnt;
}

//---------------------------------------------------------------------------
void TSendThread::Run()
{
	if(start() == 0)
	{
		pthread_mutex_lock(&mutexUp);
		*pULThreadCount = *pULThreadCount + 1;
		pthread_mutex_unlock(&mutexUp);
	}
}
//---------------------------------------------------------------------------

TSendThread::~TSendThread()
{
	
}
//---------------------------------------------------------------------------

void TSendThread::Execute()
{
	TRACE_TEXT(_T("upthread Execute running\n"));
	*pFStatus = 2;

	if(!IsTerminated() && !UpLoad(UpPara.pSite1))
	{
		TRACE_TEXT(_T("upthread Execute running 2\n"));
		if(!IsTerminated() && !UpLoad(UpPara.pSite2))
		{
			*pFStatus = 4;
			goto Eixt0;
		}
	}

	*pFStatus = 5;                   //±íÃ÷Ïß³ÌÍË³ö
Eixt0:
	pthread_mutex_lock(&mutexUp);
	*pULThreadCount = *pULThreadCount - 1;
	pthread_mutex_unlock(&mutexUp);
}
//---------------------------------------------------------------------------
//LuanSh

bool TSendThread::UpLoad(TSITE *pSite)
{
	TRACE_TEXT(_T("upthread UpLoad running\n")); 
	
	bool bret = false;
	if (pSite == NULL || strlen(pSite->url) == 0)
	{
		TRACE_TEXT(_T("UpLoad addr empty\n"));
		return bret;
	}
	if (pSite->port == 0)
	{
		pSite->port = 80; 
	}
	std::wstring strmp;
	c2w(pSite->url,strmp);
	TRACE_LOG2(_T("UpLoad begin,%s,%d\n"),pSite->url,pSite->port);
	
	CXbHttp	xbhttp;
	xbhttp.SetAddrInfo(strmp.c_str(),pSite->port);
	xbhttp.SetTimeout(5000);

	if (!xbhttp.CreateSocket())
	{
		TRACE_LOG1(_T("UpLoad CreateSocket failed,%s\n"),pSite->url);
		return bret;
	}
	
	TRACE_TEXT(_T("UpLoad CreateSocket after\n"));
	
	//xbhttp.SetTimeout(500);
	char buf[1500];

	struct sockaddr_in servaddr;
	struct msghdr msg;
    struct iovec msg1[10], msg2;

    memset(msg1, 0, sizeof(msg1));

    for(int i=0; i< 10; i++){
        msg1[i].iov_base = buf;
        msg1[i].iov_len = 1460;
    }

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = msg1;
    msg.msg_iovlen = 10;

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(pSite->url);
    servaddr.sin_port=htons(pSite->port);

	TAutoMem mem(1460);
	char *pBuff = (char*)mem.GetBuffer();
	memset(pBuff, 'A', mem.GetSize());
	int sentLen = 0;
	
	TRACE_TEXT(_T("UpLoad before send\n"));
	
	for(; (*(UpPara.pControl) != 0) && !IsTerminated();)
	{
		//if(!xbhttp.WaitForWrite()) continue;
		
		//TRACE_TEXT(_T("UpLoad send\n"));
		sentLen = sendmsg(xbhttp.m_sSocket, &msg, 0);
		if(sentLen < 0){
			if(errno == EAGAIN  || errno == EWOULDBLOCK || errno == EINTR)
                continue;
            else{
            	TRACE_LOG2(_T("SendData failed,%s,%s\n"),pSite->url,strerror(errno));
				break;	
            }
		}

		//TRACE_LOG1(_T("UpLoad send after %d\n"),sentLen);
		//m_Mutex.Lock();
		*pFSize += 1460*10;
		//m_Mutex.UnLock();
	}

	if (*(UpPara.pControl) == 0)
	{
		*pFStatus = 3;
		bret = true;
		TRACE_TEXT(_T("user stop upload\n"));
	}
	TRACE_TEXT(_T("UpLoad finish\n"));
	return bret;
}
//---------------------------------------------------------------------------

TDownloader::TDownloader():FControl(0),FStatus(0),ThreadCount(0)
{
	for(int i=0; i<5; ++i) 
	{
		pUpThread[i] = NULL;
	}
	FUStatus1 = 0;
	FUStatus2 = 0;
	FUStatus3 = 0;
	FUStatus4 = 0;

	dwUSize1 = 0;
	dwUSize2 = 0;
	dwUSize3 = 0;
	dwUSize4 = 0;

	for(int i=0; i<5; ++i) 
	{
		pDownThread[i] = NULL;
	}

	FDStatus1 = 0;
	FDStatus2 = 0;
	FDStatus3 = 0;
	FDStatus4 = 0;

	dwDSize1 = 0;
	dwDSize2 = 0;
	dwDSize3 = 0;
	dwDSize4 = 0;

	memset(WebItems,0,10 * sizeof(TWEBINFO));
	for (int i=0; i<10; ++i)
	{
		pthread[i] = NULL;
	}
}
//---------------------------------------------------------------------------

TDownloader::~TDownloader()
{
}
//---------------------------------------------------------------------------
void TDownloader::StartUpLoad(TSITE *pSite, int Items)
{
	int iSite = 0;
	for(int i=0; i<Items; i++) 
	{
		 if(strlen(pSite[i].url) > 0) 
            iSite++;
	}

	FControl = 1;
	UPTHREADPARAM p1, p2, p3, p4;
	switch(iSite)
	{
	default:
	case 0:
		FStatus = 5;
		TRACE_TEXT(_T("no upserver url\n"));
		return;
	case 1:
		p1.pSite1 = &pSite[0]; p1.pSite2 = &pSite[0]; p1.pControl = &FControl; p1.pdwSize = &dwUSize1;p1.pStatus = &FUStatus1;
		p2 = p1; p2.pdwSize = &dwUSize2;p2.pStatus = &FUStatus2;
		p3 = p1; p3.pdwSize = &dwUSize3;p3.pStatus = &FUStatus3;
		p4 = p1; p4.pdwSize = &dwUSize4;p4.pStatus = &FUStatus4;
		break;

	case 2:
		p1.pSite1 = &pSite[0]; p1.pSite2 = &pSite[1]; p1.pControl = &FControl; p1.pdwSize = &dwUSize1;p1.pStatus = &FUStatus1;
		p2 = p1; p2.pdwSize = &dwUSize2;p2.pStatus = &FUStatus2;
		p3 = p1; p3.pdwSize = &dwUSize3;p3.pStatus = &FUStatus3;
		p4 = p1; p4.pdwSize = &dwUSize4;p4.pStatus = &FUStatus4;
		break;
	case 3:
		p1.pSite1 = &pSite[0]; p1.pSite2 = &pSite[2]; p1.pControl = &FControl; p1.pdwSize = &dwUSize1;p1.pStatus = &FUStatus1;
		p2.pSite1 = &pSite[1]; p2.pSite2 = &pSite[2]; p2.pControl = &FControl; p2.pdwSize = &dwUSize2;p2.pStatus = &FUStatus2;
		p3 = p1; p3.pdwSize = &dwUSize3;p3.pStatus = &FUStatus3;
		p4 = p2; p4.pdwSize = &dwUSize4;p4.pStatus = &FUStatus4;
		break;
	case 4:
		p1.pSite1 = &pSite[0]; p1.pSite2 = &pSite[3]; p1.pControl = &FControl; p1.pdwSize = &dwUSize1;p1.pStatus = &FUStatus1;
		p2.pSite1 = &pSite[1]; p2.pSite2 = &pSite[3]; p2.pControl = &FControl; p2.pdwSize = &dwUSize2;p2.pStatus = &FUStatus2;
		p3.pSite1 = &pSite[2]; p3.pSite2 = &pSite[3]; p3.pControl = &FControl; p3.pdwSize = &dwUSize3;p3.pStatus = &FUStatus3;
		p4 = p1; p4.pdwSize = &dwUSize4;p4.pStatus = &FUStatus4;
		break;
	case 5:
		p1.pSite1 = &pSite[0]; p1.pSite2 = &pSite[4]; p1.pControl = &FControl; p1.pdwSize = &dwUSize1;p1.pStatus = &FUStatus1;
		p2.pSite1 = &pSite[1]; p2.pSite2 = &pSite[4]; p2.pControl = &FControl; p2.pdwSize = &dwUSize2;p2.pStatus = &FUStatus2;
		p3.pSite1 = &pSite[2]; p3.pSite2 = &pSite[4]; p3.pControl = &FControl; p3.pdwSize = &dwUSize3;p3.pStatus = &FUStatus3;
		p4.pSite1 = &pSite[3]; p4.pSite2 = &pSite[4]; p4.pControl = &FControl; p4.pdwSize = &dwUSize4;p4.pStatus = &FUStatus4;
		break;
	}

	FStatus = 1;
	
	p1.pThreadCnt = &ThreadCount;
	p2.pThreadCnt = &ThreadCount;
	p3.pThreadCnt = &ThreadCount;
	p4.pThreadCnt = &ThreadCount;

	pUpThread[0] = new TSendThread(false, &p1);
	pUpThread[1] = new TSendThread(false, &p2);
	pUpThread[2] = new TSendThread(false, &p3);
	pUpThread[3] = new TSendThread(false, &p4);
	pUpThread[0]->Run();
	pUpThread[1]->Run();
	pUpThread[2]->Run();
	pUpThread[3]->Run();
	
}
//---------------------------------------------------------------------------
void TDownloader::StopUpLoad()
{
	if(FStatus != 1) return;
	FStatus = 0;
	FControl = 0;

	while(ThreadCount > 0)
	{
		usleep(100 * 1000);
	}  
	for(int i=0; i<5; ++i) 
	{
		if(pUpThread[i] != NULL)
		{
			delete pUpThread[i];
			pUpThread[i] = NULL;
		}
	}
}
//---------------------------------------------------------------------------

//¿ªÊ¼ÏÂÔØ
void TDownloader::StartDownload(TSITE *pSite, int Items)
{
    int iSite = 0;
    for(int i=0; i<Items; ++i) 
    {
        if(strlen(pSite[i].url) > 0) 
            iSite++;
    }

    FControl = 1;
    THREADPARAM p1, p2, p3, p4;
    switch(iSite)
    {
	default:
    case 0:
		FStatus = 5;
        //OnError(L"·þÎñÆ÷²âËÙ²ßÂÔÅäÖÃ´íÎó£¬Ê¹ÓÃÄ¬ÈÏµØÖ·²âËÙ£¡File: %s, LineNo: %d.", __LINE__);
        fprintf(stderr, "no strategy File: %s, LineNo: %d.\n", __FILE__, __LINE__);
		return;
    case 1:
		p1.pSite1 = &pSite[0], p1.pSite2 = &pSite[0]; p1.pControl = &FControl; p1.pdwSize = &dwDSize1;p1.pStatus = &FDStatus1;
		p2 = p1; p2.pdwSize = &dwDSize2;p2.pStatus = &FDStatus2;
		p3 = p1; p3.pdwSize = &dwDSize3;p3.pStatus = &FDStatus3;
		p4 = p1; p4.pdwSize = &dwDSize4;p4.pStatus = &FDStatus4;
		break;

    case 2:
		p1.pSite1 = &pSite[0], p1.pSite2 = &pSite[1]; p1.pControl = &FControl; p1.pdwSize = &dwDSize1;p1.pStatus = &FDStatus1;
		p2 = p1; p2.pdwSize = &dwDSize2;p2.pStatus = &FDStatus2;
		p3 = p1; p3.pdwSize = &dwDSize3;p3.pStatus = &FDStatus3;
		p4 = p1; p4.pdwSize = &dwDSize4;p4.pStatus = &FDStatus4;
		break;
	case 3:
		p1.pSite1 = &pSite[0], p1.pSite2 = &pSite[2]; p1.pControl = &FControl; p1.pdwSize = &dwDSize1;p1.pStatus = &FDStatus1;
		p2.pSite1 = &pSite[1], p2.pSite2 = &pSite[2]; p2.pControl = &FControl; p2.pdwSize = &dwDSize2;p2.pStatus = &FDStatus2;
		p3 = p1; p3.pdwSize = &dwDSize3;p3.pStatus = &FDStatus3;
		p4 = p2; p4.pdwSize = &dwDSize4;p4.pStatus = &FDStatus4;
		break;
	case 4:
		p1.pSite1 = &pSite[0], p1.pSite2 = &pSite[3]; p1.pControl = &FControl; p1.pdwSize = &dwDSize1;p1.pStatus = &FDStatus1;
		p2.pSite1 = &pSite[1], p2.pSite2 = &pSite[3]; p2.pControl = &FControl; p2.pdwSize = &dwDSize2;p2.pStatus = &FDStatus2;
		p3.pSite1 = &pSite[2], p3.pSite2 = &pSite[3]; p3.pControl = &FControl; p3.pdwSize = &dwDSize3;p3.pStatus = &FDStatus3;
		p4 = p1; p4.pdwSize = &dwDSize4;p4.pStatus = &FDStatus4;
		break;
    case 5:
		p1.pSite1 = &pSite[0], p1.pSite2 = &pSite[4]; p1.pControl = &FControl; p1.pdwSize = &dwDSize1;p1.pStatus = &FDStatus1;
		p2.pSite1 = &pSite[1], p2.pSite2 = &pSite[4]; p2.pControl = &FControl; p2.pdwSize = &dwDSize2;p2.pStatus = &FDStatus2;
		p3.pSite1 = &pSite[2], p3.pSite2 = &pSite[4]; p3.pControl = &FControl; p3.pdwSize = &dwDSize3;p3.pStatus = &FDStatus3;
		p4.pSite1 = &pSite[3], p4.pSite2 = &pSite[4]; p4.pControl = &FControl; p4.pdwSize = &dwDSize4;p4.pStatus = &FDStatus4;
		break;  
    }
    FStatus = 1;
	p1.pThreadCnt = &ThreadCount;
	p2.pThreadCnt = &ThreadCount;
	p3.pThreadCnt = &ThreadCount;
	p4.pThreadCnt = &ThreadCount;

    pDownThread[0] = new TRecvThread(false, &p1);
    pDownThread[1] = new TRecvThread(false, &p2);
    pDownThread[2] = new TRecvThread(false, &p3);
    pDownThread[3] = new TRecvThread(false, &p4);/**/
	pDownThread[0]->Run();
	pDownThread[1]->Run();
	pDownThread[2]->Run();
	pDownThread[3]->Run(); /**/
}
//---------------------------------------------------------------------------

void TDownloader::StopDownload()
{
    if(FStatus != 1) return;
    FStatus = 0;
    FControl = 0;

    while(ThreadCount > 0)
    {
        //Sleep(100);
       sleep(100 * 1000);
    }  
	for(int i=0; i<5; ++i) 
	{
		if(pDownThread[i] != NULL)
		{
			delete pDownThread[i];
			pDownThread[i] = NULL;
		}
	}
}
//---------------------------------------------------------------------------

int TDownloader::GetStatus()
{
    if(FStatus == 5) return 5;

	return (FDStatus1 + FDStatus2 + FDStatus3 + FDStatus4) == 16 ? (FStatus = 0,4) : 1;
}
//---------------------------------------------------------------------------

DWORD TDownloader::GetSize()  
{
	return dwDSize1 + dwDSize2 + dwDSize3 + dwDSize4;
}
//---------------------------------------------------------------------------
DWORD TDownloader::GetUpSize()
{
	return dwUSize1 + dwUSize2 + dwUSize3 + dwUSize4;
}
//---------------------------------------------------------------------------
int TDownloader::GetUpStatus()
{
	if(FStatus == 5) return 5;

	return (FUStatus1 + FUStatus2 + FUStatus3 + FUStatus4) == 16 ? (FStatus = 0,4) : 1;
}
//---------------------------------------------------------------------------
void TDownloader::StartWebTest(std::vector<std::wstring> &urlList, int itemCnt)
{
	printf("StartWebTest begin,%d\n",itemCnt);
	FControl = 1;
	FStatus  = 1;
	for(int i=0; i<itemCnt; ++i)
	{
		std::string strtmp;
		WebItems[i].Index = i;
		w2c(urlList.at(i).c_str(),strtmp);
		printf("StartWebTest:%s\n",strtmp.c_str());
		pthread[i] = new TWebThread(urlList.at(i).c_str(), &WebItems[i]);
		if(pthread[i] != NULL) 
		{
			pthread[i]->Run();
		}
	}
}
//---------------------------------------------------------------------------
void TDownloader::StopWebTest()
{
	if (FStatus != 1) return;
	FStatus = 0;
	FControl = 0;
	
	for (int i=0; i<10; ++i)
	{
		if(pthread[i] != NULL)
		{
			delete pthread[i];
			pthread[i] = NULL;
		}
	} 
}
//---------------------------------------------------------------------------
void TDownloader::StopWebThread(int indx)
{
	if ((indx < 10) && (pthread[indx] != NULL))
	{
		pthread[indx]->Terminate();
	}
}
//---------------------------------------------------------------------------
TWEBINFO * TDownloader::GetWebTestItem(int indx)
{
	if (indx >= 10) return NULL;
	return &WebItems[indx];
}
//---------------------------------------------------------------------------

TWebThread::TWebThread(std::wstring webPage, TWEBINFO *pInfo) 
    : TCThread(),pWebInfo(pInfo)
{
	printf("TWebThread struct begin\n");
	Url = webPage;
}
//---------------------------------------------------------------------------

TWebThread::~TWebThread()    
{
}
//---------------------------------------------------------------------------

void TWebThread::Execute()    
{
	printf("%s begin\n",__FUNCTION__);
	
    CXbHttp http;
    http.SetFlushingMode();
	if(http.Get(Url.c_str()) != 0)
	{
		pWebInfo->Status = 4;
	}
	else
	{
		//printf("web content \n%s\n", http.GetRecvWebContent());
		pWebInfo->Status = 3;
		strcpy(pWebInfo->IP,http.GetServerIp());
		pWebInfo->ResponceTime = http.GetConnectTime();
		pWebInfo->WebPageSize = http.GetTotalSize();
		pWebInfo->LoadTime = http.GetLoadPageTime();
		pWebInfo->DNSTime = http.GetDnsTime();
		pWebInfo->FstPkgTime = http.GetFstPkgTime();
	}
}
//---------------------------------------------------------------------------
void TWebThread::Run()
{
	printf("%s begin\n",__FUNCTION__);
	if(start() != 0)
	{
		TRACE_TEXT("TWebThread run failed\n");
	}
}