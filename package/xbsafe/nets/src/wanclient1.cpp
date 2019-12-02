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

#include "dl.h"
#include "log.h"
#include "ct_sgw_api.h"
#include "ctsgwX.h"
#include "proxyclient.h"
#include "wanclient.h"

#if defined(GATEWAY_1_0)


static char information[256];


extern "C" int xb_main(int argc, char ** argv);

void handle_get_net_lan_info(duk_context *ctx, duk_idx_t indx0) 
{
    CtLanDevInfo *info= NULL;
    int num=0;
    int ret=0;
    char buf[128];

    ret = ctSgw_lanGetDevInfo(&info, &num);
    
    printf("ret %d, num %d\n", ret, num);

  duk_idx_t array_idx;
  duk_idx_t array_obj;

    putInt(ctx,indx0,"Result",ret);

  if(info == NULL)
    return;

  putArrayProp(ctx,indx0,"Info");
  array_idx = getObj(ctx,indx0,"Info");

  for(int i=0; i< num; i++)
  {
    putArrayIndexObj(ctx,array_idx,i,&array_obj);
    putString(ctx,array_obj,"DevName",info[i].devName); //info[i].devName
    switch(info[i].devType){
        case CT_LAN_DEV_UNKNOWN:
          putString(ctx,array_obj,"DevType","OTHER");
          break;
        case CT_LAN_DEV_PHONE:
          putString(ctx,array_obj,"DevType","Phone");
          break;
        case CT_LAN_DEV_PAD:
          putString(ctx,array_obj,"DevType","Pad");
          break;
        case CT_LAN_DEV_PC:
          putString(ctx,array_obj,"DevType","PC");
          break;
        case CT_LAN_DEV_STB:
          putString(ctx,array_obj,"DevType","STD");
          break;
        case CT_LAN_DEV_OTHER:
          putString(ctx,array_obj,"DevType","OTHER");
          break;
    }
    
      sprintf(buf, "%02X%02X%02X%02X%02X%02X", info[i].macAddr[0], info[i].macAddr[1], info[i].macAddr[2], 
            info[i].macAddr[3], info[i].macAddr[4], info[i].macAddr[5]);

    putString(ctx,array_obj,"MAC",buf);
    
    putString(ctx,array_obj,"IP",info[i].ipAddr);
    switch(info[i].connType){
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

    switch(info[i].port){
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
    putString(ctx,array_obj,"Brand",info[i].brandName);
    putString(ctx,array_obj,"OS",info[i].osName);
    putInt(ctx,array_obj,"OnlineTime",info[i].onlineTime);

    jsonTmpObjRemove(ctx,array_obj);    
  }
  jsonTmpObjRemove(ctx,array_idx);  

  free(info);
}

void BuildPortStatusObj(duk_context *ctx, duk_idx_t indx)
{
  int val=0;
  int sgwRet = 0;
  sgwRet = ctSgw_lanGetPortStatus(&val);

  putInt(ctx,indx,"Result",sgwRet);

  char* names[6] = {"LAN1Status", "LAN2Status", "LAN3Status", "LAN4Status", "WANStatus", "WIFIModuleStatus" };
  for(int i = 0; i< 6; i++){
    int bOn =  (val & (1 << i)) != 0;
    const char* strOnOff = bOn ? "ON" : "OFF";
    putString(ctx, indx, names[i], strOnOff);
  }
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

  putString(ctx,indx,"WanIPAddr",getWanIP());
  putString(ctx,indx,"LanIPAddr",getLanIP());

  sgwRet = ctSgw_sysGetCpuUsage(&intVal);
  if(sgwRet != 0){
    putInt(ctx,indx,"Result",sgwRet);
    return;
  }
  putInt(ctx,indx,"CpuPercent",intVal);       

  sgwRet = ctSgw_sysGetMemUsage(&intVal);
  if(sgwRet != 0){
    putInt(ctx,indx,"Result",sgwRet);
    return;
  }
  putInt(ctx,indx,"MemPercent",intVal);
}

void BuildWiFiStatusObj(duk_context *ctx, duk_idx_t indx)
{
  int sgwRet = 0;
  int intVal = 0;

  CtWlanSecurityCfg securityCfg;

  memset(&securityCfg,0,sizeof(securityCfg));

  sgwRet = ctSgw_wlanGetSecurityCfg(&securityCfg);
  if(sgwRet != 0){
    putInt(ctx,indx,"Result",sgwRet);
    return;
  }

  putInt(ctx,indx,"WifiEnable",securityCfg.enable);
  putString(ctx,indx,"SSID",securityCfg.ssid);

}


int ctrl_mac_access(char* mac, int internetAccess)
{
    int ret;
    CtMacAddr macAddr;
    macStrAutoToMacNum((unsigned char *)mac, macAddr.macAddr);
    ret = ctSgw_lanSetDevAccessPermission(&macAddr, internetAccess, 0);
    printf("ctSgw_lanSetDevAccessPermission %s access %d, ret %d", mac, internetAccess, ret);
    return ret;
}

void handle_get_blacklist(duk_context *ctx, duk_idx_t indx0) 
{
    CtMacAddr* macAddrList = NULL;
    int macAddrNum = 0;
    int ret=0;
    int i = 0;
    char buf[256];

    ret = ctSgw_lanGetDevInternetAccessBlacklist(&macAddrList, &macAddrNum);
    printf("ctSgw_lanGetDevInternetAccessBlacklist num %d, ret %d", macAddrNum, ret);

  duk_idx_t array_idx;
  duk_idx_t array_obj;

  putInt(ctx,indx0,"Result",ret);

  putArrayProp(ctx,indx0,"MACList");
  array_idx = getObj(ctx,indx0,"MACList");

  for(i = 0; i< macAddrNum; i++)
  {
    putArrayIndexObj(ctx,array_idx,i,&array_obj);
    macNumToStr((char *)macAddrList[i].macAddr, buf);

    putString(ctx,array_obj,"MAC",buf); //info[i].devName

    jsonTmpObjRemove(ctx,array_obj);    
  }
  jsonTmpObjRemove(ctx,array_idx);  

  if(macAddrList != NULL)
      free(macAddrList);  
  
}



//////////////////////////////////////////////////////////////////////////////////////
static char gwMac[20];
static char gwAccount[128];
static char gwWanIP[64];
static char gwLanIP[64];
static char gwModel[64];
static char gwSWVersion[64];
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
  unsigned char  mac[32] = { 0 };
    
  if(strlen(gwMac) == 0)
  {
        ctSgw_sysGetMac(mac);
        sprintf(gwMac, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
  return gwMac;
}

const char * getAccount()
{
    
  //if(strlen(gwAccount) == 0)
  {
        ctSgw_wanGetPppoeAccount(gwAccount,sizeof(gwAccount));
  }
  return gwAccount;
}

const char * getModel()
{
#if 1    
  if(gw_model_flag == 0)
  {
        std::string swVersion;
        std::string model = tryGetModel(swVersion);
        snprintf(gwModel, sizeof(gwModel), "%s", model.c_str());
        snprintf(gwSWVersion, sizeof(gwSWVersion), "%s", swVersion.c_str());
        gw_model_flag = 1;
  }
  return gwModel;
#else
  return "";
#endif
}

const char * getGatewaySWVersion()
{
  return gwSWVersion;
}

const char * getGatewayHWVersion()
{
  return "";
}

const char * getGatewayProvince()
{
  return "";
}

const char * getWanIP()
{
  int iret = -1;
    CtSysIpAddrCfg ipConf;

    if(dbgWAN){
      printf("wan %s, lan %s\n", gwWanIP, gwLanIP);
      return gwWanIP;
    }

  bzero(&ipConf, sizeof ipConf);
  iret = ctSgw_sysGetIpAddr(&ipConf);
  if(iret == 0){
    printf("ctSgw wan %s, lan %s\n", ipConf.wanIpAddr, ipConf.lanIpAddr);
    strcpy(gwWanIP,ipConf.wanIpAddr);
  }

  return gwWanIP;
}

const char * getLanIP()
{
  int iret = -1;
    CtSysIpAddrCfg ipConf;

    if(dbgLAN){
      return gwLanIP;
    }

  bzero(&ipConf, sizeof ipConf);
  iret = ctSgw_sysGetIpAddr(&ipConf);
  if(iret == 0){
    printf("ctSgw2 wan %s, lan %s\n", ipConf.wanIpAddr, ipConf.lanIpAddr);
    strcpy(gwLanIP,ipConf.lanIpAddr);
  }

  return gwLanIP;
}

const char* getManufactor(void)
{
#if defined(SW_PLATFORM_FENGHUO)
  return "fenghuo";
#elif defined(SW_PLATFORM_BEIER)
  return "beier";
#elif defined(SW_PLATFORM_NBEIER)
  return "nbeier";
#elif defined(SW_PLATFORM_HUAWEI)
  return "huawei";
#elif defined(SW_PLATFORM_TIANYI)
  return "tianyi";
#elif defined(SW_PLATFORM_TONGWEI)
  return "tongwei";
#elif defined(SW_PLATFORM_YOUHUA)
  return "youhua";
#elif defined(SW_PLATFORM_ZHONGXING)
  return "zhongxing";
#else
  return "generic";
#endif
}

const char* getPluginName(void)
{
  return "xbnets";
}

void retrieveSysConfigure(void)
{

}


static int initialize() {
  int retVal = 0;

  retVal = ctsgwInitialize();
  if (0 != retVal) {
    return -1;
  }
/*
  retVal = openLogFile("../Temp/xbnets.log");
  bzero(information, 256);
  sprintf(information, "Version - %s\n", __DATE__);
  logOut(information, LEVEL_INFORMATION, __FILE__, __LINE__);
  retVal = ctsgwInitialize();
  if (0 != retVal) {
    return -1;
  }
  if (0 == deviceVersion()) {
    logOut("The Device Has SSN!\n", LEVEL_INFORMATION, __FILE__, __LINE__);
  }
  else {
    logOut("The Device Has Only SN!\n", LEVEL_INFORMATION, __FILE__, __LINE__);
  }
*/
  return retVal;
}

int main(int argc, char ** argv)
{
  int ret;

  if(initialize() != 0)
      return -1;
    
  memset(&xb_conn, 0, sizeof(xb_conn));
  strcpy(xb_conn.host, "qoe.xbsafe.cn"); //222.222.104.254 220.181.85.226 
  xb_conn.port = 61000;

  xb_conn.retry_interval = 10;
  xb_conn.policy = 0; 

  loadconfig_file("default_xbnets.ini");

  ret = xb_main(argc, argv);
    ctSgw_cleanup();

    printf(" gateway 1.0 main exit %d\n", ret);

  return ret;
}

#endif