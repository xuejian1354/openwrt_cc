#pragma once
#include <string>
#include <string.h>
#include "Common.h"

class CNetInfoCheck
{
public:
	CNetInfoCheck(void);
	~CNetInfoCheck(void);
public:
	 /// 判断IP地址是否为内网IP地址
	 BOOL IsInnerIP(const wchar_t *ipAddress);

	 BOOL IsInnerIP(const char *ipAddress);

	 ///check the ip string is legal or not.
	 BOOL IsIPV4(const wchar_t *pwc_Strip);

	 BOOL IsIPV4(const char *pzAddr);

	 /// 把IP地址转换为Long型数字
	 unsigned long GetIpNum(const wchar_t *ipAddress);

	 //获取本机的Mac地址
	 std::wstring GetMyPcMAC();

	 //获取本机的名称
	 BOOL GetMyPcHostName(char *RecvBuf,int len);
	 //--------------------------------------------------------------------------


	 int Dns(wchar_t *wszHost,char *pIp,size_t sIpSize);
	 BOOL GetSockClientIp(SOCKET s,char *ipaddr);//local pc
	 BOOL GetSockServerIp(SOCKET s,char *ipaddr);//remote pc
	 BOOL GetCurIpWithSocket(const char *host, char *pIpAddr);//获取当前用户上网ip 0表示成功，其他表示失败
	 BOOL GetIpListWithWSAioctl(int &icnt);

	 BOOL GetCurUsedIp(char* pipAddr,int iplen);
	 BOOL GetLocalIP(char* pipAddr,int iplen);
private:

	 /// 判断用户IP地址转换为Long型后是否在内网IP地址所在范围
	 BOOL IsInner(unsigned long userIp,unsigned long begin,unsigned long end);

	 ///convert the ip string to unsigned int.
	 BOOL Str2ulongIP(wchar_t *pwc_Strip,unsigned long * puint_ip);
	
};

