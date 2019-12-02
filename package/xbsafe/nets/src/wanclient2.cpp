#include "wanclient.h"

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/sysinfo.h>
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

#include "duktape.h"
#include "json.h"

#include "proxyclient.h"

#if defined(GATEWAY_2_0)

#ifdef __cplusplus
extern "C"
{
#endif

#include "capi.h"    //no extern "C" in it

#ifdef __cplusplus
}
#endif


void handle_get_net_lan_info(duk_context *ctx, duk_idx_t indx0) 
{
    int num=0;
    int ret=0;
    char buf[128];
    guint32 result=0;
    char * errmsg = NULL;

  CtSgwLANHostSnapshot_t* info = NULL;

  ret = CtSgwGetOnlineDevicesSnapShot(&info, &num, &result, &errmsg);
  g_print("CtSgwGetNetworkInfo ret = %d\n", ret);


  if(errmsg != NULL){
    printf("CtSgwGetNetworkInfo errmsg %s\n", errmsg);
    g_free(errmsg);
  }

  duk_idx_t array_idx;
  duk_idx_t array_obj;

  putInt(ctx,indx0,"Result",ret);

  if(info == NULL || result != 0 || ret != CTSGW_OK)
    return;

  putArrayProp(ctx,indx0,"Info");
  array_idx = getObj(ctx,indx0,"Info");

  for(int i=0; i< num; i++)
  {
    putArrayIndexObj(ctx,array_idx,i,&array_obj);
    putString(ctx,array_obj,"DevName",info[i].DevName);
    putString(ctx,array_obj,"DevType",info[i].DevType);
    
    putString(ctx,array_obj,"MAC",info[i].MAC);
    
    putString(ctx,array_obj,"IP",info[i].IP);
    switch(info[i].ConnectionType){
      case CT_LAN_DEV_CONN_UNKNOWN:
        putInt(ctx,array_obj,"ConnectType",0);
        break;
      case CT_LAN_DEV_CONN_WIRED:
        putInt(ctx,array_obj,"ConnectType",0);
        break;
      case CT_LAN_DEV_CONN_WLAN:
        putInt(ctx,array_obj,"ConnectType",1);
        break;
    }

    switch(info[i].Port){
      case 0: //unknown
        putInt(ctx,array_obj,"Port",0);
        break;
      case 1:
        putInt(ctx,array_obj,"Port",1);
        break;
      case 2:
        putInt(ctx,array_obj,"Port",2);
        break;
      case 3:
        putInt(ctx,array_obj,"Port",3);
        break;
      case 4:
        putInt(ctx,array_obj,"Port",4);
        break;
      case 5:
      default:
        putInt(ctx,array_obj,"Port",0); //WIFI
        break;

    }
    putString(ctx,array_obj,"Brand",info[i].Brand);
    putString(ctx,array_obj,"OS",info[i].OS);
    putInt(ctx,array_obj,"OnlineTime",info[i].OnlineTime);

    jsonTmpObjRemove(ctx,array_obj);    
  }
  jsonTmpObjRemove(ctx,array_idx);  

  g_free(info);
}

void BuildPortStatusObj(duk_context *ctx, duk_idx_t indx)
{
  int val=0;
  int ret = 0;  
  const char* strOnOff = "OFF";

  CtSgwNetworkInfo_t net_info;

  memset(&net_info, 0, sizeof(CtSgwNetworkInfo_t));
  ret = CtSgwGetNetworkInfo(&net_info);
  g_print("CtSgwGetNetworkInfo ret = %d\n", ret);

  putInt(ctx,indx,"Result",ret);

  if(ret != CTSGW_OK){
    return;
  }

  strOnOff = net_info.LAN1Status ? "ON" : "OFF";
  putString(ctx, indx, "LAN1Status", strOnOff);

  strOnOff = net_info.LAN2Status ? "ON" : "OFF";
  putString(ctx, indx, "LAN2Status", strOnOff);

  strOnOff = net_info.LAN3Status ? "ON" : "OFF";
  putString(ctx, indx, "LAN3Status", strOnOff);

  strOnOff = net_info.LAN4Status ? "ON" : "OFF";
  putString(ctx, indx, "LAN4Status", strOnOff);

  strOnOff = net_info.WANLinkStatus[0] ? "ON" : "OFF";
  putString(ctx, indx, "WANStatus", strOnOff);

  strOnOff = net_info.WIFIModuleStatus[0] ? "ON" : "OFF";
  putString(ctx, indx, "WIFIModuleStatus", strOnOff);

}

void BuildBasicStatusObj(duk_context *ctx, duk_idx_t indx)
{
  int sgwRet = 0;
  int intVal = 0;

  struct sysinfo info;
  sysinfo(&info);

  putInt(ctx,indx,"SYSDuration",info.uptime);  

  putInt(ctx,indx,"PluginBootTime",xb_conn.uptime);  
  putInt(ctx,indx,"PluginRegisterTime",xb_conn.register_time); 

  CtSgwNetworkInfo_t net_info;

  memset(&net_info, 0, sizeof(CtSgwNetworkInfo_t));
  sgwRet = CtSgwGetNetworkInfo(&net_info);
  g_print("CtSgwGetNetworkInfo sgwRet = %d\n", sgwRet);

  if(sgwRet !=CTSGW_OK){
    putInt(ctx,indx,"Result",sgwRet);
    return;
  }

  putString(ctx,indx,"WanIPAddr",net_info.WANIPAddr);
  putString(ctx,indx,"LanIPAddr",net_info.LANIPAddr);

  CtSgwDeviceInfo_t dev_info;

  memset(&dev_info, 0, sizeof(CtSgwDeviceInfo_t));
  sgwRet = CtSgwGetDeviceInfo(&dev_info);
  g_print("CtSgwGetDeviceInfo sgwRet = %d\n", sgwRet);

  if(sgwRet !=CTSGW_OK){
    putInt(ctx,indx,"Result",sgwRet);
    return;
  }

  putInt(ctx,indx,"CpuPercent",dev_info.CPUUsage);       
  putInt(ctx,indx,"MemPercent",dev_info.MEMUsage);

}

void BuildWiFiStatusObj(duk_context *ctx, duk_idx_t indx)
{
  int sgwRet = 0;
  int intVal = 0;

  CtSgwWiFiInfo_t securityCfg;

  memset(&securityCfg,0,sizeof(securityCfg));

  sgwRet = CtSgwGetWiFiInfo(&securityCfg);
  if(sgwRet != CTSGW_OK){
    putInt(ctx,indx,"Result",sgwRet);
    return;
  }

  putInt(ctx,indx,"WifiEnable",securityCfg.Enable2G);
  putString(ctx,indx,"SSID", "");

}

int ctrl_mac_access(char* mac, int internetAccess)
{
    int ret = 0;

    return ret;
}

void handle_get_blacklist(duk_context *ctx, duk_idx_t indx0) 
{

  putInt(ctx,indx0,"Result",0);

}


//////////////////////////////////////////////////////////////////////////////////////
static char gwMac[20];
static char gwAccount[128];
static char gwWanIP[64];
static char gwLanIP[64];
static char gwModel[64];
static char gwSWVersion[64];
static char gwHWVersion[64];
static char gwManufactor[64];
static char gwProvince[16];

static char unused[64];    //prevent memory overflow.

static int gw_model_flag = 0;

static int  dbgWAN = 0;
static int  dbgLAN = 0;


int debug_mac(char *mac)
{
  stpcpy(gwMac,mac);
  return 0;
}

int debug_account(char *account)
{
  stpcpy(gwAccount,account);
}

int debug_wan(char *ip)
{
  dbgWAN = 1;
  stpcpy(gwWanIP,ip);
}

int debug_lan(char *ip)
{
  dbgLAN = 1;
  stpcpy(gwLanIP,ip);
}

const char * getMac()
{
  return gwMac;
}

const char * getAccount()
{
  int ret = 0;   
  CtSgwPPPoE_t pppoe;
  memset(&pppoe, 0, sizeof(CtSgwPPPoE_t));
  ret = CtSgwGetPPPoEConfig(&pppoe);
  g_print("CtSgwGetPPPoEConfig ret = %d\n", ret);
  if(ret ==CTSGW_OK){
    snprintf(gwAccount, sizeof(gwAccount), "%s", pppoe.PPPoEUserName);
  }

  return gwAccount;
}

const char * getModel()
{
  return gwModel;
}

const char * getGatewaySWVersion()
{
  return gwSWVersion;
}

const char * getGatewayHWVersion()
{
  return gwHWVersion;
}

const char * getGatewayProvince()
{
  return gwProvince;
}

const char * getWanIP()
{
  int ret = 0;

  CtSgwNetworkInfo_t net_info;

  memset(&net_info, 0, sizeof(CtSgwNetworkInfo_t));
  ret = CtSgwGetNetworkInfo(&net_info);
  g_print("CtSgwGetNetworkInfo ret = %d\n", ret);

  if(ret == CTSGW_OK){
    snprintf(gwWanIP, sizeof(gwWanIP), "%s", net_info.WANIPAddr);
    snprintf(gwLanIP, sizeof(gwLanIP), "%s", net_info.LANIPAddr);
  }  
  return gwWanIP;
}

const char * getLanIP()
{
  int ret = 0;

  CtSgwNetworkInfo_t net_info;

  memset(&net_info, 0, sizeof(CtSgwNetworkInfo_t));
  ret = CtSgwGetNetworkInfo(&net_info);
  g_print("CtSgwGetNetworkInfo ret = %d\n", ret);

  if(ret == CTSGW_OK){
    snprintf(gwWanIP, sizeof(gwWanIP), "%s", net_info.WANIPAddr);
    snprintf(gwLanIP, sizeof(gwLanIP), "%s", net_info.LANIPAddr);
  }  
  return gwLanIP;
}

const char* getManufactor(void){
  return gwManufactor;
}

const char* getPluginName(void)
{
#ifdef GW_CHIPSET
  #define MYPLUGIN "nets_" GW_CHIPSET
  return MYPLUGIN;
#else
  return "xbnets";
#endif  
}

void retrieveSysConfigure(void)
{
  int ret = 0;  
  CtSgwDeviceInfo_t dev_info;

  memset(&dev_info, 0, sizeof(CtSgwDeviceInfo_t));
  ret = CtSgwGetDeviceInfo(&dev_info);
  g_print("CtSgwGetDeviceInfo ret = %d\n", ret);

  if(ret ==CTSGW_OK){
    snprintf(gwManufactor, sizeof(gwManufactor), "%s", dev_info.Manufacturer);
    snprintf(gwMac, sizeof(gwMac), "%s", dev_info.MAC);

    snprintf(gwModel, sizeof(gwModel), "%s", dev_info.ProductClass);
    snprintf(gwSWVersion, sizeof(gwSWVersion), "%s", dev_info.SWVersion);
    snprintf(gwHWVersion, sizeof(gwHWVersion), "%s", dev_info.HDVersion);
    gw_model_flag = 1;

  }

  CtSgwSystemCmd_t syscmdObj;
  ret = CtSgwGetSystemCmdConfig(&syscmdObj);
  g_print("CtSgwGetSystemCmdConfig ret = %d\n", ret);

  if(ret ==CTSGW_OK){
    snprintf(gwProvince, sizeof(gwProvince), "%s", syscmdObj.Province);
  }

  CtSgwNetworkInfo_t net_info;

  memset(&net_info, 0, sizeof(CtSgwNetworkInfo_t));
  ret = CtSgwGetNetworkInfo(&net_info);
  g_print("CtSgwGetNetworkInfo ret = %d\n", ret);

  if(ret == CTSGW_OK){
    snprintf(gwWanIP, sizeof(gwWanIP), "%s", net_info.WANIPAddr);
    snprintf(gwLanIP, sizeof(gwLanIP), "%s", net_info.LANIPAddr);
  }

}

extern "C" void updateNetworkConfigure(void)
{
  
}

#endif