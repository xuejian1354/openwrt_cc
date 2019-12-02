//g++ Common.cpp -o common -L. -lrt -Wall
#include <stdio.h>
#include <wchar.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <wctype.h>
//#include "DES.h"
//#include "ZjyClass.h"

#include "Common.h"

CCommonInit CommonInit;

CCommonInit::CCommonInit()
{
	::srand((int)time(NULL));     //每次执行种子不同，生成不同的随机数
}
CCommonInit::~CCommonInit()
{

}
int CCommonInit::GetRand(int nSeed)
{
	if (nSeed == 0)
	{
		return 0;
	}
	return rand() % nSeed;
}

int CCommonInit::GetRand(int LSeed, int RSeed)
{
	int pos, dis;
	if(LSeed == RSeed)
	{
		return LSeed;
	}
	else if(LSeed > RSeed)
	{
		pos = RSeed;
		dis = LSeed - RSeed + 1;
		return rand() % dis + pos;
	}
	else
	{
		pos = LSeed;
		dis = RSeed - LSeed + 1;
		return rand() % dis + pos;
	}
}

//查找源串中目标串的位置 如果找到返回值 >=0，否则返回-1;   (区分大小写)
int strpos(const char *pSrc, const char *pDst)
{
	int iRet = -1;
	size_t sLen = strlen(pSrc);
	size_t dLen = strlen(pDst);
	if(dLen == 0) return -1;

	int temp = sLen - dLen;
	if (temp < 0)
	{
		return -1;
	}

	size_t val = sLen - dLen + 1;
	const char *ps = pSrc;
	const char *pd = pDst;

	for(size_t i=0; i<val; i++)
	{
		if(memcmp(ps, pd, dLen) == 0)
		{
			iRet = (int)i;
			break;
		}

		ps++;
	}

	return iRet;
}
//---------------------------------------------------------------------------

//查找源串中目标串的位置 如果找到返回值 >=0，否则返回-1;   （不区分大小写)
int stripos(const char *pSrc,const char *pDst)
{
	int iRet = -1;
	size_t sLen = strlen(pSrc);
	size_t dLen = strlen(pDst);
	if(dLen == 0) return -1;

	int temp = sLen - dLen;
	if (temp < 0)
	{
		return -1;
	}
	size_t val = sLen - dLen + 1;
	const char *ps = pSrc;
	const char *pd = pDst;

	for(size_t i=0; i<val; i++)
	{
		if(strncasecmp(ps, pd, dLen) == 0)
		{
			iRet = (int)i;
			break;
		}

		ps++;
	}

	return iRet;
}
//---------------------------------------------------------------------------

//查找源串中目标串的位置 如果找到返回值 >=0，否则返回-1;   (区分大小写)
int wstrpos(const wchar_t *pSrc, const wchar_t *pDst)
{
	int iRet = -1;
	size_t sLen = wcslen(pSrc);
	size_t dLen = wcslen(pDst);
	if(dLen == 0) return -1;
	int temp = sLen - dLen;
	if (temp < 0)
	{
		return -1;
	}
	size_t val = sLen - dLen + 1;
	const wchar_t *ps = pSrc;
	const wchar_t *pd = pDst;

	for(size_t i=0; i<val; i++)
	{
		if(wmemcmp(ps, pd, dLen) == 0)
		{
			iRet = (int)i;
			break;
		}

		ps++;
	}

	return iRet;
}
//---------------------------------------------------------------------------

//查找源串中目标串的位置 如果找到返回值 >=0，否则返回-1;   （不区分大小写)
int wstripos(const wchar_t *pSrc,const wchar_t *pDst)
{	
	int iRet = -1;
	size_t sLen = wcslen(pSrc);
	size_t dLen = wcslen(pDst);
	//printf("%s,%u,%u begin\n",__FUNCTION__,sLen,dLen);
	if(dLen == 0) return -1;
	int temp = sLen - dLen;
	if (temp < 0)
	{
		return -1;
	}
	size_t val = sLen - dLen + 1;
	const wchar_t *ps = pSrc;
	const wchar_t *pd = pDst;
	
	for(size_t i=0; i<val; i++)
	{
		if(wcsncasecmp(ps, pd, dLen) == 0)
		{
			iRet = (int)i;
			break;
		}

		ps++;
	}

	return iRet;
}
//---------------------------------------------------------------------------

//ASCII字符串快速比较，忽略大小写
size_t strnicmp_fast(char *pSrc, char *pDst, size_t Len)
{
	if (Len == 0) return 0;

	// L'a' - L'A' = 32 = 00100000B, 这难道是巧合??    
	unsigned short mask = ~(L'a' - L'A');
	char chs = *pSrc;
	char chd = *pDst;

	do
	{
		if(((chs | chd) & 0x80) == 0)
		{
			if(((chs ^ chd) & mask) != 0) break;
		}
		else
		{
			if(chs != chd) break;
		}

		chs = *(++pSrc);
		chd = *(++pDst);
	}while((--Len) && chs);

	// 返回差值，如果相等，结果为0
	return (chs - chd);
}
//---------------------------------------------------------------------------

//宽字符串快速比较，忽略大小写
size_t wcsnicmp_fast (wchar_t *pSrc, wchar_t *pDst, size_t Len)
{                        
	if (Len == 0) return 0;  

	unsigned short mask = ~(L'a' - L'A');  
	unsigned short wcmask = 0xFF80;

	wchar_t wchs = *pSrc;
	wchar_t wchd = *pDst;

	do
	{                            
		if (((wchs | wchd) & wcmask) == 0)      //E文字符 < 0x80
		{   
			if (((wchs ^ wchd) & mask) != 0 ) break;
		}

		// 非英文字符直接对比    
		else 
		{
			if (wchs != wchd)  break;
		}

		wchs = *(++pSrc);
		wchd = *(++pDst);   
	}
	while ((--Len) && wchs);

	//Len == 0, 相同
	if(Len == 0) return 0;

	// 返回差值，如果相等，结果为0
	return (wchs - wchd);
}
//---------------------------------------------------------------------------

//产生 0 - aValue 之间的随机整数
int Random(int aValue)
{
	return CommonInit.GetRand(aValue);
}

//产生 m - n 之间的随机整数
int Random(int mValue,int nValue)
{
	return CommonInit.GetRand(mValue,nValue);
}

//---------------------------------------------------------------------------

void URLEncode(wchar_t *pzUrl, int MaxChars)
{
	TAutoMem Mem1((MaxChars + 1) * sizeof(wchar_t));   
	wchar_t *url = (wchar_t*)Mem1.GetBuffer();
	wcscpy(url, pzUrl);

	int len = (int)wcslen(url);
	wmemset(pzUrl, 0, MaxChars);

	int idx = 0;
	for (int i = 0; i < len; ++i)
	{
		switch (url[i])
		{
		case ' ':
			wcscat(pzUrl, L"%20");
			idx += 3;
			break;
		case '+':
			wcscat(pzUrl, L"%2b");
			idx += 3;
			break;
		case '\'':
			wcscat(pzUrl, L"%27");
			idx += 3;
			break;
		case '/':
			wcscat(pzUrl, L"%2F");
			idx += 3;
			break;
		case '.':
			wcscat(pzUrl, L"%2E");
			idx += 3;
			break;
		case '<':
			wcscat(pzUrl, L"%3c");
			idx += 3;
			break;
		case '>':
			wcscat(pzUrl, L"%3e");
			idx += 3;
			break;
		case '#':
			wcscat(pzUrl, L"%23");
			idx += 3;
			break;
		case '%':
			wcscat(pzUrl, L"%25");
			idx += 3;
			break;
		case '&':
			wcscat(pzUrl, L"%26");
			idx += 3;
			break;
		case '{':
			wcscat(pzUrl, L"%7b");
			idx += 3;
			break;
		case '}':
			wcscat(pzUrl, L"%7d");
			idx += 3;
			break;
		case '\\':
			wcscat(pzUrl, L"%5c");
			idx += 3;
			break;
		case '^':
			wcscat(pzUrl, L"%5e");
			idx += 3;
			break;
		case '~':
			wcscat(pzUrl, L"%73");
			idx += 3;
			break;
		case '[':
			wcscat(pzUrl, L"%5b");
			idx += 3;
			break;
		case ']':
			wcscat(pzUrl, L"%5d");
			idx += 3;
			break;

		default:
			pzUrl[idx] = url[i];
			idx++;
			break;
		}
		pzUrl[idx] = 0;
	}
}
//---------------------------------------------------------------------------

//去除返回字符串中非法字符
void DelChars(wchar_t *pzSrc)
{
	wchar_t *p1 = pzSrc;
	wchar_t *p2 = pzSrc;

	while(*p2)
	{
		switch(*p2)
		{
		case L' ':
		case L'\b':
		case L'\r':
		case L'\n':
		case L'\t':
		case L'\a':
			break;

		default:
			*p1 = *p2;
			p1++;
			break;  
		} 

		p2++;
	}

	*p1 = 0; 
}
//---------------------------------------------------------------------------

//去除返回字符串中非法字符
void DelChars(char *pzSrc)
{
	char *p1 = pzSrc;
	char *p2 = pzSrc;

	while(*p2)
	{
		switch(*p2)
		{
		case ' ':
		case '\b':
		case '\r':
		case '\n':
		case '\t':
		case '\a':
			break;

		default:
			*p1 = *p2;
			p1++;
			break;  
		} 

		p2++;
	}

	*p1 = 0; 
}
//---------------------------------------------------------------------------
std::string TrimEndLine(const char* pCode)
{
	std::string strValue ="";
	int nLength = strlen( pCode );

	if( nLength <= 0 )
	{
		return strValue;
	}  

	char *pBuffer = new char[nLength + 1];
	memset( pBuffer, 0, nLength + 1);
	int i = 0;

	int nCount = 0;

	while(  i < nLength )
	{
		if ( (0x20 == pCode[i])||('\r' == pCode[i])||('\t' == pCode[i])||('\n' == pCode[i] ))
		{
			i++;
			continue;
		}

		pBuffer[nCount] = pCode[i];
		++nCount;
		++i;
	}

	strValue = pBuffer;
	delete [] pBuffer;

	return strValue;
}
//---------------------------------------------------------------------------
//取得 <name>...</name>之间的内容
std::wstring GetXmlNodeValue(const wchar_t *pwzXmlTxt, const wchar_t *pNodeName)
{
	std::wstring sRet;
	wchar_t sNode[33] = {0};    //此处节名不可超过32字符    
	swprintf(sNode, 33, L"<%ls>", pNodeName);

	for(int i=0; i<1; i++)
	{
		const wchar_t *p1 = pwzXmlTxt;
		int x = wstripos(p1, sNode);
		if(x < 0) 
		{
			printf("%s,wstripos1\n",__FUNCTION__);
			break;
		}

		p1 += x;
		p1 += wcslen(sNode);

		swprintf(sNode, 33, L"</%ls>", pNodeName);  
		x = wstripos(p1, sNode);
		if(x < 0) 
		{
			printf("%s,wstripos1\n",__FUNCTION__);
			break;
		}

		for(int j=0; j<x; j++)
		{
			sRet += p1[j];
		}
	}

	return sRet;
}
//---------------------------------------------------------------------------

//取得一个字符型属性值
std::wstring GetXmlAttributeSValue(const wchar_t *pwzXmlTxt, const wchar_t *pAttribName)
{	
	//printf("%s begin\n",__FUNCTION__);
	std::wstring sRet;
	wchar_t sName[33] = {0};    //此处值名不可超过32字符
	swprintf(sName,33 , L"%ls=\"", pAttribName);
	
	//printf("%s swprintf\n",__FUNCTION__);
	
	//std::string strtmp;
	//w2c(sName,strtmp);
	//printf("%s,%s begin\n",__FUNCTION__,strtmp.c_str());
	
	for(int i=0; i<1; i++)
	{
		const wchar_t *p1 = pwzXmlTxt;
		int x = wstripos(p1, sName);
		if(x < 0) break;

		p1 += x;
		p1 += wcslen(sName);
		while(*p1)
		{
			if(*p1 == L'\"') break;
			sRet += *p1++;
		} 
	}

	return sRet;
}
//---------------------------------------------------------------------------

//取得一个整型属性值
int GetXmlAttributeIValue(const wchar_t *pwzXmlTxt, const wchar_t *pAttribName)
{
	std::wstring sRet;
	wchar_t sName[33] = {0};    //此处值名不可超过32字符
	swprintf(sName,33, L"%ls=\"", pAttribName);

	for(int i=0; i<1; i++)
	{
		const wchar_t *p1 = pwzXmlTxt;
		int x = wstripos(p1, sName);
		if(x < 0) break;

		p1 += x;
		p1 += wcslen(sName);
		while(*p1)
		{
			if((*p1 < L'0') || (*p1 > L'9')) break;
			sRet += *p1++;
		} 
	}
	std::string cov;
	w2c(sRet.c_str(),cov);
	
	return atoi(cov.c_str());
}
//---------------------------------------------------------------------------
int GetXmlAttributeIValue(const char *pzXmlTxt,const  char *pAttribName)
{
	std::string sRet;
	char sName[33] = {0};    //此处值名不可超过32字符
	sprintf(sName, "%s=\"", pAttribName);

	for(int i=0; i<1; i++)
	{
		const char *p1 = pzXmlTxt;
		int x = stripos(p1, sName);
		if(x < 0) break;

		p1 += x;
		p1 += strlen(sName);
		while(*p1)
		{
			if((*p1 < L'0') || (*p1 > L'9')) break;
			sRet += *p1++;
		} 
	}

	return atoi(sRet.c_str());
}
//---------------------------------------------------------------------------

std::string GetXmlNodeValue(const char *pwzXmlTxt, const char *pNodeName)
{
	std::string sRet;
	char sNode[33] = {0};    //此处节名不可超过32字符    
	sprintf(sNode, "<%s>", pNodeName);

	for(int i=0; i<1; i++)
	{
		const char *p1 = pwzXmlTxt;
		int x = stripos(p1, sNode);
		if(x < 0) break;

		p1 += x;
		p1 += strlen(sNode);

		sprintf(sNode, "</%s>", pNodeName);  
		x = stripos(p1, sNode);
		if(x < 0) break;

		for(int j=0; j<x; j++)
		{
			sRet += p1[j];
		}
	}

	return sRet;
}
//---------------------------------------------------------------------------
//取得一个字符型属性值
std::string GetXmlAttributeSValue(const char *pwzXmlTxt, const char *pAttribName)
{
	std::string sRet;
	char sName[33] = {0};    //此处值名不可超过32字符
	sprintf(sName, "%s=\"", pAttribName);

	for(int i=0; i<1; i++)
	{
		const char *p1 = pwzXmlTxt;
		int x = stripos(p1, sName);
		if(x < 0) break;

		p1 += x;
		p1 += strlen(sName);
		while(*p1)
		{
			if(*p1 == '\"') break;
			sRet += *p1++;
		} 
	}

	return sRet;
}

//检查是不是有效的IP
BOOL IsIPV4(wchar_t *pwzAddr)
{
	int i1 = 0, i2 = 0, i3 = 0, i4 = 0;
	int x = swscanf(pwzAddr, L"%d.%d.%d.%d", &i1, &i2, &i3, &i4); 
	if(x != 4) return FALSE;

	if((i1 < 0 || i1 > 255) || (i2 < 0 || i2 > 255) ||
		(i3 < 0 || i3 > 255) || (i4 < 0 || i4 > 255)) return FALSE;

	return TRUE;
}  

BOOL IsIPV4(char *pzAddr)
{
	int i1 = 0, i2 = 0, i3 = 0, i4 = 0;
	int x = sscanf(pzAddr, "%d.%d.%d.%d", &i1, &i2, &i3, &i4); 
	if(x != 4) return FALSE;

	if((i1 < 0 || i1 > 255) || (i2 < 0 || i2 > 255) ||
		(i3 < 0 || i3 > 255) || (i4 < 0 || i4 > 255)) return FALSE;

	return TRUE;
}
//---------------------------------------------------------------------------

//截取字符
const char* strncut(char*strDest,char*strSource,int iStartPos,int iNum) 
{
	for(int i=0;i<iNum;i++) 
	{
		strDest[i]= strSource[iStartPos++]; 
	}
	strDest[iNum]='\0'; 
	return strDest; 
}

bool w2c(const wchar_t *pwstr,std::string &pcstr)
{
	//printf("%s\n",__FUNCTION__);
	bool bRet = false;
	if (pwstr == NULL)
	{
		pcstr = "";
		return bRet;
	}

	int len = 0;
	w2c(pwstr,NULL, &len);
	len += 1;
	//printf("%s 1,%d\n",__FUNCTION__,len);
	TAutoMem mem(len);
	//printf("%s,will into base w2c\n",__FUNCTION__);
	
	bRet = w2c(pwstr,(char *)mem.GetBuffer(),&len);
	//printf("%s 2,%d\n",__FUNCTION__,len);
	pcstr = (char *)mem.GetBuffer();

	return bRet;	
}

bool w2c(const wchar_t *wstr, char *pstr, int *nsize)
{
//	printf("%s base begin\n",__FUNCTION__);
	bool bRet = false;
	int wcLen;
	
	if (wstr == NULL || nsize == NULL) 
		return bRet;

	wcLen = wcstombs( NULL,wstr,0);
	if(wcLen == -1) return bRet;
	
	if (pstr == NULL)
	{
		*nsize = wcLen;
	}else
	{
		if (wcLen > *nsize)
		{
			*nsize = wcLen;
			return bRet;
		}
	//	printf("%s wcstombs begin\n",__FUNCTION__);
		*nsize = wcstombs( pstr, wstr, wcslen(wstr)+1);  
//		printf("%s wcstombs af\n",__FUNCTION__);
	}
	
	bRet = true;

	return bRet;
}

bool c2w(const char *pstr,std::wstring &pwstr)
{
	bool bRet = false;
	if (pstr == NULL)
	{
		pwstr = L"";
		return bRet;
	}
	//printf("%s\n",__FUNCTION__);
	
	int len = 0;
	c2w(pstr,NULL, &len);
	//printf("%s,len:%d\n",__FUNCTION__,len);
	len += 1;
	TAutoMem mem(len * sizeof(wchar_t));
	bRet = c2w(pstr,(wchar_t *)mem.GetBuffer(),&len);
	wchar_t *ptmp = (wchar_t *)mem.GetBuffer();
	//printf("%s,len:%d GetBuffer before\n",__FUNCTION__,len);
	if(ptmp != NULL)
	{
		//printf("%s,append before\n",__FUNCTION__);
		pwstr.append(ptmp);
	}
	//printf("%s,len:%d finish\n",__FUNCTION__,len);
	
	return bRet;
}
bool c2w(const char *str, wchar_t *pwstr, int *nsize)
{
	bool bRet = false;
	int wcLen;
	
//	printf("%s,%d base begin\n",__FUNCTION__,*nsize);
	
	if (str == NULL || nsize == NULL) return bRet;

	wcLen = mbstowcs(NULL,str, 0);
	if(wcLen == -1) return bRet;
	
	if (pwstr == NULL)
	{
		*nsize = wcLen;
	}else
	{
		//不需要==因为返回的大小已经包含了后面的结尾符号
		if (wcLen > *nsize)
		{
			*nsize = wcLen;
			return bRet;
		}
	//	printf("%s mbstowcs before\n",__FUNCTION__);
		*nsize = mbstowcs(pwstr,str,strlen(str)+1);    
	//	printf("%s mbstowcs after\n",__FUNCTION__);
	}
	//printf("%s,%d base finish\n",__FUNCTION__,*nsize);
	bRet = true;

	return bRet;
}

bool UTF8ToUnicode(const char *pUtf8,std::wstring &strUnicodeDes)
{
	bool bRet = false;
	int wcLen;
	strUnicodeDes.clear();

	if (pUtf8 == NULL)
	{
		strUnicodeDes = L"";
		return bRet;
	}

	wcLen =(size_t)mbstowcs(NULL, pUtf8, 0);
	if(wcLen == -1) return bRet;
	
	wchar_t *pBuffer = NULL;
	pBuffer = new wchar_t[wcLen + 1];
	if (pBuffer)
	{
		memset(pBuffer,0,wcLen + 1);
		mbstowcs(pBuffer, pUtf8, strlen(pUtf8)+1);    
		strUnicodeDes.append(pBuffer);

		delete []pBuffer;
		bRet = true;
	}

	return bRet;
}

bool UnicodeToUTF8(const wchar_t *pUnicode,std::string &strUtf8Des)
{
	bool bRet = false;
	int nbytes;
	strUtf8Des.clear();

	if (pUnicode == NULL)
	{
		strUtf8Des = "";
		return bRet;
	}

	nbytes = wcstombs(NULL,pUnicode,0);
	if(nbytes == -1) return bRet;
	
	char * pBuffer = NULL;
	pBuffer = new char[nbytes + 1];
	if (pBuffer)
	{
		wcstombs( pBuffer, pUnicode, wcslen(pUnicode) +1 );
		strUtf8Des.append(pBuffer);

		delete []pBuffer;
		bRet = true;
	}

	return bRet;
}

bool ParseUrl(const wchar_t*strWeb,std::wstring &ustrHost,std::wstring &ustrObj,int &nPort)
{
	bool bRet = false;
	if(!strWeb  || (wcslen(strWeb) == 0))
	{
		return bRet;
	}
	
	nPort = 80;

	std::wstring strServerWeb;
	std::wstring stempsrc = strWeb;
	MyToLower(stempsrc);
	size_t idx = stempsrc.find(L"http://");
	if (idx != std::wstring::npos) 
	{
		stempsrc = strWeb;
		stempsrc = stempsrc.substr(idx + 7);
	}else
	{
		idx = stempsrc.find(L"https://");
		stempsrc = strWeb;
		if (idx != std::wstring::npos)
		{
			nPort = 443;
			stempsrc = stempsrc.substr(idx + 8);
		}
	}

	strServerWeb = stempsrc;

	idx = strServerWeb.find(L"/");
	if(idx != std::wstring::npos)
	{
		ustrHost = strServerWeb.substr(0, idx);
		ustrObj = strServerWeb.substr(idx);
	}else
	{
		ustrHost =  strServerWeb;
		ustrObj = L"/";
	}

	if(!ustrHost.empty())
	{
		idx = ustrHost.find(L":");
		if(idx != std::wstring::npos)
		{
			std::string cov;
			w2c(ustrHost.substr(idx + 1).c_str(),cov);
			nPort = atoi(cov.c_str());
			ustrHost = ustrHost.substr(0,idx);
		}
		bRet = true;
	}
	return bRet;
}

bool ParseUrl(const char*strWeb,std::string &ustrHost,std::string &ustrObj,int &nPort)
{
	bool bRet = false;
	if(!strWeb  || (strlen(strWeb) == 0))
	{
		return bRet;
	}
	
	nPort = 80;

	std::string strServerWeb;
	std::string stempsrc = strWeb;
	MyToLower(stempsrc);
	size_t idx = stempsrc.find("http://");
	if (idx != std::string::npos) 
	{
		stempsrc = strWeb;
		stempsrc = stempsrc.substr(idx + 7);
	}else
	{
		idx = stempsrc.find("https://");
		stempsrc = strWeb;
		if (idx != std::string::npos)
		{
			nPort = 443;
			stempsrc = stempsrc.substr(idx + 8);
		}
	}

	strServerWeb = stempsrc;

	idx = strServerWeb.find("/");
	if(idx != std::string::npos)
	{
		ustrHost = strServerWeb.substr(0, idx);
		ustrObj = strServerWeb.substr(idx);
	}else
	{
		ustrHost =  strServerWeb;
		ustrObj = "/";
	}

	if(!ustrHost.empty())
	{
		idx = ustrHost.find(":");
		if(idx != std::string::npos)
		{
			std::string cov = ustrHost.substr(idx + 1);
			nPort = atoi(cov.c_str());
			ustrHost = ustrHost.substr(0,idx);
		}
		bRet = true;
	}
	return bRet;
}

//------------------------------------------------------------------------
char * Byte2Hex(BYTE Val)
{
	static char ch[3];

	BYTE b = (Val >> 4);
	if(b > 9) b = ('A' + b - 10);
	else b += 0x30;
	ch[0] = b;

	b = (Val & 0x0F);
	if(b > 9) b = ('A' + b - 10);
	else b += 0x30;
	ch[1] = b;
	ch[2] = 0;

	return ch;
}
//------------------------------------------------------------------------
std::string& MyTrim(std::string& text)
{
	if(!text.empty())
	{
		text.erase(0,text.find_first_not_of((" \n\r\t")));
		text.erase(text.find_last_not_of((" \n\r\t")) + 1);
	}
	return text;
}
//------------------------------------------------------------------------
std::wstring& MyTrim(std::wstring& text)
{
	if(!text.empty())
	{
		text.erase(0,text.find_first_not_of(L" \n\r\t"));
		text.erase(text.find_last_not_of(L" \n\r\t") + 1);
	}
	return text;
}

//WCHAR wideStr[] = L"Abc";
//_wcslwr_s(wideStr, wcslen(wideStr) + 1); // abc
//_wcsupr_s(wideStr, wcslen(wideStr) + 1);// ABC
//------------------------------------------------------------------------
std::wstring& MyToLower(std::wstring& text)
{
	transform(text.begin(), text.end(), text.begin(), towlower);
	return text;
}
//------------------------------------------------------------------------
std::wstring& MyToUpper(std::wstring& text)
{
	transform(text.begin(), text.end(), text.begin(), towupper);
	return text;
}
//------------------------------------------------------------------------
std::string& MyToLower(std::string& text)
{
	transform(text.begin(),text.end(),text.begin(),tolower);
	return text;
}
//------------------------------------------------------------------------
std::string& MyToUpper(std::string& text)
{
	transform(text.begin(), text.end(), text.begin(), toupper);
	return text;
}
// ---------------------------------------------------------------------------
bool Dns(const char *szHost, char *pIp, size_t sIpSize)
{
	bool bret = false;
	bool DnsFlag = false;
	if(szHost == NULL || strlen(szHost) == 0) return false;
	
	if(szHost != NULL && strlen(szHost) != 0)
	{		
		std::string s2 = szHost;
		int idx = s2.length();

		for (int i = 0; i < idx; i++) {
			if (s2[i] != '.' && (s2[i] < '0' || s2[i] > '9')) {
				DnsFlag = true;
				break;
			}
		}
	}	

	if (DnsFlag) {
		struct hostent *he = ::gethostbyname(szHost);
		if(he != NULL)
		{
			BYTE *p = (BYTE *)he->h_addr_list[0];
			sprintf(pIp, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
			bret = true;
		}
	}
	else {
		sprintf(pIp, "%s", szHost);
		bret = true;
	}
	return bret;
}

std::wstring ULToStr(unsigned long nV)
{
	wchar_t buf[100];
	wmemset(buf,0,100);
	swprintf(buf,100,L"%lu",nV);
	return buf;
}

void MyWstrSplit(std::wstring& text,wchar_t *pSplit,std::vector<std::wstring> &vec)
{
	int size = text.length();
	size_t pos = 0;
	for (int i=0; i<size; )
	{
		pos = text.find(pSplit,i);
		if (pos == std::wstring::npos)
		{
			vec.push_back(text.substr(i, size - i));
			break;
		}
		else 
		{
			vec.push_back(text.substr(i, pos - i));
			i = pos + wcslen(pSplit);
		}
	}
}

void MyAstrSplit(std::string& text,char *pSplit,std::vector<std::string> &vec)
{
	int size = text.length();
	unsigned pos = 0;
	for (int i=0; i<size; )
	{
		pos = text.find(pSplit,i);
		if (pos == std::string::npos)
		{
			vec.push_back(text.substr(i, size - i));
			break;
		}
		else
		{
			vec.push_back(text.substr(i, pos - i));
			i = pos + strlen(pSplit);
		}
	}
}

int getCurTime(wchar_t *strTime)
{
	time_t t;
	//char buff[10];
	struct tm *tm = NULL;
	t = time(NULL);
	if(t == -1)
	{
		return -1;
	}
	tm = localtime(&t);
	if(tm == NULL)
	{
		return -1;
	}

	swprintf(strTime ,BUFFSIZE ,L"%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900 , tm->tm_mon + 1, tm->tm_mday , tm->tm_hour , tm->tm_min , tm->tm_sec);
	return 0;
}

void GetSystemTime(std::wstring &startTime)
{
	wchar_t buf[BUFFSIZE];
	wmemset(buf,0,BUFFSIZE);
	getCurTime(buf);
	startTime = buf;
}

int _wtoi(const wchar_t *pvalue)
{
	std::string strtmp;
	w2c(pvalue,strtmp);
	printf("%s:%s\n",__FUNCTION__,strtmp.c_str());
	return atoi(strtmp.c_str());
}

int checkNetLink(const char *host) 
{
	struct hostent *he = gethostbyname(host);
	if ((NULL == he) || (he->h_addr_list[0] == NULL)) {
		printf("h_errno = %d(%s)\n", h_errno, strerror(h_errno));
		return 0;
	}
	return 1;
}
