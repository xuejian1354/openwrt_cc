// dllmain.cpp : Defines the entry point for the DLL application.
//g++ base64.cpp Common.cpp PhysicalSocket.cpp EpNetIA.cpp NetInfoCheck.cpp ping.c tracert.c XbHttp.cpp RouterLine.cpp TestEpNetSpeed.cpp Main.cpp -o main -L. -lrt -lpthread -Wall
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <map>
#include <vector>
#include <wchar.h>
#include <sstream>
#include <wchar.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <getopt.h>
#include <signal.h>
#include <semaphore.h> 

#include "Lock.h"
#include "json.h"
#include "RouterLine.h"
#include "Common.h"
#include "TestEpNetSpeed.h"
#include "EpNetIA.h"
#include "XbHttp.h"
//#include "base64.h"
#include "SpeedTest.h"
#include "ctcSpeedTest.h"
#include "ping.h"
#include "tracert.h"
#include "wanclient.h"
//#include "localserver.h"

#include "ct_sgw_api.h"

#include "Flow.h"
#include "ini.h"
#include "period_task.h"
#include "echoClient.h"

std::map<int,std::wstring> g_Test_Info;
std::wstring strSvrId;

connect_method xb_conn;

//功能id；运行状态：0未运行，1正在运行,2取消运行
std::map<int,int> g_fun_status;
CMutex g_map_mutex;
CMutex g_map_result_mutex;
std::map<int,std::vector<LINETESTRESULT> > g_fun_result;
std::map<int,std::string> g_send_result;



unsigned long totalSize = 0;
unsigned long totalTime = 0;

static void * runFuncList(void *ptr);
static void * threadFuncBody(void *ptr);

int send_report_with_new_connection(const char *msg,int len);

//--------------------------------------------------------------------------------
//以下是新版本的测速逻辑，作为插件应用
//--------------------------------------------------------------------------------
void Initialize()
{
	std::vector<LINETESTRESULT> tmp;
	std::string strtmp;
	for(int i=0;i<11;++i)
	{
		g_fun_status[200 + i] = 0;
		g_fun_result[200 + i] = tmp;
		g_send_result[200 +i] = strtmp;
	}
}

static int ParsePluginParameter(duk_context *ctx, duk_idx_t array_obj, int array_index, PluginParam * pstructParam)
{
	std::string tmp;
	char delims[] = "|";
	char *result = NULL;
	char *purl = NULL;
	int i = 0;
	unsigned int ipos = 0;

	pstructParam->funcList[0].ninterval = getArrayObjInt(ctx, array_obj, array_index, "Interval");
	pstructParam->funcList[0].ntimes = getArrayObjInt(ctx, array_obj, array_index,"Times");
	getArrayObjString(ctx, array_obj, array_index, "SpeedTime", pstructParam->funcList[0].szSpeedTime);
	
	if(pstructParam->funcList[0].nfuncId == 104 || 
       pstructParam->funcList[0].nfuncId == 105 ||
      pstructParam->funcList[0].nfuncId == 109 ||
      pstructParam->funcList[0].nfuncId == 111)
	{
		purl = (char *)malloc(5 * MAX_PATH);
		memset(purl,0,5 * MAX_PATH);
		getArrayObjString(ctx, array_obj, array_index, "SpeedAddr",purl);
		strcpy(pstructParam->SpeedAddr, purl);
		result = strtok( purl, delims );
       i = 0;
		while( result != NULL ) {
		   if(strlen(result) > 0 && i < 5)
		   {
			   if(pstructParam->funcList[0].nfuncId == 104 || pstructParam->funcList[0].nfuncId == 109 || pstructParam->funcList[0].nfuncId == 111)
			   {
				   strcpy(pstructParam->addrInfo[i].addr,result);
			   }else
			   {
				   tmp = result;
				   ipos = tmp.find(":");
				   if(ipos != std::string::npos)
				   {
					   strcpy(pstructParam->addrInfo[i].addr,tmp.substr(0,ipos).c_str());
					   pstructParam->addrInfo[i].nport = atoi(tmp.substr(ipos + 1).c_str());
				   }else
				   {
					   strcpy(pstructParam->addrInfo[i].addr,result);
					   pstructParam->addrInfo[i].nport = 80;
				   }
			   }
			   ++i;
		   }
		   result = strtok( NULL, delims );
		} 
		free(purl);
	}
	else
	{
		getArrayObjString(ctx, array_obj, array_index, "SpeedAddr",pstructParam->addrInfo[0].addr);
		strcpy(pstructParam->SpeedAddr, pstructParam->addrInfo[0].addr);
	}
	
	printf("%s finish\n",__FUNCTION__);
	return 0;
}


int ParsePluginParameterList(const char *pParameter, std::vector<PluginParam*>* request, PluginParam *pstructParam)
{
	std::string decodestr;

	duk_context *ctx = NULL;
	duk_idx_t indx0;


	int i;
	int len;
    duk_ret_t ret ;
    duk_idx_t array_obj;

	printf("%s begin\n",__FUNCTION__);
	
	decodestr = pParameter;//Base64::decode64(pParameter);
	if(decodestr.empty())
		return 4;
	
	jsonInitialize(&ctx,&indx0);
	
	pstructParam->nReqId = getInt(ctx,decodestr.c_str(),"ID");
	//printf("%s begin,1\n",__FUNCTION__);
	getString(ctx,decodestr.c_str(),"CmdType",pstructParam->szCmdType);
	//printf("%s begin,2\n",__FUNCTION__);
	getString(ctx,decodestr.c_str(),"ExtendInfo",pstructParam->ExtendInfo);
	getString(ctx,decodestr.c_str(),"TestToken",pstructParam->TestToken);
	getString(ctx,decodestr.c_str(),"UUID",pstructParam->UUID);
	getString(ctx,decodestr.c_str(),"UserBandWidth",pstructParam->UserBandWidth);
	
	if(strcmp(pstructParam->szCmdType,"SET_SPEED_START") == 0)
	{
		pstructParam->ncmd = 0;
	}else if(strcmp(pstructParam->szCmdType,"SET_SPEED_STOP") == 0)
	{
		pstructParam->ncmd = 1;
	}
	else if(strcmp(pstructParam->szCmdType,"SET_SPEED_QUERY") == 0)
	{
		pstructParam->ncmd = 2;
	}else 
	{
		jsonDestroy(ctx);
		return 4;
	}
	//printf("%s begin,3\n",__FUNCTION__);
	getString(ctx,decodestr.c_str(),"SequenceId",pstructParam->szSeqId);

	ret = loadJsonArrayEx(ctx, decodestr.c_str(), "ParameterList", &len, &array_obj);

	pstructParam->nbusinessType = getArrayObjInt(ctx, array_obj, 0,"BusinessType");

    for(i=0; i< len; i++){
    	PluginParam *param = (PluginParam *)malloc(sizeof(PluginParam));
		memcpy(param,pstructParam,sizeof(PluginParam));
        param->nbusinessType = getArrayObjInt(ctx, array_obj, i,"BusinessType");
        if(param->nbusinessType < 200 && param->nbusinessType > 213)
			return 4;

		param->funcList[0].nfuncId = param->nbusinessType - 100;
		if(pstructParam->ncmd == 0)
			ParsePluginParameter(ctx, array_obj, i, param);
		request->push_back(param);
    }

    jsonDestroy(ctx);
    return 0;
}

void ReturnPluginExecuteResult(struct NetworkPort* port,const char *pResult)
{
	printf("%s,send:%s\n",__FUNCTION__,pResult);
	//发送结果信息
	port->sendback(port,pResult,strlen(pResult));
}

void BuildMidResult(struct NetworkPort* port,PluginParam *param,int errcode,const char *pmsg)
{
	std::string strResult;
	duk_context *ctx = NULL;
	duk_idx_t indx0;
	duk_size_t datalen;
	
	jsonInitialize(&ctx,&indx0);
	
	putInt(ctx,indx0,"Result",0);
	putInt(ctx,indx0,"ID",param->nReqId);
	putString(ctx,indx0,"CmdType",param->szCmdType);
	putString(ctx,indx0,"SequenceId",param->szSeqId);
	putIntToString(ctx,indx0,"Status",errcode);
	putObjProp(ctx,indx0,"ResultData");
	
	duk_json_encode(ctx,indx0);
	strResult = duk_get_lstring(ctx,indx0,&datalen);
	jsonDestroy(ctx);
	//strResult = Base64::encode64(strResult);
	
	ReturnPluginExecuteResult(port,strResult.c_str());
}

unsigned int ComputeBgFlow(){
	if(totalSize !=0 && totalTime != 0)
		return (totalSize/(float)totalTime);  //KB.
	return 0;
}

void BuildFinResult(PluginParam *param, duk_context *ctx, duk_idx_t array_idx, int index);
void BuildExtObject(char * extend_info, duk_context *ctx, duk_idx_t indx);

std::string getCurrentTime()
{
	std::string currentDate;

	std::wstring strtmp;
	GetSystemTime(strtmp);
	w2c(strtmp.c_str(),currentDate);

	return currentDate;
}

void BuildFinResultList(std::vector<PluginParam*>* request)
{
	int i=0;
	PluginParam* param = (*request)[i];

	std::string strResult;
	duk_context *ctx = NULL;
	duk_idx_t indx0;
	duk_idx_t array_idx;

	duk_size_t datalen;
	jsonInitialize(&ctx,&indx0);
	
	putString(ctx,indx0,"RPCMethod","Report");
	putInt(ctx,indx0,"OrgID",param->nReqId);
	putString(ctx,indx0,"CmdType","REPORT_SPEED");
	putString(ctx,indx0,"Time",param->funcList[0].szSpeedTime);
	putString(ctx,indx0,"GatewayTime",getCurrentTime().c_str());
	putIntToString(ctx,indx0,"BusinessType",param->nbusinessType);
	putString(ctx,indx0,"Mac",getMac());
	putInt(ctx,indx0,"BgFlow",ComputeBgFlow());
    putInt(ctx,indx0,"PHYStatus",10);
    putString(ctx,indx0,"SW_Channel",xb_conn.channel);
    
	if(strlen(param->TestToken) != 0)
		putString(ctx,indx0,"TestToken",param->TestToken);
	if(strlen(param->UUID) != 0)
		putString(ctx,indx0,"UUID",param->UUID);

	putArrayProp(ctx,indx0,"SpeedInfo");
	array_idx = getObj(ctx,indx0,"SpeedInfo");

	for(i=0; i< request->size(); i++)
	{
		param = (*request)[i];
		BuildFinResult(param, ctx, array_idx, i);
	}

	if(strlen(param->ExtendInfo) != 0)
		BuildExtObject(param->ExtendInfo, ctx, indx0);
		

	jsonTmpObjRemove(ctx,array_idx);
	
	duk_json_encode(ctx,indx0);
	strResult = duk_get_lstring(ctx,indx0,&datalen);
	jsonDestroy(ctx);
	printf("%s,%s\n",__FUNCTION__,strResult.c_str());
	for(i=0; i< request->size(); i++)
	{
		param = (*request)[i];
		g_send_result[param->nbusinessType] = strResult;//Base64::encode64(strResult);
	}	
	
	printf("ReturnPluginExecuteResult before\n");
	
	int isend = 0;

	if(param->port->packetFlag == 0)
	{
		isend = send_report_with_new_connection(strResult.c_str(),strResult.length());
	}else
	{
		isend = param->port->sendback(param->port, strResult.c_str(),strResult.length());
	}

	
	printf("%s finish,send:%d\n",__FUNCTION__,isend);
}

void BuildFinResult(PluginParam *param, duk_context *ctx, duk_idx_t array_idx, int index)
{
	int subArray = 0;	
	duk_idx_t array_obj;
	duk_idx_t sub_array_idx;
	duk_idx_t sub_array_obj;
	int i = 0;

	printf("%s begin\n",__FUNCTION__);
	
	int isize = g_fun_result[param->nbusinessType].size();
	if(isize == 0)
	{
		return;
	}

	while( param->funcList[i].nfuncId != 0)
	{
		printf("%s begin to struct reuslt,%d.\n",__FUNCTION__,param->funcList[i].nfuncId);
		
		putArrayIndexObj(ctx,array_idx,index,&array_obj);
		putIntToString(ctx,array_obj,"funcid",param->funcList[i].nfuncId);
		putIntToString(ctx,array_obj,"errcode",0);
		putString(ctx,array_obj,"errmsg","测试完成");
		putString(ctx,array_obj,"SpeedAddr", param->SpeedAddr);

		switch(param->funcList[i].nfuncId)
		{
			case ENUM_ROUTE_TRACER:
			{					
				LINETESTRESULT lt;
				std::string strtmp;
				//list array 
				subArray = 0;
				sub_array_obj = 0;
				sub_array_idx = 0;
				putArrayProp(ctx,array_obj,"list");
				sub_array_idx = getObj(ctx,array_obj,"list");
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode == 4)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putArrayIndexObj(ctx,sub_array_idx,subArray,&sub_array_obj);
							putIntToString(ctx,sub_array_obj,"id",lt.nIndx);
							putString(ctx,sub_array_obj,"ip",lt.strip);
							putDoubleToString(ctx,sub_array_obj,"min",lt.nmin);
							putDoubleToString(ctx,sub_array_obj,"max",lt.nmax);
							putDoubleToString(ctx,sub_array_obj,"avg",lt.navg);
							putDoubleToString(ctx,sub_array_obj,"varian",lt.ndx);
							
							++subArray;
							jsonTmpObjRemove(ctx,sub_array_obj);
						}
					}
				}	
				jsonTmpObjRemove(ctx,sub_array_idx);
			}
			break;
			case ENUM_TCP_DELAY:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode == 4)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);
							putDoubleToString(ctx,array_obj,"min",lt.nmin);
							putDoubleToString(ctx,array_obj,"max",lt.nmax);
							putDoubleToString(ctx,array_obj,"avg",lt.navg);
							putDoubleToString(ctx,array_obj,"varian",lt.ndx);
							putIntToString(ctx,array_obj,"send",lt.nSend);
							putIntToString(ctx,array_obj,"lost",lt.nLost);
							break;
						}
					}
				}
			}
			break;
			case ENUM_UDP_DELAY:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode == 4)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);
							putDoubleToString(ctx,array_obj,"min",lt.nmin);
							putDoubleToString(ctx,array_obj,"max",lt.nmax);
							putDoubleToString(ctx,array_obj,"avg",lt.navg);
							putDoubleToString(ctx,array_obj,"varian",lt.ndx);
							putIntToString(ctx,array_obj,"send",lt.nSend);
							putIntToString(ctx,array_obj,"lost",lt.nLost);
							break;
						}
					}
				}
			}
			break;
			case ENUM_UDP_LOST:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode == 4)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);
							putIntToString(ctx,array_obj,"send",lt.nSend);
							putIntToString(ctx,array_obj,"recv",lt.nSend - lt.nLost);
							putIntToString(ctx,array_obj,"lost",lt.nLost);
							break;
						}
					}
				}
			}
			break;
			case ENUM_SPEED_DOWN:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode != 3)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);
							putDoubleToString(ctx,array_obj,"min",lt.nmin);
							putDoubleToString(ctx,array_obj,"max",lt.nmax);
							putDoubleToString(ctx,array_obj,"avg",lt.navg);
							putDoubleToString(ctx,array_obj,"varian",lt.ndx);
							break;
						}
					}
				}
			}
			break;
			case ENUM_SPEED_1000M_DOWN:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode != 3)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);

							putIntToString(ctx,array_obj,"send",lt.nSend);
							putIntToString(ctx,array_obj,"recv",lt.nSend - lt.nLost);
							putIntToString(ctx,array_obj,"lost",lt.nLost);

							putIntToString(ctx,array_obj,"udptime",lt.nUDPTime);
							putIntToString(ctx,array_obj,"udppps",lt.nUDPPPS);
							putIntToString(ctx,array_obj,"udpbps",lt.nUDPBPS);
							putIntToString(ctx,array_obj,"totaltime",lt.nTotalTime);

							putDoubleToString(ctx,array_obj,"min",lt.nmin);
							putDoubleToString(ctx,array_obj,"max",lt.nmax);
							putDoubleToString(ctx,array_obj,"avg",lt.navg);
							putDoubleToString(ctx,array_obj,"varian",lt.ndx);

							putIntToString(ctx,array_obj,"user_bandwidth",lt.nUserBandWidth);
							putIntToString(ctx,array_obj,"orgavg",lt.nOldAvg);
							putIntToString(ctx,array_obj,"wanbps",lt.nWANBPS);
							putString(ctx,array_obj,"ifpolicy",lt.if_policy);

							break;
						}
					}
				}
			}
			break;
			case ENUM_SPEED_ROUTER_DOWN:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode != 3)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);

							putIntToString(ctx,array_obj,"send",lt.nSend);
							putIntToString(ctx,array_obj,"recv",lt.nSend - lt.nLost);
							putIntToString(ctx,array_obj,"lost",lt.nLost);

							putIntToString(ctx,array_obj,"udptime",lt.nUDPTime);
							putIntToString(ctx,array_obj,"udppps",lt.nUDPPPS);
							putIntToString(ctx,array_obj,"udpbps",lt.nUDPBPS);
							putIntToString(ctx,array_obj,"totaltime",lt.nTotalTime);

							putDoubleToString(ctx,array_obj,"min",lt.nmin);
							putDoubleToString(ctx,array_obj,"max",lt.nmax);
							putDoubleToString(ctx,array_obj,"avg",lt.navg);
							putDoubleToString(ctx,array_obj,"varian",lt.ndx);

							putIntToString(ctx,array_obj,"user_bandwidth",lt.nUserBandWidth);
							putIntToString(ctx,array_obj,"orgavg",lt.nOldAvg);
							putIntToString(ctx,array_obj,"wanbps",lt.nWANBPS);
							putString(ctx,array_obj,"ifpolicy",lt.if_policy);

							break;
						}
					}
				}
			}
			break;
			case ENUM_SPEED_CTC_DOWN:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode != 3)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);

							putIntToString(ctx,array_obj,"send",lt.nSend);
							putIntToString(ctx,array_obj,"recv",lt.nSend - lt.nLost);
							putIntToString(ctx,array_obj,"lost",lt.nLost);

							putIntToString(ctx,array_obj,"udptime",lt.nUDPTime);
							putIntToString(ctx,array_obj,"udppps",lt.nUDPPPS);
							putIntToString(ctx,array_obj,"udpbps",lt.nUDPBPS);
							putIntToString(ctx,array_obj,"totaltime",lt.nTotalTime);

							putDoubleToString(ctx,array_obj,"min",lt.nmin);
							putDoubleToString(ctx,array_obj,"max",lt.nmax);
							putDoubleToString(ctx,array_obj,"avg",lt.navg);
							putDoubleToString(ctx,array_obj,"varian",lt.ndx);

							putIntToString(ctx,array_obj,"ctcStatus",getCtcTestStatus());
							putString(ctx,array_obj,"ctcResult",getCtcTestResult());
							putString(ctx,array_obj,"ctcURL",getCtcTestURL());
							putIntToString(ctx,array_obj,"ctcLastRet",getCtcTestLastRet());
							putString(ctx,array_obj,"ctcLastFun",getCtcTestLastFun());

							break;
						}
					}
				}
			}
			break;
			case ENUM_SPEED_UP:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode != 3)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);
							putDoubleToString(ctx,array_obj,"min",lt.nmin);
							putDoubleToString(ctx,array_obj,"max",lt.nmax);
							putDoubleToString(ctx,array_obj,"avg",lt.navg);
							putDoubleToString(ctx,array_obj,"varian",lt.ndx);
							break;
						}
					}
				}
			}
			break;
			
			case ENUM_SPEED_UDP_UPLOAD:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode != 3)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);
							putDoubleToString(ctx,array_obj,"min",lt.nmin);
							putDoubleToString(ctx,array_obj,"max",lt.nmax);
							putDoubleToString(ctx,array_obj,"avg",lt.navg);
							putDoubleToString(ctx,array_obj,"varian",lt.ndx);

							putIntToString(ctx,array_obj,"udptime",lt.nUDPTime);
							putIntToString(ctx,array_obj,"udppps",lt.nUDPPPS);
							putIntToString(ctx,array_obj,"udpbps",lt.nUDPBPS);
							putIntToString(ctx,array_obj,"totaltime",lt.nTotalTime);
							putString(ctx,array_obj,"udpResult",getUdpUpTestResult());

							break;
						}
					}
				}
			}
			break;

			case ENUM_SPEED_PING:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode == 4)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);
							putDoubleToString(ctx,array_obj,"min",lt.nmin);
							putDoubleToString(ctx,array_obj,"max",lt.nmax);
							putDoubleToString(ctx,array_obj,"avg",lt.navg);
							putDoubleToString(ctx,array_obj,"varian",lt.ndx);
							putIntToString(ctx,array_obj,"send",lt.nSend);
							putIntToString(ctx,array_obj,"lost",lt.nLost);
							break;
						}
					}
				}
			}
			break;
			case ENUM_SPEED_TRACEROUTE:
			{
				LINETESTRESULT lt;
				//list array 
				subArray = 0;
				sub_array_obj = 0;
				sub_array_idx = 0;
				putArrayProp(ctx,array_obj,"list");
				sub_array_idx = getObj(ctx,array_obj,"list");
				
				for(int j=0; j<isize; ++j)
				{					
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode == 4)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{							
							putArrayIndexObj(ctx,sub_array_idx,subArray,&sub_array_obj);
							putIntToString(ctx,sub_array_obj,"id",lt.nIndx);
							putString(ctx,sub_array_obj,"ip",lt.strip);
							putDoubleToString(ctx,sub_array_obj,"delay",lt.navg);
							
							++subArray;
							jsonTmpObjRemove(ctx,sub_array_obj);
						}
					}
				}
				jsonTmpObjRemove(ctx,sub_array_idx);
			}
			break;
			case ENUM_SPEED_TELNET:
			{
				LINETESTRESULT lt;
				std::string strtmp;
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						if(lt.nvalCode == 4)
						{
							putString(ctx,array_obj,"ip",lt.strip);
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{
							putString(ctx,array_obj,"ip",lt.strip);
							putDoubleToString(ctx,array_obj,"conntime",lt.navg);
						}
					}
				}
			}
			break;
			case ENUM_SPEED_WEB_SPEED:
			{
				LINETESTRESULT lt;
				//list array 
				subArray = 0;
				sub_array_obj = 0;
				sub_array_idx = 0;
				putArrayProp(ctx,array_obj,"list");
				sub_array_idx = getObj(ctx,array_obj,"list");
				
				for(int j=0; j<isize; ++j)
				{
					lt = g_fun_result[param->nbusinessType].at(j);
					if(lt.funcId == param->funcList[i].nfuncId)
					{
						printf("ENUM_SPEED_WEB_SPEED %d\n",lt.nvalCode);
						if(lt.nvalCode == 4)
						{
							putIntToString(ctx,array_obj,"errcode",-1);
							putString(ctx,array_obj,"errmsg","测试失败");
							break;
						}else
						{							
							putArrayIndexObj(ctx,sub_array_idx,subArray,&sub_array_obj);
							putString(ctx,sub_array_obj,"url",lt.strip);
							putIntToString(ctx,sub_array_obj,"errcode",lt.WebStatus);
							putIntToString(ctx,sub_array_obj,"pagesize",lt.WebPageSize);
							putDoubleToString(ctx,sub_array_obj,"dnstime",lt.DNSTime);
							putDoubleToString(ctx,sub_array_obj,"conntime",lt.ResponceTime);
							putDoubleToString(ctx,sub_array_obj,"firstpackagetime",lt.FstPkgTime);
							putDoubleToString(ctx,sub_array_obj,"loadtime",lt.LoadTime);

							++subArray;
							jsonTmpObjRemove(ctx,sub_array_obj);
						}
					}
				}
				jsonTmpObjRemove(ctx,sub_array_idx);
			}
			break;
			default:
			break;
		}
		++i;
		jsonTmpObjRemove(ctx,array_obj);
	}
	
}

static std::string buildIdResult(int id, std::string origionalResult)
{
  char buf[32];
  sprintf(buf, "{\"ID\":%d,", id);

  return buf + origionalResult.erase(0,1);
}

void SendFinResult(struct NetworkPort* port,PluginParam *param)
{
	if(g_fun_result[param->nbusinessType].size() == 0 || g_send_result[param->nbusinessType].length() == 0)
	{
		BuildMidResult(port,param,-1,"该业务不存在，或执行失败。");
		return;
	}
	
	std::string result = buildIdResult(param->nReqId, g_send_result[param->nbusinessType]);
	ReturnPluginExecuteResult(port, result.c_str());
}

void free_request(std::vector<PluginParam*>* request)
{
	int i;
	PluginParam* param = NULL;
	for(i=0; i< request->size(); i++)
	{
		param = (*request)[i];
		if(param != NULL)
		{
			free(param);
		}
	}

	delete request;
}

int NetQualityTest(struct NetworkPort* port,const char *pData)
{
    int ret = 0;
	int ntimeout = 0;
	int cancleStatus = 0;
	int request_number = 0;
	int i;
	int verifyCode = 0;
	PluginParam* param = NULL;
	
	PluginParam *pstructParam = (PluginParam *)malloc(sizeof(PluginParam));
	memset(pstructParam,0,sizeof(PluginParam));

	std::vector<PluginParam*>* request = new std::vector<PluginParam*>();

	int iret = ParsePluginParameterList(pData,request, pstructParam);
	if(iret != 0)
	{
		BuildMidResult(port,pstructParam,iret,"参数格式不对");
		free_request(request);
		free(pstructParam);
		return -100;
	}

	for(i=0; i< request->size(); i++)
	{
		param = (*request)[i];
		param->port = port;
		if((pstructParam->ncmd == 0 || pstructParam->ncmd == 2) && g_fun_status[param->nbusinessType] == 1)
		{
			verifyCode = 3; //
			break;
		}
		if(pstructParam->ncmd == 2 && g_fun_status[param->nbusinessType] == 0)
		{
			//直接将结果返回
			verifyCode = 5; //SendFinResult(port,param);
			break;
		}

		if(pstructParam->ncmd == 1 && g_fun_status[param->nbusinessType] == 0)
		{
			verifyCode = 1; 
			
		}
		//正在执行
		if(pstructParam->ncmd == 1 && g_fun_status[param->nbusinessType] == 1)
		{
			g_map_mutex.Lock();
			g_fun_status[param->nbusinessType] = 2;//取消该业务执行
			g_map_mutex.Unlock();
			
			while(g_fun_status[param->nbusinessType] != 0)
			{
				sleep(1);
				ntimeout += 1;
				if(ntimeout >= 6) 
				{
					cancleStatus = 1;
					break;
				}
			}
			if(cancleStatus == 1)
				verifyCode = 2; 
			else
				verifyCode = 1; 

		}		
	}

	printf("ncmd=%d,request size %d, verifyCode=%d\n", pstructParam->ncmd, request->size(), verifyCode);

	if(verifyCode == 1)
	{
		BuildMidResult(port,pstructParam,1,"该业务取消执行成功"); //BuildMidResult(sock,pstructParam,1,"该业务已停止执行");
	}else if(verifyCode == 2)
	{
		BuildMidResult(port,pstructParam,2,"该业务取消失败");
	}else if(verifyCode == 3)
	{
		BuildMidResult(port,pstructParam,3,"该业务正在测试中");
	}else if(verifyCode == 5)
	{
		SendFinResult(port,pstructParam);
	}else if(verifyCode == 0)
	{
		BuildMidResult(port,pstructParam,0,"启动成功");
	}
	
	
	//printf("type:%d-ncmd=%d,current status:%d\n",pstructParam->nbusinessType,pstructParam->ncmd,g_fun_status[pstructParam->nbusinessType]);
	
	if(verifyCode != 0 || request->size() == 0){
		free_request(request);
		free(pstructParam);
		return 0;
	}

	pthread_t threadID;
	
	int result = pthread_create(&threadID, NULL, &runFuncList, (void *)request);
	if (0 != result) {		  
		perror("pthread_create failed");
		
		g_map_mutex.Lock();
		g_fun_status[pstructParam->nbusinessType] = 0;
		g_map_mutex.Unlock();
		
		//BuildMidResult(port,pstructParam,6,"测试线程启动失败");
		free(pstructParam);
		free_request(request);
		return 0;
	}
	
	free(pstructParam);
    return 0;
	/**/
	//threadFuncBody((void *)pstructParam);
}

static void * runFuncList(void *ptr)
{
	int i;
	int verifyCode = 0;
	PluginParam* param = NULL;

	pthread_detach(pthread_self());

	std::vector<PluginParam*>* request = (std::vector<PluginParam*>*) ptr;

	for(i=0; i< request->size(); i++)
	{
		param = (*request)[i];
		g_map_mutex.Lock();
		g_fun_status[param->nbusinessType] = 1;
		g_map_mutex.Unlock();
	}

	for(i=0; i< request->size(); i++)
	{
		param = (*request)[i];
		threadFuncBody(param);
	}

	BuildFinResultList(request);
	free_request(request);
	pthread_detach(pthread_self());
	pthread_exit(0);
}

static std::string get_next_token(std::string& param, char token, size_t& idx)
{
	if(idx == std::string::npos)
		return "";

	size_t start_idx = idx + 1;
	size_t idx2 = param.find(token, start_idx);
	
	std::string val;
	if(idx2 != std::string::npos)
	{	
		val = param.substr(start_idx, idx2 - idx);
	}else
	{
		val = param.substr(start_idx);
	}
	idx = idx2;

	printf("token %s\n", val.c_str());
	return val;
}


//测速执行的线程体
static void * threadFuncBody(void *ptr)
{
	int *pnTypeStatus = NULL;

	
	PluginParam *pParam = (PluginParam *)ptr;
	PluginParam &param = *pParam;
	
	//以下是要处理业务开始，正式开始执行
	//清除结果集合
	g_fun_result[param.nbusinessType].clear();
	g_send_result[param.nbusinessType].clear();
	//多线程同步标识
	pnTypeStatus = &g_fun_status[param.nbusinessType];
	printf("%d,%d\n",*pnTypeStatus,g_fun_status[param.nbusinessType]);
	
	int i = 0;
	while( param.funcList[i].nfuncId != 0)
	{
		if( *pnTypeStatus == 2) 
			break;
		
		int testinval = 0;
		int testcnt = 0;
		int port = 0;
		std::string strtmp;
		std::string strobj;
		switch(param.funcList[i].nfuncId)
		{
			case ENUM_ROUTE_TRACER:
			{
				printf("%d-ENUM_ROUTE_TRACER begin\n",param.nbusinessType);
				//GetTracerLineInfo(it->second, testinval, testcnt, addr);
				CRouterLineQ RouterLine;
				RouterLine.Initialize(param.funcList[i].ntimes, param.funcList[i].ninterval, param.addrInfo[0].addr,pnTypeStatus);
				if (RouterLine.TraceRouterLineStart() == 0)
				{
					RouterLine.GetResult().clear(); 
				}
				
				{
					printf("%d-TraceRouterLineStart suc\n",param.nbusinessType);
					
					std::vector<LINETESTRESULT> &vect = RouterLine.GetResult(); 
					if(vect.size() == 0)
					{
						LINETESTRESULT lr;
						lr.funcId = ENUM_ROUTE_TRACER;
						lr.nvalCode = 4;
						vect.push_back(lr);
					}
					
					g_map_result_mutex.Lock();
					g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
					g_map_result_mutex.Unlock();
					//PostData2Server(&RouterLine);
				}
				printf("%d-ENUM_ROUTE_TRACER finish\n",param.nbusinessType);
			}
			break;
			case ENUM_TCP_DELAY:
			{
				printf("%d-ENUM_TCP_DELAY begin\n",param.nbusinessType);
				//GetTcpUdpInfo(it->second, testinval, testcnt, addr, port);
				testinval = param.funcList[i].ninterval;
				testcnt = param.funcList[i].ntimes;
				ParseUrl(param.addrInfo[0].addr,strtmp,strobj,port);
				if ( testcnt <= 0 ) testcnt = LINETESTRESULT::DEFAULT_TEST_TIMES;
				if ( testinval <= 0 ) testinval = LINETESTRESULT::DEFAULT_TEST_INTERVAL;
				TestEpNetSpeed testSpeed;
				//if 
				(testSpeed.Init1(strtmp.c_str(),port,ENUM_TCP_DELAY,pnTypeStatus));
				{
					//if 
					(testSpeed.TcpPackageDelay(testcnt, testinval));
					{
						printf("%d-TcpPackageDelay suc\n",param.nbusinessType);
						
						//g_fun_result[param.nbusinessType].push_back(testSpeed.GetResult());
						std::vector<LINETESTRESULT> &vect = testSpeed.GetResult(); 
						if(vect.size() == 0)
						{
							LINETESTRESULT lr;
							lr.funcId = ENUM_TCP_DELAY;
							lr.nvalCode = 4;
							vect.push_back(lr);
						}
						
						g_map_result_mutex.Lock();
						g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
						g_map_result_mutex.Unlock();
						//PostData2Server(&testSpeed);
					}
				}
				printf("%d-ENUM_TCP_DELAY finish\n",param.nbusinessType);
			}
			break;
			case ENUM_UDP_DELAY:
			{
				printf("%d-ENUM_UDP_DELAY begin\n",param.nbusinessType);
				//GetTcpUdpInfo(it->second, testinval, testcnt, addr, port);
				testinval = param.funcList[i].ninterval;
				testcnt = param.funcList[i].ntimes;
				ParseUrl(param.addrInfo[0].addr,strtmp,strobj,port);
				if ( testcnt <= 0 ) testcnt = LINETESTRESULT::DEFAULT_TEST_TIMES;
				if ( testinval <= 0 ) testinval = LINETESTRESULT::DEFAULT_TEST_INTERVAL;
				TestEpNetSpeed testSpeed;
				//if 
				(testSpeed.Init1(strtmp.c_str(),port,ENUM_UDP_DELAY,pnTypeStatus));
				{
					//if 
					(testSpeed.UdpPackageDelay(testcnt, testinval));
					{
						printf("%d-UdpPackageDelay suc\n",param.nbusinessType);
						
						//g_fun_result[param.nbusinessType].push_back(testSpeed.GetResult());
						std::vector<LINETESTRESULT> &vect = testSpeed.GetResult(); 
						if(vect.size() == 0)
						{
							LINETESTRESULT lr;
							lr.funcId = ENUM_TCP_DELAY;
							lr.nvalCode = 4;
							vect.push_back(lr);
						}
						
						g_map_result_mutex.Lock();
						g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
						g_map_result_mutex.Unlock();
						//PostData2Server(&testSpeed);
					}
				}
				
				printf("%d-ENUM_UDP_DELAY finish\n",param.nbusinessType);
			}
			break;
			case ENUM_UDP_LOST:
			{
				printf("%d-ENUM_UDP_LOST begin\n",param.nbusinessType);
				//GetTcpUdpInfo(it->second, testinval, testcnt, addr, port);
				testinval = param.funcList[i].ninterval;
				testcnt = param.funcList[i].ntimes;
				ParseUrl(param.addrInfo[0].addr,strtmp,strobj,port);
				if ( testcnt <= 0 ) testcnt = LINETESTRESULT::DEFAULT_TEST_TIMES;
				if ( testinval <= 0 ) testinval = LINETESTRESULT::DEFAULT_TEST_INTERVAL;
				TestEpNetSpeed testSpeed;
				//if 
				(testSpeed.Init1(strtmp.c_str(),port,ENUM_UDP_LOST,pnTypeStatus));
				{
					//if 
					(testSpeed.UdpPackageLost(testcnt, testinval));
					{
						printf("%d-UdpPackageLost suc\n",param.nbusinessType);
						
						//g_fun_result[param.nbusinessType].push_back(testSpeed.GetResult());
						std::vector<LINETESTRESULT> &vect = testSpeed.GetResult(); 
						if(vect.size() == 0)
						{
							LINETESTRESULT lr;
							lr.funcId = ENUM_TCP_DELAY;
							lr.nvalCode = 4;
							vect.push_back(lr);
						}
						
						g_map_result_mutex.Lock();
						g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
						g_map_result_mutex.Unlock();
						//PostData2Server(&testSpeed);
					}
				}
				printf("%d-ENUM_UDP_LOST finish\n",param.nbusinessType);
			}
			break;
			case ENUM_SPEED_DOWN:
			{
				printf("%d-ENUM_SPEED_DOWN begin\n",param.nbusinessType);
				NetFlow bgFlow;
				bgFlow.start();

				TSpeedTest speed;
				if(speed.GetDownSpeedTestStrategy(param.addrInfo) != 0)
					speed.StartDownload(param.funcList[i].ntimes,pnTypeStatus);
				bgFlow.stop();

				//if(speed.GetDownSpeedTestStrategy(param.addrInfo) != 0 && speed.StartDownload(param.funcList[i].ntimes,pnTypeStatus))
				{
					printf("%d-StartDownload suc\n",param.nbusinessType);
					
					totalSize = bgFlow.wan_rx_bytes > speed.GetTotalSize() ? bgFlow.wan_rx_bytes-speed.GetTotalSize() : 0;
					totalTime = bgFlow.time_interval;

					//g_fun_result[param.nbusinessType](testSpeed.GetResult());
					std::vector<LINETESTRESULT> &vect = speed.GetResult(); 
					g_map_result_mutex.Lock();
					g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
					g_map_result_mutex.Unlock();
				}
				printf("%d-ENUM_SPEED_DOWN finish\n",param.nbusinessType);
				//PostData2Server(&speed);
			}
			break;
			case ENUM_SPEED_CTC_DOWN:
			{
				printf("%d-ENUM_SPEED_DOWN begin\n",param.nbusinessType);
				int ret = 0;
				ret = GetCtcDownSpeedTestStrategy(param.addrInfo);
				if(ret == 0)
					printf("GetCtcDownSpeedTestStrategy suc\n");
				else
					printf("GetCtcDownSpeedTestStrategy fail\n");

				if(ret == 0)
					ret = CtcStartHttpDownloadTest();

				std::vector<double> speedList;
				int sleepTime = param.funcList[i].ntimes;
				printf("sleepTime %d\n", sleepTime);
				if(sleepTime < 5)
					sleepTime = 5;

				if(ret == 0){
					for(int j=0; j< 3; j++){
						sleep(sleepTime + 7);				
						ret = CtcGetHttpDownloadTestResult();
						printf("Main.cpp call CtcGetHttpDownloadTestResult %d\n", ret);
						if(ret != 0)
							break;
						
						char* speedResult = getCtcTestResult();
						int resultCount = convert_ctc_result(sleepTime, speedResult, speedList);

						if(resultCount > 0)
							break;
					}
					
				}
				

				//if(speed.GetDownSpeedTestStrategy(param.addrInfo) != 0 && speed.StartDownload(param.funcList[i].ntimes,pnTypeStatus))
				{
					printf("%d-StartDownload suc\n",param.nbusinessType);
					LINETESTRESULT testResult;
					

					testResult.funcId = ENUM_SPEED_CTC_DOWN;
					memcpy(testResult.strip,param.addrInfo[0].addr,MAX_PATH - 1);
					testResult.nSend = 0;
					testResult.ninterval = 0;
					ComputeResultFromSpeedList(testResult, speedList);
					testResult.nLost = 0;
					testResult.nvalCode = 3;
					std::vector<LINETESTRESULT> m_vecResult;
					m_vecResult.push_back(testResult);

					g_map_result_mutex.Lock();
					g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),m_vecResult.begin(),m_vecResult.end());
					g_map_result_mutex.Unlock();

				}
				printf("%d-ENUM_SPEED_DOWN finish\n",param.nbusinessType);
				//PostData2Server(&speed);
			}
			break;
			case ENUM_SPEED_1000M_DOWN:
			{
				printf("%d-ENUM_SPEED_DOWN begin\n",param.nbusinessType);

				int auto_speed_test = 0;
				TSpeedTest speed;

				ParseUrl(param.addrInfo[0].addr,strtmp,strobj,port);
				int user_bandwidth = atoi(param.UserBandWidth); //default 1M
				int test_bandwidth = 1;
				int packetSize = 1460;  //default 1460 bytes
				int realPacketSize = 1460;
				if(strobj.length() > 1){
					size_t idx = 0;
					std::string val;
					val = get_next_token(strobj, '/', idx);
					test_bandwidth = atoi(val.c_str());
					if(idx != std::string::npos)
					{
						val = get_next_token(strobj, '/', idx);
						packetSize = atoi(val.c_str());
						realPacketSize = packetSize;
					}

					if(idx != std::string::npos)
					{
						val = get_next_token(strobj, '/', idx);
						realPacketSize = atoi(val.c_str());
					}
					auto_speed_test = 0;
				}else{
					auto_speed_test = 1;
				}

				if(user_bandwidth == 0)
					user_bandwidth = test_bandwidth;

				printf("1000M speed %s, %d, band %d/%d, packet %d, real %d\n", strtmp.c_str(), port, user_bandwidth, test_bandwidth, packetSize, realPacketSize);
				speed.GetDownSpeedTestStrategy(param.addrInfo);

				if(auto_speed_test == 0){

					NetFlow bgFlow;
					bgFlow.start();

					speed.StartUDPDownload(param.funcList[i].ntimes,pnTypeStatus, strtmp.c_str(), port, user_bandwidth, test_bandwidth, packetSize, realPacketSize);
					bgFlow.stop();

					//if(speed.GetDownSpeedTestStrategy(param.addrInfo) != 0 && speed.StartDownload(param.funcList[i].ntimes,pnTypeStatus))
					//{
						printf("%d-StartDownload suc\n",param.nbusinessType);
						
						totalSize = bgFlow.wan_rx_bytes > speed.GetTotalSize() ? bgFlow.wan_rx_bytes-speed.GetTotalSize() : 0;
						totalTime = bgFlow.time_interval;

					//}

				}else{
					printf("1000M speed uri path %s\n", strobj.c_str());
					int ntms = 1000;
					
					void enable_cache_interface();
					void disable_cache_interface();

					totalTime = 0;
					totalSize = 0;

					enable_cache_interface();
					speed.AutoStartUDPDownload(param.funcList[i].ntimes,pnTypeStatus, strtmp.c_str(), port, test_bandwidth, packetSize, realPacketSize);
					disable_cache_interface();
				}

				std::vector<LINETESTRESULT> &vect = speed.GetResult(); 
				g_map_result_mutex.Lock();
				g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
				g_map_result_mutex.Unlock();				

				printf("%d-ENUM_SPEED_DOWN finish\n",param.nbusinessType);
				//PostData2Server(&speed);				
			}
			break;

			case ENUM_SPEED_ROUTER_DOWN:
			{
				printf("%d-ENUM_SPEED_DOWN begin\n",param.nbusinessType);

				int auto_speed_test = 0;
				TSpeedTest speed;

				ParseUrl(param.addrInfo[0].addr,strtmp,strobj,port);
				int user_bandwidth = 1; //default 1M
				int packetSize = 1460;  //default 1460 bytes
				int realPacketSize = 1460;
				if(strobj.length() > 1){
					size_t idx = 0;
					std::string val;
					val = get_next_token(strobj, '/', idx);
					user_bandwidth = atoi(val.c_str());
					if(idx != std::string::npos)
					{
						val = get_next_token(strobj, '/', idx);
						packetSize = atoi(val.c_str());
						realPacketSize = packetSize;
					}

					if(idx != std::string::npos)
					{
						val = get_next_token(strobj, '/', idx);
						realPacketSize = atoi(val.c_str());
					}
					auto_speed_test = 0;
				}else{
					auto_speed_test = 1;
				}

				printf("1000M speed %s, %d, band %d, packet %d, real %d\n", strtmp.c_str(), port, user_bandwidth, packetSize, realPacketSize);
				speed.GetDownSpeedTestStrategy(param.addrInfo);

				NetFlow bgFlow;
				bgFlow.start();

				speed.StartRouterUDPDownload(param.funcList[i].ntimes,pnTypeStatus, strtmp.c_str(), port, user_bandwidth, packetSize, realPacketSize);
				bgFlow.stop();

				//if(speed.GetDownSpeedTestStrategy(param.addrInfo) != 0 && speed.StartDownload(param.funcList[i].ntimes,pnTypeStatus))
				//{
					printf("%d-StartDownload suc\n",param.nbusinessType);
					
					totalSize = bgFlow.wan_rx_bytes > speed.GetTotalSize() ? bgFlow.wan_rx_bytes-speed.GetTotalSize() : 0;
					totalTime = bgFlow.time_interval;

				//}


				std::vector<LINETESTRESULT> &vect = speed.GetResult(); 
				g_map_result_mutex.Lock();
				g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
				g_map_result_mutex.Unlock();				

				printf("%d-ENUM_SPEED_DOWN finish\n",param.nbusinessType);
				//PostData2Server(&speed);				
			}
			break;
			case ENUM_SPEED_UP:
			{
				printf("%d-ENUM_SPEED_UP begin\n",param.nbusinessType);
				NetFlow bgFlow;
				bgFlow.start();
				
				TSpeedTest speed;
				if(speed.GetUpSpeedTestStrategy(param.addrInfo) != 0)
					speed.StartUpLoad(param.funcList[i].ntimes,pnTypeStatus);
				//if(speed.GetUpSpeedTestStrategy(param.addrInfo) != 0 && speed.StartUpLoad(param.funcList[i].ntimes,pnTypeStatus))
				{
					printf("%d-StartUpLoad suc\n",param.nbusinessType);

					totalSize = bgFlow.wan_tx_bytes > speed.GetTotalSize() ? bgFlow.wan_rx_bytes-speed.GetTotalSize() : 0;
					totalTime = bgFlow.time_interval;

					//g_fun_result[param.nbusinessType](testSpeed.GetResult());
					std::vector<LINETESTRESULT> &vect = speed.GetResult(); 
										
					g_map_result_mutex.Lock();
					g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
					g_map_result_mutex.Unlock();
				}
				printf("%d-ENUM_SPEED_UP finish\n",param.nbusinessType);
			}
			break;
			
			case ENUM_SPEED_UDP_UPLOAD:
			{
				printf("%d-ENUM_SPEED_DOWN begin\n",param.nbusinessType);
				int ret = 0;
				
				ParseUrl(param.addrInfo[0].addr,strtmp,strobj,port);
				int user_bandwidth = 0; //default 1M
				int testMethod = 0;    //auto
			
				if(strobj.length() > 1){
					size_t idx = 0;
					std::string val;
					val = get_next_token(strobj, '/', idx);
					user_bandwidth = atoi(val.c_str());
					if(idx != std::string::npos)
					{
						val = get_next_token(strobj, '/', idx);
						testMethod = atoi(val.c_str());
					}
				}

				std::vector<double> speedList;
				int sleepTime = param.funcList[i].ntimes;
				printf("sleepTime %d\n", sleepTime);
				if(sleepTime < 5)
					sleepTime = 5;

				int totalTime =0;
				int bps = 0;
				int pps = 0;

				ret = StartUdpUpTest(strtmp.c_str(), port, sleepTime, user_bandwidth, testMethod, &totalTime, &bps, &pps);

				if(ret == 0){
					char* speedResult = getUdpUpTestResult();
					int resultCount = convert_ctc_result(sleepTime, speedResult, speedList);
				}
				

				//if(speed.GetDownSpeedTestStrategy(param.addrInfo) != 0 && speed.StartDownload(param.funcList[i].ntimes,pnTypeStatus))
				{
					printf("%d-StartDownload suc\n",param.nbusinessType);
					LINETESTRESULT testResult;
					

					testResult.funcId = ENUM_SPEED_UDP_UPLOAD;
					memcpy(testResult.strip,param.addrInfo[0].addr,MAX_PATH - 1);
					testResult.nSend = 0;
					testResult.ninterval = 0;
					ComputeResultFromSpeedList(testResult, speedList);
					testResult.nLost = 0;
					testResult.nvalCode = 3;
					testResult.nTotalTime = totalTime;
					testResult.nUDPBPS = bps;
					testResult.nUDPPPS = pps;

					std::vector<LINETESTRESULT> m_vecResult;
					m_vecResult.push_back(testResult);

					g_map_result_mutex.Lock();
					g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),m_vecResult.begin(),m_vecResult.end());
					g_map_result_mutex.Unlock();

				}
				printf("%d-ENUM_SPEED_DOWN finish\n",param.nbusinessType);
				//PostData2Server(&speed);
			}
			break;

			case ENUM_SPEED_PING:
			{
				printf("%d-ENUM_SPEED_PING begin\n",param.nbusinessType);
				std::vector<LINETESTRESULT> vect;
				LINETESTRESULT lt;
				
				static_ping_result stResult;
				memset(&stResult,0,sizeof(static_ping_result));
				ParseUrl(param.addrInfo[0].addr,strtmp,strobj,port);
				ping(strtmp.c_str(),param.funcList[i].ntimes,param.funcList[i].ninterval,&stResult,pnTypeStatus);
				
				printf("%d-ping suc\n",param.nbusinessType);
										
				if(*pnTypeStatus == 1 && stResult.nSend != 0)
				{
					printf("%d-ping nsend != 0,%s\n",param.nbusinessType,param.addrInfo[0].addr);
					
					memcpy(lt.strip,param.addrInfo[0].addr,MAX_PATH - 1);
					printf("%d-ping 1\n",param.nbusinessType);
					
					lt.funcId = ENUM_SPEED_PING;
					lt.nSend = stResult.nSend;
					lt.nLost = stResult.nSend - stResult.nRecv;
					lt.ninterval = param.funcList[i].ninterval;
					lt.nmax = stResult.maxtime;
					lt.nmin = stResult.mintime;
					lt.navg = stResult.avgtime;
					lt.ndx = stResult.mvari;
					
					printf("%d-ping 2\n",param.nbusinessType);
					
					vect.push_back(lt);
				}else
				{						
					lt.funcId = ENUM_SPEED_PING;
					lt.nvalCode = 4;
					vect.push_back(lt);
				}
									
				g_map_result_mutex.Lock();
				g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
				g_map_result_mutex.Unlock();
			
				printf("%d-ENUM_SPEED_PING finish\n",param.nbusinessType);
			}
			break;
			case ENUM_SPEED_TRACEROUTE:
			{
				printf("%d-ENUM_SPEED_TRACEROUTE begin\n",param.nbusinessType);
				std::vector<LINETESTRESULT> vect;
				LINETESTRESULT lt;
				int number = 30;
				tracert_result stResult[30];
				memset(&stResult,0,sizeof(tracert_result) * 30);
				ParseUrl(param.addrInfo[0].addr,strtmp,strobj,port);
				if(tracert(strtmp.c_str(),stResult,&number,pnTypeStatus) != -1)
				{
					printf("%d-tracert suc\n",param.nbusinessType);
					for(int j=0;j<number;++j)
					{
						lt.funcId = ENUM_SPEED_TRACEROUTE;
						lt.nIndx = j;
						memcpy(lt.strip,stResult[j].dstIp,MAX_PATH);
						lt.navg = stResult[j].time;
						vect.push_back(lt);
					}
					
				}else
				{
					memset(&lt,0,sizeof(LINETESTRESULT));
					lt.funcId = ENUM_SPEED_TRACEROUTE;
					lt.nvalCode = 4;
					vect.push_back(lt);
				}
				
				if(*pnTypeStatus != 1) break;
				
				g_map_result_mutex.Lock();
				g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
				g_map_result_mutex.Unlock();
				printf("%d-ENUM_SPEED_TRACEROUTE finish\n",param.nbusinessType);
			}
			break;
			case ENUM_SPEED_TELNET:
			{
				printf("%d-ENUM_SPEED_TELNET begin\n",param.nbusinessType);
				std::vector<LINETESTRESULT> vect;
				LINETESTRESULT lt;

				ParseUrl(param.addrInfo[0].addr,strtmp,strobj,port);
				int nsize = 32;
				char ip[32];
				wchar_t wcsip[32];
				memset(ip,0,32);
				memset(wcsip,0,64);
				Dns(strtmp.c_str(),ip,32);
				c2w(ip,wcsip,&nsize);
				CXbHttp http;
				http.SetAddrInfo(wcsip,(const unsigned short)port);
				lt.funcId = ENUM_SPEED_TELNET;
				memcpy(lt.strip,ip,MAX_PATH);
				if(http.CreateSocket())
				{
					printf("%d-CreateSocket suc\n",param.nbusinessType);
					lt.navg = http.GetConnectTime();
					vect.push_back(lt);
					
				}else
				{
					lt.nvalCode = 4;
					vect.push_back(lt);
				}
				
				if(*pnTypeStatus != 1) break;
				
				g_map_result_mutex.Lock();
				g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
				g_map_result_mutex.Unlock();
				printf("%d-ENUM_SPEED_TELNET finish\n",param.nbusinessType);
			}
			break;
			case ENUM_SPEED_WEB_SPEED:
			{
				printf("%d-ENUM_SPEED_WEB_SPEED begin\n",param.nbusinessType);
				TSpeedTest speed;
				//if
				(speed.StartWebTest(param.addrInfo,pnTypeStatus));
				{
					printf("%d-StartWebTest suc\n",param.nbusinessType);
					std::vector<LINETESTRESULT> &vect = speed.GetResult(); 
					if(vect.size() == 0)
					{
						LINETESTRESULT lr;
						lr.funcId = ENUM_SPEED_WEB_SPEED;
						lr.nvalCode = 4;
						vect.push_back(lr);
					}
					
					g_map_result_mutex.Lock();
					g_fun_result[param.nbusinessType].insert(g_fun_result[param.nbusinessType].end(),vect.begin(),vect.end());
					g_map_result_mutex.Unlock();
				}
				printf("%d-ENUM_SPEED_WEB_SPEED finish\n",param.nbusinessType);
			}
			break;
			default:
			break;
		}
		TRACE_LOG2(_T("****bustype:%d,funcid:%d test finish****\n"),param.nbusinessType,param.funcList[i].nfuncId);
		++i;
	}
	TRACE_TEXT(_T("***************all finish*******************\n"));
	
	//将结果转换为json格式
	printf("BuildFinResult before\n");
	//BuildFinResult(pParam);
	printf("BuildFinResult after\n");
	//---------------------------------
	
	g_map_mutex.Lock();
	g_fun_status[param.nbusinessType] = 0;
	g_map_mutex.Unlock();
	
	//free(pParam);

    return 0;
}


/*
<resultCode>0</resultCode><ErrDescription>success</ErrDescription> <srvid>d7194373-aa92-4348-9bb0-8a202ab3ee3f</srvid>
<func_test><test_code>1</test_code><test_ip>196.56.35.1</test_ip><testcnt>100</testcnt><testinval>100</testinval></func_test>
<func_test><test_code>2</test_code><testip>196.56.35.1:8080</testip><testcnt>100</testcnt><testinval>100</testinval></func_test>
<func_test><test_code>3</test_code><testip>196.56.35.1:8080</testip><testcnt>100</testcnt><testinval>100</testinval></func_test>
<func_test><test_code>4</test_code><testip>196.56.35.1:8080</testip><testcnt>100</testcnt><testinval>100</testinval></func_test>
<func_test><test_code>5</test_code><serverlist>
		<src url="http://ss.btte.net/netspeet/data/speet.dat" type="1" />
		<src url="http://221.238.15.242/netspeet/data/speet.dat" type="1" />
		<src url="http://219.148.63.18/netspeet/data/speet.dat" type="1" />
		</serverlist></func_test>
<func_test><test_code>6</test_code><serverlist>
		<src url="221.238.15.242" port="21356" />
		<src url="219.148.63.18" port="21356" />
		<src url="220.181.85.226" port="21356" />
		</serverlist></func_test>

*/

#define LOCKFILE "/var/run/mySpeed.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/* set advisory lock on file */
int lockfile(int fd)
{
        struct flock fl;
 
        fl.l_type = F_WRLCK;  /* write lock */
        fl.l_start = 0;
        fl.l_whence = SEEK_SET;
        fl.l_len = 0;  //lock the whole file
 
        return(fcntl(fd, F_SETLK, &fl));
}

int already_running(const char *filename)
{
	int fd;
	char buf[16];
	int iret = -1;
	fd = open(filename, O_RDWR | O_CREAT, LOCKMODE);
	if (fd < 0) {
		printf("can't open %s: %m\n", filename);
		return iret;
	}

	/* 先获取文件锁 */
	if (lockfile(fd) == -1) {
		if (errno == EACCES || errno == EAGAIN) {
			printf("file: %s already locked\n", filename);
			iret = 1;
		}else
		{
			printf("can't lock %s: %m\n", filename);
		}
		close(fd);
	}else
	{
		/* 写入运行实例的pid */
		ftruncate(fd, 0);
		sprintf(buf, "%ld", (long)getpid());
		write(fd, buf, strlen(buf) + 1);
		iret = 0;
		//close(fd);
	}
	return iret;
}

int debug_mac(char *mac);

int debug_account(char *account);
int debug_wan(char *ip);
int debug_lan(char *ip);


/******************************************************************************
 * usage
 ******************************************************************************/
static void usage(void)
{
    fprintf(stderr, "Usage: encode [options]\n\n"
      "Options:\n"
      "-h | --host       Speech file to record to\n"
      "-p | --port        Video file to record to\n"
      "-e | --ether     interface for binding.\n"
      "-a | --account   .\n"
      "-m | --mac     mac address.\n"
      "-o | --output     Video standard to use for display (see below).\n");
}

/******************************************************************************
 * parseArgs
 ******************************************************************************/
static void parseArgs(int argc, char *argv[])
{
    const char shortOptions[] = "a:m:h:p:w:l:";
    const struct option longOptions[] = {
        {"account",       required_argument, NULL, 'a'},
        {"mac",       required_argument, NULL, 'm'},
        {"host",       required_argument, NULL, 'h'},
        {"port",        required_argument, NULL, 'p'},
        {"wan",       required_argument, NULL, 'w'},
        {"lan",       required_argument, NULL, 'l'},
        {0, 0, 0, 0}
    };

    int     index;
    int     c;
    char    *extension;

    for (;;) {
        c = getopt_long(argc, argv, shortOptions, longOptions, &index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
                break;
            case 'a':
                debug_account(optarg);
                break;
            case 'm':
                debug_mac(optarg);
                break;

            case 'h':
                strcpy(xb_conn.host, optarg);
                break;

            case 'p':
                xb_conn.port = atoi(optarg);
                break;

            case 'w':
                debug_wan(optarg);
                break;

            case 'l':
                debug_lan(optarg);
                break;

            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }

}

void terminate_handle(int sig)
{
	printf("received SIGTERM %d\n", sig);
	_exit(0);
}

int send_report_with_new_connection(const char *msg,int len)
{
	int socket = -1;
	int retLen = 0;
	char *pbuf = NULL;
	int iret = -1;
	duk_context *ctx;
	duk_idx_t indx;
	struct timeval timeoutRecv = {30, 0};

	socket = create_tcp_client(xb_conn.reportHost,xb_conn.reportPort);
	if(socket == -1)
		return -1;

	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeoutRecv, sizeof(timeoutRecv));

	retLen = tcp_send(socket, msg, len);
	if(retLen <=0)
		return -1;

	tcp_recv(socket,&pbuf);

	jsonInitialize(&ctx,&indx);
	if(pbuf != NULL)
	{
		printf("*********%s,recv:%s=====\n",__FUNCTION__,pbuf);
		iret = getInt(ctx,pbuf,"Result");
		free(pbuf);		
	}
	jsonDestroy(ctx);

	close(socket);
	printf("send_report finished %d\n", iret);
	return iret;
}

int is_network_accessible(void)
{
	return checkNetLink("www.baidu.com") == 1;
}

int try_tcp_with_sleep(int* running, char* host, int port)
{
    int sleep_factor = 1;
    int socketWanClient = -1;

	while(*running)
	{
	   socketWanClient = create_tcp_client(host,port);
	   if(socketWanClient >= 0)
	   	return socketWanClient;

	   if(!is_network_accessible()){
	   		sleep_factor = 6;
	   }else{
	   		if(sleep_factor < 3000)
				sleep_factor *=2;
	   }

	   printf("try_tcp_request %s:%d, sleep %d!\n", host, port, 10*sleep_factor);
	   sleep(10* sleep_factor);
	}
	return -1;
}

int start_register(int* running, char* host, int port, int* sockReturn)
{
	int resultCode = -1;
	int socketWanClient = -1;

	socketWanClient = create_tcp_client(host,port);
	if(socketWanClient < 0)
		return -1;
	resultCode = registerself(socketWanClient);
    if(resultCode == -1){
    	close(socketWanClient);
		return -1;
    }

    if(resultCode == 0){
    	close(socketWanClient);
    	return resultCode;
    }

	*sockReturn = socketWanClient;

    return resultCode;
}

int start_boot(int* running, char* host, int port, int* sockReturn)
{
	int resultCode = -1;
	int socketWanClient = -1;

	socketWanClient = create_tcp_client(host,port);
	if(socketWanClient < 0)
		return -1;
	resultCode = bootRegisterSelf(socketWanClient);
    if(resultCode == -1){
    	close(socketWanClient);
		return -1;
    }

    if(resultCode == 0){
    	close(socketWanClient);
    	return resultCode;
    }

    *sockReturn = socketWanClient;

    return resultCode;
}

int start_connecting(int* running, char* host, int port, int* sock)
{
	int resultCode = -1;

	resultCode = start_boot(running, host, port, sock);
	if(resultCode != 0){
		return resultCode;
	}

	resultCode = start_register(running, xb_conn.registerHost, xb_conn.registerPort, sock);

	if(resultCode != 0){
		return resultCode;
	}

	return resultCode;
}

int create_local_udp_server()
{
    int socket_desc;
    struct sockaddr_in server;
    int port = 41355;

    //Create socket
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
        return -1;
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure

    server.sin_family = AF_INET;
    //server.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, "127.0.0.1", &(server.sin_addr));
    server.sin_port = htons( port );

    int on=1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind udp failed. Error");
        close(socket_desc); 
        return -1;
    }

    return socket_desc;
}

char* pollingEventRequestMsg(int sock);
static int wakeup_socket = -1;

int start_polling_event_loop(int* running, char* host, int port, int* sockReturn)
{
	struct NetworkPort networkPort;
	int resultCode = -1;
	int socketWanClient = -1;
	

	socketWanClient = create_tcp_client(host,port);
	if(socketWanClient < 0)
		return -1;

	tcp_set_timout(socketWanClient, 30);

    int so_keepalive=1;
    setsockopt(socketWanClient, SOL_SOCKET, SO_REUSEADDR, &so_keepalive, sizeof(so_keepalive));

	char* msg = pollingEventRequestMsg(socketWanClient);

  	if(tcp_send(socketWanClient,msg,strlen(msg)) <= 0)
  	{   
   	 printf("************tcp_send failed\n");
    	close(socketWanClient);
    	return -1;
  	}

	*sockReturn = socketWanClient;

	networkPort.sendback = port_send;
	networkPort.socket = socketWanClient;
	networkPort.packetFlag = 1;

    start_wanclient_recv_polling(&networkPort, wakeup_socket, running, msg);
    
    free(msg);
    return resultCode;
}

sem_t product_number;
static int my_next_event = 0;  // 1, reconnect, 2:polling event.
static int my_next_event_source = 0;  // 1, for reconnect, 2, just after boot stage. 3, heartbeat, 4, calendar task.

static int my_running = 1;
static int my_reconnecting = 0;
static int local_server_socket = -1;

void notify_event(int event, int source)
{	
	my_next_event = event;
	my_next_event_source = source;
	sem_post(&product_number); 
}

void force_close_local_server_socket()
{
	if(local_server_socket != -1){
		printf("close local_server_socket %d\n", local_server_socket);
		shutdown(local_server_socket, SHUT_RDWR); 
		close(local_server_socket);
	}
}

static void force_close_main_working_socket()
{
	if(xb_conn.current_connect_socket != -1){
		printf("close main socket %d\n", xb_conn.current_connect_socket);
		close(xb_conn.current_connect_socket);
	}
}

void notify_reconnect()
{	
	my_reconnecting = 1;

	force_close_main_working_socket();

	udp_send_msg_only("127.0.0.1", 41355, "x");

	stop_period_task("Heartbeat");
	notify_event(EVENT_RECONNECT, RECONNECT_REQUEST); 
}

static void * runLocalListener(void *ptr)
{
	int resultCode = -1;
	while(my_running){
		resultCode = start_local_receive(41355, &my_running, &local_server_socket); //port 41355
		local_server_socket = -1;
		if(resultCode == 0){
			notify_reconnect(); 
		}

		if(xb_conn.policy == 1){
			break;
		}

		sleep(5);
	}
	printf("local thread exit\n");
	
}

static void copy_default_address(void)
{
	strcpy(xb_conn.registerHost, xb_conn.host); 
    xb_conn.registerPort = xb_conn.port;
	strcpy(xb_conn.reportHost, xb_conn.host); 
    xb_conn.reportPort = xb_conn.port;
    strcpy(xb_conn.pollingEventHost, ""); 
    xb_conn.pollingEventPort = 0;
    xb_conn.pollingAction = 0;
    xb_conn.pollingTimeOut = 100;
    xb_conn.HeartbeatInterval = 28;
    
}

static int my_ini_handler(void* user, const char* section, const char* name,
                   const char* value)
{
    connect_method* pconfig = (connect_method*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    
    if (MATCH("global", "channel")) {
        strncpy(pconfig->channel, value, sizeof(pconfig->channel) -1 );
    } else {
        //return 0;  /* unknown section/name, error */
    }

    if (MATCH("global", "sw_version")) {
        strncpy(pconfig->sw_version, value, sizeof(pconfig->sw_version) -1);
    } else {
        //return 0;  /* unknown section/name, error */
    }

    return 1;
}

int loadconfig_file(const char* file_path)
{

    if (ini_parse(file_path, my_ini_handler, &xb_conn) < 0) {
        printf("Can't load 'test.ini'\n");
        return 1;
    }
    printf("[config] channel %s\n", xb_conn.channel);

    return 0;

}

static void retrieveCommonSysInfo(void)
{
    /* get monotonic clock time */
    struct timespec monotime;
    clock_gettime(CLOCK_MONOTONIC, &monotime);
    xb_conn.uptime = monotime.tv_sec;
}

static long updateRegisterTime(void)
{
    /* get monotonic clock time */
    struct timespec monotime;
    clock_gettime(CLOCK_MONOTONIC, &monotime);
    xb_conn.register_time = monotime.tv_sec;
}

extern "C" int xb_main(int argc, char ** argv)
{
	int run = 1;
	int resultCode = -1;
    int sleep_factor = 1;
    struct NetworkPort networkPort;
    pthread_t pid;
    char buf[32];
    
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction( SIGPIPE, &sa, 0 );

    //signal(SIGPIPE, SIG_IGN);

    signal(SIGTERM,terminate_handle);
 
 	retrieveSysConfigure(); 
 	
 	retrieveCommonSysInfo();

 	parseArgs(argc, argv);
    
    copy_default_address();

    //sniffer_initialize();
    sem_init(&product_number, 0, 0);  
    pthread_create(&pid, NULL, runLocalListener, NULL); 

	wakeup_socket = create_local_udp_server();
	fcntl(wakeup_socket, F_SETFL, O_NONBLOCK);

	while(my_running)
	{
		xb_conn.current_connect_state = -3;

		//wait wan_ip
		int count = 0;
		const char* wanIP = getWanIP();
		while (strlen(wanIP) == 0 || strcmp(wanIP, "0.0.0.0") == 0){
			wanIP = getWanIP();
			sleep(5);
			count++;
			if(count > 11)
				break;
		}
		
		copy_default_address();
		while (read(wakeup_socket, &buf, 32) > 0);

		my_reconnecting = 0;
		xb_conn.current_connect_state = -2;
		xb_conn.current_sleep_count = sleep_factor;
		xb_conn.current_connect_socket = -1;

		resultCode = start_connecting(&my_running, xb_conn.host,xb_conn.port, &xb_conn.current_connect_socket);
		xb_conn.current_connect_state = resultCode;
		
		updateRegisterTime();

		if(resultCode == 0){
			if(my_running == 1 && my_reconnecting == 0 && strlen(xb_conn.pollingEventHost) > 0){
				xb_conn.current_connect_socket = -1;
				xb_conn.current_connect_state = 1; //polling status
				start_polling_event_loop(&my_running, xb_conn.pollingEventHost,xb_conn.pollingEventPort, &xb_conn.current_connect_socket);
			}


			if(my_running == 1 && my_reconnecting == 0 && strlen(xb_conn.heartBeatHost) > 0){
				xb_conn.current_connect_socket = -1;
				xb_conn.current_connect_state = 2; //heartbeat status
				start_heart_beat_task(xb_conn.heartBeatHost, xb_conn.heartBeatPort, xb_conn.HeartbeatInterval);
			}

			int event, source;
			do{
				sem_wait(&product_number);  
				event = my_next_event;
				source = my_next_event_source;

				if(event == EVENT_POLLING){
					xb_conn.current_connect_socket = -1;
					xb_conn.current_connect_state = 1; //polling status					
					start_polling_event_loop(&my_running, xb_conn.host,xb_conn.port, &xb_conn.current_connect_socket);
					if(my_reconnecting == 1)
						break;
					if(source == HEARTBEART_REQUEST){
						xb_conn.current_connect_socket = -1;
						xb_conn.current_connect_state = 2; //heartbeat status
						start_heart_beat_task(xb_conn.heartBeatHost, xb_conn.heartBeatPort, xb_conn.HeartbeatInterval);
					}
				}
			}while(my_running && event != EVENT_RECONNECT);

			printf("will reconnect again\n");
			continue;
		}else if(resultCode == 10000){
			networkPort.sendback = port_send;
			networkPort.socket = xb_conn.current_connect_socket;
			networkPort.packetFlag = 1;
			start_wanclient_recv(&networkPort, wakeup_socket, &my_running);
			xb_conn.current_connect_socket = -1;
			resultCode = 0;
		}else if(resultCode == 9999){
			my_running = 0;
			if(local_server_socket != -1){
				printf("close local_server_socket %d\n", local_server_socket);
				shutdown(local_server_socket, SHUT_RDWR); 
				close(local_server_socket);
			}
			break;
		}else {
			// network error.
		}

		if(!is_network_accessible()){
	   		sleep_factor = 6;
	   }else{
	   		if(sleep_factor < 3000)
				sleep_factor *=2;
	   }
		
		//check policy 
		if(xb_conn.policy == 1){
			my_running = 0;
			if(local_server_socket != -1){
				printf("close local_server_socket %d\n", local_server_socket);
				shutdown(local_server_socket, SHUT_RDWR); 
				close(local_server_socket);
			}
			break;
		}

		sleep(xb_conn.retry_interval* sleep_factor);
	}

    if(wakeup_socket != -1)
    	close(wakeup_socket);

	pthread_join(pid, NULL);
    
	printf("main thread done!!!!!!\n");
	return 0;
}
