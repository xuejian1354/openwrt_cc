
#include "NetInfoCheck.h"
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

CNetInfoCheck::CNetInfoCheck(void)
{
}

CNetInfoCheck::~CNetInfoCheck(void)
{
}

 BOOL CNetInfoCheck::IsInnerIP(const wchar_t * ipAddress)
 {
	 if (!ipAddress)
	 {
		 return FALSE;
	 }
	 //私有IP：还有127这个网段是环回地址   
	 unsigned long aBegin = GetIpNum(L"10.0.0.0");//A类  10.0.0.0
	 unsigned long aEnd	  = GetIpNum(L"10.255.255.255"); //A类  10.255.255.255

	 unsigned long bBegin = GetIpNum(L"172.16.0.0");// B类  172.16.0.0
	 unsigned long bEnd   = GetIpNum(L"172.31.255.255"); // B类  172.31.255.255

	 unsigned long cBegin = GetIpNum(L"192.168.0.0");// C类  192.168.0.0
	 unsigned long cEnd   = GetIpNum(L"192.168.255.255");  // C类  192.168.255.255

	 bool isInnerIp = FALSE;
	 unsigned long ipNum = GetIpNum(ipAddress);

	 std::wstring str;
	 str.append(ipAddress);

	 isInnerIp = IsInner(ipNum, aBegin, aEnd) || IsInner(ipNum, bBegin, bEnd) || IsInner(ipNum, cBegin, cEnd) || (str.find(L"127.") == 0);

	 return isInnerIp;
 }

 BOOL CNetInfoCheck::IsInnerIP(const char * ipAddress)
 {
	 if (!ipAddress || !IsIPV4(ipAddress))
	 {
		 return FALSE;
	 }
	 std::string str = ipAddress;

	 //私有IP：还有127这个网段是环回地址   
	 unsigned long aBegin = GetIpNum(L"10.0.0.0");//A类  10.0.0.0
	 unsigned long aEnd	  = GetIpNum(L"10.255.255.255"); //A类  10.255.255.255

	 unsigned long bBegin = GetIpNum(L"172.16.0.0");// B类  172.16.0.0
	 unsigned long bEnd   = GetIpNum(L"172.31.255.255"); // B类  172.31.255.255

	 unsigned long cBegin = GetIpNum(L"192.168.0.0");// C类  192.168.0.0
	 unsigned long cEnd   = GetIpNum(L"192.168.255.255");  // C类  192.168.255.255

	 bool isInnerIp = FALSE;
	 std::wstring strw;
	 c2w(str.c_str(),strw);
	 unsigned long ipNum = GetIpNum(strw.c_str());
	 
	 isInnerIp = IsInner(ipNum, aBegin, aEnd) || IsInner(ipNum, bBegin, bEnd) || IsInner(ipNum, cBegin, cEnd) || (str.find("127.") == 0);

	 return isInnerIp;
 }

 unsigned long CNetInfoCheck::GetIpNum(const wchar_t * ipAddress)
 {
	 std::wstring wstrIp;

	 if (!ipAddress)
	 {
		 return 0;
	 }
	 wstrIp.append(ipAddress);

	 unsigned long lNumArray[4] = {0};
	 std::string str;
	 if (w2c(ipAddress,str))
	 {
		 size_t j = 0;
		 int count = 0;
		 str += ".";
		 for( size_t i = 0; i < str.length(); i++)
		 {
			 if(str.c_str()[i] == '.')
			 {
				 std::string strTemp = str.substr(j,(i - j));
				 lNumArray[count] = atol(strTemp.c_str());
				 j = i + 1;
				 count++;
			 }
		 }
	 }
	 else
		 return 0;

	 unsigned long ipNum = lNumArray[0] * 256 * 256 * 256 + lNumArray[1] * 256 * 256 + lNumArray[2] * 256 + lNumArray[3];
	 return ipNum;
 }

 BOOL CNetInfoCheck::IsInner(unsigned long userIp, unsigned long begin, unsigned long end)
 {
	 BOOL bRet = (userIp >= begin) && (userIp <= end);
	 return bRet;
 }

 //检查字符串IP是否合法
 //参数：宽字符类型的IP
 BOOL CNetInfoCheck::IsIPV4(const wchar_t *pwc_Strip)
 {
	 //char * pcharBuffer = NULL;
	 if (!pwc_Strip)
	 {
		 return FALSE;
	 }

	 std::string str;
	 int dotCnt = 0;
	 int j = 0;
	 char cIpPart[5] = {0};
	 if (w2c(pwc_Strip,str))
	 {
		 for( size_t i = 0; i < str.length(); i++)
		 {

			 // let's check if all entered char in entered
			 // IP address are digits
			 if(str.c_str()[i] == '.')//判断ip的点分规范
			 {
				 ++dotCnt;
				 int iPart = atoi(cIpPart);
				 if (iPart > 255)
				 {
					 return FALSE;
				 }
				 j = 0;
				 memset(cIpPart,0,5);
				 continue;
			 }
			 if (j >= 3)
			 {
				 return FALSE;
			 }
			 cIpPart[j++] = str.c_str()[i];//将每个ip的.之间的数据记录下来判断是否超过255

			 if(!isdigit(str.c_str()[i]))//判断ip串中是否存在不为数字的字符
			 {
				 return FALSE;
			 }
		 }

		 if (dotCnt == 3)
		 {
			 return TRUE;
		 }
	 }
	 return FALSE;
 }
 
 BOOL CNetInfoCheck::IsIPV4(const char *pzAddr)
 {
	 int i1 = 0, i2 = 0, i3 = 0, i4 = 0;
	 int x = sscanf(pzAddr, "%d.%d.%d.%d", &i1, &i2, &i3, &i4); 
	 if(x != 4) return FALSE;

	 if((i1 < 0 || i1 > 255) || (i2 < 0 || i2 > 255) ||
		 (i3 < 0 || i3 > 255) || (i4 < 0 || i4 > 255)) return FALSE;

	 return TRUE;
 }

 BOOL CNetInfoCheck::Str2ulongIP(wchar_t *pwc_Strip,unsigned long * puint_ip)
 {
	 bool bret = FALSE;
	 std::string str;
	 unsigned long uLongRet = INADDR_NONE;

	 if (!pwc_Strip || !puint_ip)
	 {
		 return FALSE;
	 }

	 if (w2c((const wchar_t *)pwc_Strip,str))
	 {
		 uLongRet = (unsigned long)inet_addr(str.c_str());

		 if(uLongRet == INADDR_NONE)
		 {
			 bret = FALSE;
		 }
		 else
		 {
			 *puint_ip = uLongRet;
			 bret = TRUE;
		 }
	 }
	 return bret;
 }