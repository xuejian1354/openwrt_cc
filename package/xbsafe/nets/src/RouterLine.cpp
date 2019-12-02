// RouterLine.cpp : Defines the exported functions for the DLL application.
//

#include "RouterLine.h"
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <math.h>
#include <sstream>
#include <wchar.h>
#include "tracert.h"
#include "ping.h"
#include "NetInfoCheck.h"

//---------------------------------------------------------------------------

CRouterLineQ::CRouterLineQ()
{
}
//---------------------------------------------------------------------------

CRouterLineQ::~CRouterLineQ()
{
}
//--------------------------------------------------------------------------------------------------
void CRouterLineQ::GetRouterLine()
{
	printf("GetRouterLine begin\n");
	
	int i = 0;
	if (m_testip.empty() || *m_prun != 1)
	{
		return;
	}
	
	std::vector<std::string> m_vectRouter2;	
	std::vector<std::string> m_vectRouter3;
	
	printf("GetRouterLine,ip:%s\n",m_testip.c_str());
	int nnum = 30;
	tracert_result tracerIp[30];
	memset(tracerIp,0,30*sizeof(tracert_result));
	printf("tracert begin\n");

	tracert(m_testip.c_str(),tracerIp,&nnum,m_prun);
	for(i =0;i<nnum; ++i)
	{
		if(strcmp(tracerIp[i].dstIp,"*") != 0)
			m_vectRouterLine1.push_back(tracerIp[i].dstIp);
	}
	
	if (*m_prun != 1)
	{
		return;
	}
	TRACE_TEXT(_T("1 time finish\n"));
	//return ;
	nnum = 30;
	memset(tracerIp,0,30*sizeof(tracert_result));
	tracert(m_testip.c_str(),tracerIp,&nnum,m_prun);
	for(i =0;i<nnum; ++i)
	{
		if(strcmp(tracerIp[i].dstIp,"*") != 0)
			m_vectRouter2.push_back(tracerIp[i].dstIp);
	}
	
	if (!m_vectRouterLine1.empty() && CheckVecIpEqual(m_vectRouterLine1,m_vectRouter2))
	{
		printf("the second equal trace\n");
		return;
	}
	
	if (*m_prun != 1)
	{
		return;
	}
	
	TRACE_TEXT(_T("1,2 is not equal,go on"));
	nnum = 30;
	memset(tracerIp,0,30*sizeof(tracert_result));
	tracert(m_testip.c_str(),tracerIp,&nnum,m_prun);
	for(i =0;i<nnum; ++i)
	{
		if(strcmp(tracerIp[i].dstIp,"*") != 0)
			m_vectRouter3.push_back(tracerIp[i].dstIp);
	}
	if (!m_vectRouterLine1.empty() && !m_vectRouter3.empty() && CheckVecIpEqual(m_vectRouterLine1,m_vectRouter3))
	{
		printf("the 1=3 equal trace\n");
		return;
	}else if(!m_vectRouter2.empty() && !m_vectRouter3.empty() && CheckVecIpEqual(m_vectRouter2,m_vectRouter3))
	{
		printf("the 2=3 equal trace\n");
		m_vectRouterLine1.clear();
		m_vectRouterLine1.swap(m_vectRouter3);
	}else
	{
		int x = m_vectRouterLine1.size();
		int y = m_vectRouter2.size();
		int z = m_vectRouter3.size();
		if(y >= x && y >= z)
		{
			printf("the 2 has the max ip\n");
			m_vectRouterLine1.clear();
			m_vectRouterLine1.swap(m_vectRouter2);
		}else if(z >= x && z >= y)
		{
			printf("the 3 has the max ip\n");
			m_vectRouterLine1.clear();
			m_vectRouterLine1.swap(m_vectRouter3);
		}
	}
	printf("trace finish,%d\n",m_vectRouterLine1.size());
}

BOOL CRouterLineQ::CheckVecIpEqual(std::vector<std::string> ipv1,std::vector<std::string> ipv2)
{
	if (ipv2.size() != ipv1.size())
	{
		return FALSE;
	}
	for (size_t i=0;i<ipv1.size();++i)
	{
		if(ipv1[i].compare(ipv2[i]) != 0)
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL CRouterLineQ::PingSpecRouter(const char *pHost,int nSeq)
{
	printf("%s begin\n",__FUNCTION__);
	
	LINETESTRESULT lr;
	lr.funcId = ENUM_ROUTE_TRACER;
	lr.nmin = LINETESTRESULT::INT_LR_VERYLARGE_VALUE;
	lr.nmax = LINETESTRESULT::INT_LR_INVALID_VALUE;
	memcpy(lr.strip,pHost,MAX_PATH - 1);
	lr.nSend = m_pingtms;
	lr.ninterval = m_pingInterval > 0 ? m_pingInterval : 0;
	lr.nIndx = nSeq;
	
	static_ping_result stResult;
	memset(&stResult,0,sizeof(static_ping_result));
	printf("%s ping(%d,%d) begin,cnt:%d,inter:%d\n",__FUNCTION__,nSeq,*m_prun,m_pingtms,m_pingInterval);
	ping(pHost,m_pingtms,m_pingInterval,&stResult,m_prun);
	
	lr.navg = stResult.avgtime;
	lr.nmax = stResult.maxtime;
	lr.nmin = stResult.mintime;
	lr.ndx = stResult.mvari;
	m_vecResult.push_back(lr);
	return TRUE;
}

BOOL CRouterLineQ::TestLineRoutes()
{
	printf("%s\n",__FUNCTION__);
	if (m_vectRouterLine1.size() > 0)
	{
		int i = 1;
		std::vector<std::string>::iterator it = m_vectRouterLine1.begin();
		//wchar_t wip[33];
		//int len = 33;
		CNetInfoCheck NetInfoCheck;
		while(it != m_vectRouterLine1.end())
		{
			if (*m_prun != 1)
			{
				break;
			}
			printf("TestLineRoutes:%d,%s\n",i,it->c_str());
			//wmemset(wip,0,33);
			//len = 33;
			//c2w(it->c_str(),wip,&len);
			/*if (NetInfoCheck.IsInnerIP(it->c_str())) //不判断内网外网.
			{
				PingSpecRouter(it->c_str(),0);
			}else */
			{
				PingSpecRouter(it->c_str(),i);
				++i;
			}			
			
			++it;
		}		
	}
	printf("%s finish-finish\n",__FUNCTION__);
	if (*m_prun == 1)
	{
		return TRUE;
	}	
	return FALSE;
}

BOOL  CRouterLineQ::Initialize(int times,int nInterval,const char *ptestIp,int *nrun)
{
	printf("Initialize begin\n");
	m_prun = nrun;
	m_pingInterval = nInterval;
	if (nInterval <= 0)
	{
		m_pingInterval = LINETESTRESULT::DEFAULT_TEST_INTERVAL; //100;
		TRACE_LOG1("线路测试传入时间间隔不正确， 取默认:%d", m_pingInterval);
	}
	m_pingtms = times;
	if (m_pingtms == 0)
	{
		m_pingtms = LINETESTRESULT::DEFAULT_TEST_TIMES; //50;
		TRACE_LOG1("线路测试传入测试次数不正确， 取默认:%d", m_pingtms);
	}
	std::string strtmp;
	int nport;
	ParseUrl(ptestIp,m_testip,strtmp,nport);
	
	printf("Initialize:cnt=%d,interval=%d\n",times,nInterval);
	return TRUE;
}

int CRouterLineQ::TraceRouterLineStart()
{
	m_vectRouterLine1.clear();
	m_vecResult.clear();
	
	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),m_startTm);
	
	printf("TraceRouterLineStart:starttime=%s\n",m_startTm.c_str());
	
	GetRouterLine();
	
	return (int)TestLineRoutes();
}
