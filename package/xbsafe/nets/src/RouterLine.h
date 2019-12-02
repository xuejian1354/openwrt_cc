// RouterLine.h : Declare the exported functions for the DLL application.
//
#ifndef ROUTERLINE_H
#define ROUTERLINE_H
#include <string.h>
#include <vector>
#include <string>
#include "Common.h"

class CRouterLineQ 
{
public:
	CRouterLineQ();
	~CRouterLineQ();

public:
	const std::string& GetStartTime(){ return m_startTm;  }
	std::vector<LINETESTRESULT>& GetResult(){ return m_vecResult; }

	//初始化当前类，将netevmchecker，xbmain的程序模块载入，并初始化相关的数据
	//pServGuid:传入服务器生成的guid，作为统一批次id,如果没有，可以主动请求服务器获取。
	//times:多少次为一组，ping多少次
	//nInterval：发包间隔
	BOOL  Initialize(int times,int nInterval,const char *ptestIp,int *nrun);

	//启动网络质量测试线程
	int  TraceRouterLineStart();
public:
	BOOL TestLineRoutes();
	void GetRouterLine();
	BOOL PingSpecRouter(const char *pHost,int nSeq);
	BOOL CheckVecIpEqual(std::vector<std::string> ipv1,std::vector<std::string> ipv2);
	double GetDxValue(std::vector<double> &vecVals,double nex);
private:
	std::string m_testip;
	std::string m_startTm;
	int 	*m_prun;
	int		m_pingtms;//发包数量
	int		m_pingInterval;//发包间隔
	
	std::vector<std::string> m_vectRouterLine1;	
	std::vector<LINETESTRESULT> m_vecResult;//线路测试计算后的结果集
};

#endif