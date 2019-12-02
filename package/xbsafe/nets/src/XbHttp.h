#ifndef XBHTTP_H
#define XBHTTP_H

#include <wchar.h>
#include <string>
#include "Common.h"

#pragma pack(push,1)
typedef struct _WebPageTestResult{
	DWORD LoadInSize;//加载字节数
	UINT  HttpCd; //测试网页的返回码
	double LoadTime;//网页加载时间
	double DnsTime;//域名解析的时间
	double ConnTime;//连接时间
	double FstPckTime;//首包时间
	double RecvDataTime;//接收数据时间
}WebPageTestResult;
#pragma pack(pop)

class CXbHttp
{
public:
	CXbHttp(void);
	~CXbHttp(void);
public:
	//设置访问超时的毫秒数
	void SetTimeout(int Interval = 15000);

	SOCKET  m_sSocket;
	
	//指定ip地址测试的时候使用
	void SetAddrInfo(const wchar_t *pIp,const unsigned short port);

	// 不保存数据。
	void SetFlushingMode();

	//-1表示失败，0表示成功
	int Get(const wchar_t *pwszUrl);
	//-1表示失败，0表示成功
	int Post(const wchar_t *pwszUrl,const char *pPostData);

	const char *GetRecvWebContent();
	const char *GetHttpRecvHeader();
	const char *GetServerIp();

	DWORD GetTotalSize();
	DWORD GetLoadSizeInBytes();
	double GetLoadPageTime();
	double GetDnsTime();
	double GetConnectTime();
	double GetFstPkgTime();
	//-1表示api socket_error或者连接超时，非负值为响应时间
	double GetTcpResponceTime(const wchar_t *pServerHost, const unsigned short port = 80);

	int ParseUrltoDomainName(const wchar_t *pUrl);
	bool WaitForWrite();
	bool WaitForRead();
	bool CreateSocket();
	int SendData(const char *pbuf,DWORD dwsize);
	int RecvData(BYTE **pbuf,DWORD *pDwsize); //需要调用者删除收到的buf，delete [] buf;
	int FlushingRecvData(DWORD *pDwsize);
	void DisConnect();
	
	void GetTestResult(WebPageTestResult *pResult);
private:
	bool Send(const char *pPostData,int type);
	bool RecvData_();
	int  RequestData(const wchar_t *pwszUrl,const char *pPostData,int type);
	void ProcessTransferEncodeChunked();
	bool GetHTTPRspHeader();
	bool GetHTTPRspCode();
	DWORD FormatRequestHeader(const char *pServer,const char *pObject,DWORD dwcontentlen = 0,int itype = 0);//itype 0为get，1为post
private:
	int     m_iTimeout;
	bool m_FlushingMode;

	std::wstring m_strUrl;
	std::wstring m_strUrlObj;
	std::wstring m_strSvrDns;
	
	std::string m_strSvrIP;
	std::string m_StrReqBody;
	std::string m_strRspHdr;
	std::string m_requestheader;
	UINT m_uPort;
	UINT m_uHttpCd;
	UINT m_TotalSize;
	double m_loadTime;
	double m_dnsTime;
	double m_connTime;
	double m_fstpacketTime;
	double m_recvdataTime;
};


#endif