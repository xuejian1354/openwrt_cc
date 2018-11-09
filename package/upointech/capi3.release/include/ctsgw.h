/************************************************************************************************/
/*																								*/
/* Copyright (c) 2015-2016	Shanghai Upointech Information Techology Co. All Rights Reserved.	*/
/*																								*/
/*				上海有点信息科技有限公司	   版权所有 2015-2016								*/
/*											   													*/
/* This program is the proprietary software of Upointech Co., and may only be used,				*/
/* duplicated, modified or distributed pursuant to the terms and conditions of a separate,		*/
/* written license agreement executed between you and Upointech(an "Authorized License").		*/
/* The recipient of this software implicitly accepts the terms of the license.					*/
/*																								*/
/* 本程序的版权属于上海有点信息科技有限公司，任何人士阅读、使用、复制、修改或者发行都必			*/
/* 须获得相应的书面授权,承担保密责任和接受相应的法律约束.										*/
/*																					   			*/
/************************************************************************************************/
#ifndef CTSGW_H
#define CTSGW_H

#include "capi-util.h"
#include "capi-sys-service.h"
#include "capi-dbus-service.h"

#define MAX_LENGTH  (4096)

#ifdef __cplusplus
extern "C" {
#endif

#define STR_SUCCESS     "OK"
#define STR_FAIL        "ERROR"

/* LAN device connection type */
typedef enum {
    CT_LAN_DEV_CONN_WIRED,      // 0
    CT_LAN_DEV_CONN_WLAN,       // 1
    CT_LAN_DEV_CONN_UNKNOWN,    // 2
} CtLanDevConnType;

/* LAN device type */
typedef enum {
    CT_LAN_DEV_UNKNOWN,
    CT_LAN_DEV_PHONE,
    CT_LAN_DEV_PAD,
    CT_LAN_DEV_PC,
    CT_LAN_DEV_STB,
    CT_LAN_DEV_ROUTER,
    CT_LAN_DEV_SMTDEV,
    CT_LAN_DEV_AP,
    CT_LAN_DEV_PLC,
    CT_LAN_DEV_OTHER
} CtLanDevType;

/* Wireless Security Mode */
typedef enum {
    CT_WL_AUTH_OPEN,
    CT_WL_AUTH_SHARED,
    CT_WL_AUTH_WPA_PSK,
    CT_WL_AUTH_WPA2_PSK,
    CT_WL_AUTH_WPA2_PSK_MIX
} CtWlanAuthMode;

typedef struct {
    char            devName[STR_LEN_136];
    char            devType[STR_LEN_24];
    //CtLanDevType    devType;
    char            macAddr[STR_LEN_24];
    char            ipAddr[STR_LEN_24];
    CtLanDevConnType connType;
    int             port;
    char            brandName[STR_LEN_32];
    char            osName[STR_LEN_32];
    unsigned int    onlineTime;
    int             PowerLevel;
} CtLanDevInfo;

typedef struct {
    char user[MAX_LENGTH];
    char password[MAX_LENGTH];
} Account;


typedef enum {
    DAY=1,
}ControlCycleType;


typedef struct _StrongTimer
{
    int weekDay[7];
    char setTm[10];
    int nactive;
    int nEnable;
}StrongTimer;


/************************************************************************************************
 *                                                                                              *
 *  Function declaration                                                                        *
 *                                                                                              *
 ************************************************************************************************/

int ctSgw_lanGetDevBandwidth(const unsigned char *macAddr, int *usBandwidth, int *dsBandwidth, char *status);
int ctSgw_lanSetDevMaxBandwidth(const unsigned char *macAddr, int usBandwidth, int dsBandwidth, char *status);
int ctSgw_lanGetDevInfo(CtLanDevInfo **devInfoList, int *devNum, char *status);
int ctSgw_lanUpdateDevInfo(const CtLanDevInfo *devInfo, char *status);
int ctSgw_lanGetDevInternetAccessBlacklist(unsigned char **macAddrList, int *macAddrNum, char *status);
int ctSgw_lanGetDevStorageAccessBlacklist(char **macAddrList, int *macAddrNum, char *status);
int ctSgw_wanGetPPPoEAccount(char *pppoeAccount, char *status);
int ctSgw_lanSetInternetAccessPermission(const char *macAddr, int internetAccess, char *status);
int ctSgw_lanSetStorageAccessPermission(const char *macAddr, int storageAccess, char *status);
int ctSgw_sysGetCpuUsage(int *percent, char *status);
int ctSgw_sysGetMemUsage(int *percent, char *status);
int ctSgw_sysGetLoid(char *loid, char *status);
int CtSgwSetLEDTimer(CtSgwLED_t *obj);
int ctSgw_getLanIpAddr(char *lanIpAddr, char *lanSubnetMask, char *ipv6LanAddr, char *status);
int ctSgw_getWanIpAddr(char *wanIpAddr, char *wanSubnetMask, char *ipv6WanAddr, char *status);
int ctSgw_sysGetMac(char *mac, char *status);
int ctSgw_sysGetSSN(char *ssn, int ssnLength, char *status);
int ctSgw_sysSetSSN(const char *ssn, char *status);
int ctSgw_wlanGetSecurityCfg(int ssidIndex, int *enable, CtWlanAuthMode *authMode, char *ssid, char *password, char *status);
int ctSgw_wps(int ssidIdx, int on1OrOff0, int time, char *status);
int ctSgw_setLanDeviceFlow(int enable, char *status);
//int ctSgw_setAccountManage(Account *ftp, Account *samba, char *httpPassword, char *status);
int ctSgw_setAccountManage(int *ftpEnable, int *ftpService, int *sambaEnable, int *sambaService, int *httpEnable, Account *ftp, int *ftpNumber, Account *samba, int *sambaNumber, char *httpPassword, char *status);
//int ctSgw_plugGetParameter(Parameter *parameter, char *status);
//int ctSgw_plugSetParameter(Parameter *parameter, char *status);
int ctSgw_setOSRight(int cpu, int flash, int ram, int socket, int port, int thread, char *status);
int ctSgw_sysGetMfName(char *mfName, char *status);
int ctSgw_sysGetSoftwVer (char *SoftVer, char *status);
int ctSgw_sysGetHardwVer (char *hardVer, char *status);
int ctSgw_sysGetModel (char *pModel, char *status);
int ctSgw_sysGetCPUInfo(char * cpuVendor,char *cpuModel, char *status);
int ctSgw_sysGetDevModel (char *pdevModel, char *status);
int ctSgw_sysGetDevType (char *pdevType,  char *status);
int ctSgw_sysGetCap (char *pgwCap,char *status);
int ctSgw_sysGetUplink(int *plink, char *status);
int ctSgw_sysGetIpv6(int *pipv6, char *status);
int ctSgw_sysGetVLanBind(int *pbind, char *status);
int ctSgw_sysGetFlashSize (unsigned int *pFlashSize, char *status);
int ctSgw_sysGetRamSize (unsigned int *pRamSize, char *status);
int ctSgw_sysGetDevName (char *pdevName, char *maclist, char *status);
int ctSgw_sysGetWifiMode(char * pwifiModel, char *status);
int ctSgw_sysGetWanStatus (int *pwan, char *status);
int ctSgw_sysGetLanStatus (int *lan1, int *lan2, int *lan3, int *lan4, char *status);
int ctSgw_sysGetWifiModuleStatus (int *pwifiModule, char *status);
int ctSgw_sysGetPPPoEStatus (int *Ipv4ConnctionStatus, char *Ipv4WanStatus, char *Ipv4DialReason, int *Ipv6ConnctionStatus,char *Ipv6WanStatus, char *Ipv6DialReason, char *status);
int ctSgw_sysGetWanLinkNum (int *pNum, char *status);
int ctSgw_sysGetWanLinksDesc (char *pwanLinksDesc, char *status);
int ctSgw_sysGetWanBaseInfo (char *pwanLinkName,int *pindex,char *pifName,char *pserviceList,char *pconnType,char *pvlanID,char *p8021p,char *pprotocol,char *status);
int ctSgw_sysGetWanIpInfo (const char *pwanLinkName,char * pconnStatus,char *pipAddr,char *psubNetMask,char *pgateway,char *pdns1,char *pdns2,char *status);
int ctSgw_sysGetWanIpv6Info (const char *pwanLinkName,char * pconnStatus,char *pipAddr,int *pprifixLen,char *pgateway,char *pdns1,char *pdns2,char *pprifix,char *status);
int ctSgw_sysGetSSIDInfo (int ssidIndex,char *pssidName,char *ppwd,int *pcryptType,int *ppowerLeve,int *pchannel,int *penable, char *status);
int ctSgw_sysGetvoipInfo (char *pvoipName1,char *pvoipName2, char *status);
int ctSgw_sysGetVoipRegStatus (char *pline1Status,char * pline2Status,char *status);
int ctSgw_sysLOIDRegistry (const char *pusr,const char *ppwd, char *status);
int ctSgw_sysGetDevRegisterStatus (char *pregisterStatus,int * pRegisterResult,char *pregBussNameResult, char *status);
int ctSgw_sysGetPONStatus(double *ptemp,double *pvot,double *pcur,double *ptxpw,double *prxpw, char *status);
int ctSgw_sysGetPONRegStatus(char *pstatus, char *status);
int ctSgw_sysSetDateTime(const char *pTime, char *status);
int ctSgw_sysGetWifiEnable(int *pEnable, char *status);
int ctSgw_sysGetWanRecv (unsigned long long *pnRecvbytes, unsigned long long *pnRecvPkgs, unsigned long long *pnWrongPkgs, unsigned long long *pnLostPkgs, char *status);
int ctSgw_sysGetWanSend (unsigned long long *pnSendbytes, unsigned long long *pnSendPkgs, unsigned long long *pnWrongPkgs, unsigned long long *pnLostPkgs, char *status);
int ctSgw_sysSetSSIDSettings (int ssidIndex,const char *pssidName, const char *ppwd, int cryptType,int powerLeve,int channel,int enable, char *status);
int ctSgw_sysCloseSSID (int ssidIndex, char *status);
int ctSgw_sysSetWifiTimer (const char * pStartTime,const char * pEndTime, ControlCycleType ControlCycle,int Enable);
int ctSgw_sysSetApiRight (const char *pApi,int right, char *status);
int ctSgw_sysGetSystemDuration (int *duration, char *status);
int ctSgw_sysGetPPPOEDuration (int *duration, char *status);
int ctSgw_sysGetPONDuration (int *duration, char *status);
int ctSgw_sysGetBussinessStatus (char *pstatus, char *status);
int ctSgw_sysReboot(char *status);
int ctSgw_sysRestoreNotPrimeParm(char *status);
int ctSgw_sysGetTopInfo(char *ptop);
int ctSgw_sysGetLOID(char *ploid, char *status);
int ctSgw_sysCheckPwd(const char *pw, char *status);
int ctSgw_sysTrigerFwUpdate(const char *purl,int mode,int method, int needreboot) ;
int ctSgw_sysGetLanFlow(double *pport1Rx, double *pport1Tx, double *pport2Rx, double *pport2Tx, double *pport3Rx, double *pport3Tx, double *pport4Rx, double *pport4Tx, char *status);
int ctSgw_sysGetPPPoEFailReason(const char *pusr, const char *ppwd, const char *pvpnAcc, const char *pvpnPwd, int *pconnStatus,char *pwanStatus,int len1,char *pdialReason,int len2, char *status);
int ctSgw_sysGetWifiTimerInfo (char *pStartTime,char *pEndTime, ControlCycleType *pControlCycle,int *pEnable);
int ctSgw_sysGetSleepStatus (int * pnStatus, char *status);
int ctSgw_sysGetLedStatus (int *pled, char *status);
int ctSgw_sysSetLedTimer (char * pStartTime,char * pEndTime, char * ControlCycle,int Enable, char *status);
int ctSgw_sysSetDns(const char* pdns1,const char* pdns2, char *status);
int ctSgw_sysAddObject(char* pObject, char *status);
int ctSgw_sysDelObject(const char* pObject, char *status);
int ctSgw_sysGetBlackList (char *pmaclist, char *status);
int ctSgw_sysSetLedStatus(char* nStatus, char *status);
int ctSgw_sysSetSleepTimer(int Action, StrongTimer sleepTimer[], int nCount, char *status);
int ctSgw_sysSetStrongWifiTimer(int Action, StrongTimer wifiTimer[], int nCount, char *status);
int ctSgw_sysSetDevName (const char *pdevName, char *status);
int ctSgw_sysGetDNS (char *pdns1,char *pdns2, char *status);
int ctSgw_sysGetSleepTimer(StrongTimer *sleepTimer, int nCount, char *status);
int ctSgw_sysGetStrongWifiTimer(StrongTimer *sleepTimer, int nCount, char *status);
int ctSgw_lanGetDevMaxBandwidth(const unsigned char *pmacAddr, int *pusBandwidth, int *pdsBandwidth, char *status);
int ctSgw_sysBootSSID(int ssidIndx, char *status);
int ctSgw_sysGetSN(char *psn, char *status);
int ctSgw_sysGetOsVer(char * pOsVer, char *status);

#ifdef __cplusplus
}
#endif

#endif
