//gcc libctsgw.c -o libctsgw.so -fPIC -shared -Wall
#include <string.h>
#include "ct_sgw_api.h"
#include <stdio.h>
#include <stdlib.h>

char ssnRecord[64];

int ctSgw_init(int *fd) {
  fd = 0;
  bzero(ssnRecord, 64);
  strcpy(ssnRecord, "");
//strcpy(ssnRecord, "01234567012345670123456701234567");
//strcpy(ssnRecord, "5C2F481126E-123456789ABC");
  return 0;
}

int ctSgw_cleanup(void) {
  return 0;
}

int ctSgw_sysGetIpAddr(CtSysIpAddrCfg *ctSysIpAddrCfg) {
  ctSysIpAddrCfg->lanPrefixLen = 1;
  ctSysIpAddrCfg->wanPrefixLen = 0;
  strcpy(ctSysIpAddrCfg->ipv6LanAddr, "");
  strcpy(ctSysIpAddrCfg->ipv6WanAddr, "");
  strcpy(ctSysIpAddrCfg->lanIpAddr, "192.168.1.1");
  strcpy(ctSysIpAddrCfg->lanSubnetMask, "255.255.255.0");
  strcpy(ctSysIpAddrCfg->wanIpAddr, "200.16.108.8");
  strcpy(ctSysIpAddrCfg->wanSubnetMask, "255.255.255.0");
  return 0;
}

int ctSgw_sysGetMac(unsigned char mac[6]) {
  mac[0] = 0x12;
  mac[1] = 0x34;
  mac[2] = 0x56;
  mac[3] = 0x78;
  mac[4] = 0x9A;
  mac[5] = 0xBC;
  if (strlen((char *)mac) > 6) {
    mac[6] = 0x00;
  }
  return 0;
}

int ctSgw_sysGetSSN(char *ssn, int ssnLen) {
  strncpy(ssn, ssnRecord, ssnLen);
  return 0;
}

int ctSgw_sysSetSSN(const char *ssn) {
  strcpy(ssnRecord, ssn);
  return 0;
}

int ctSgw_wlanGetSecurityCfg(CtWlanSecurityCfg *ctWlanSecurityCfg) {
  ctWlanSecurityCfg->enable = 1;
  ctWlanSecurityCfg->authMode = CT_WL_AUTH_WPA2_PSK_MIX;
  strcpy(ctWlanSecurityCfg->ssid, "ChinaNet-0001");
  strcpy(ctWlanSecurityCfg->password, "12345678");
  return 0;
}

/*
  refer ctSgw.c 
*/

int ctSgw_wanGetPppoeAccount(char *pppoeAccount, int len){

  memcpy(pppoeAccount, "123456789", 13);
  return 0;  
}

int ctSgw_sysGetCpuUsage(int *percent){
  *percent = 50;
  return 0;  
}

int ctSgw_sysGetMemUsage(int *percent){
  *percent = 30;
  return 0;  
}

static void printf_mac(const CtMacAddr *macAddr){
  printf("%s,mac:%02X%02X%02X%02X%02X%02X, begin\n",__FUNCTION__,macAddr->macAddr[0],
  macAddr->macAddr[1],macAddr->macAddr[2],macAddr->macAddr[3],macAddr->macAddr[4],macAddr->macAddr[5]);
}

int ctSgw_lanGetDevBandwidth(const CtMacAddr *macAddr, int *usBandwidth, int *dsBandwidth)
{
  printf_mac(macAddr);
  *usBandwidth = 1024;
  *dsBandwidth = 7092;

  return 0;
}

int ctSgw_wanGetIfStats(int *usStats, int *dsStats)
{
  *usStats = 1024;
  *dsStats = 7092;
  return 0;
}

int ctSgw_lanSetDevMaxBandwidth(const CtMacAddr *macAddr, int usBandwidth, int dsBandwidth){
  printf_mac(macAddr);

  return 0;
}

 int ctSgw_lanGetDevInfo(CtLanDevInfo **devsList, int *devNum){

   CtLanDevInfo *devs = NULL;
   
   *devNum = 3;
 
   devs = (CtLanDevInfo *)malloc(sizeof(CtLanDevInfo) * 3 + 100);
   
   *devsList = devs;
  memset(devs,0,sizeof(CtLanDevInfo) * 3 + 100);
  strcpy(devs[0].devName,"usr-pc");
  printf("%s,dev:%s\n",__FUNCTION__,devs[0].devName);
  devs[0].macAddr[0] = 63;
  devs[0].macAddr[1] = 63;
  devs[0].macAddr[2] = 114;
  devs[0].macAddr[3] = 63;
  devs[0].macAddr[4] = 112;
  devs[0].macAddr[5] = 64;
  memcpy(devs[0].ipAddr,"192.168.1.11",CT_IP_ADDR_STR_MAX_LEN);
  memcpy(devs[0].brandName,"lenov",CT_LAN_DEV_BRAND_NAME_MAX_LEN);
  memcpy(devs[0].osName,"windows",CT_LAN_DEV_OS_NAME_MAX_LEN);
  devs[0].devType = 3;
  devs[0].connType = 2;
  devs[0].port = 3025;
  devs[0].onlineTime = 358612;

  memcpy(devs[1].devName,"xxxxxx",64);
  printf("%s,dev:%s\n",__FUNCTION__,devs[1].devName);
  devs[1].macAddr[0] = 63;
  devs[1].macAddr[1] = 62;
  devs[1].macAddr[2] = 66;
  devs[1].macAddr[3] = 100;
  devs[1].macAddr[4] = 125;
  devs[1].macAddr[5] = 64;
  memcpy(devs[1].ipAddr,"192.168.1.101",64);
  memcpy(devs[1].brandName,"ace",64);
  memcpy(devs[1].osName,"windows",64);
  devs[1].devType = 3;
  devs[1].connType = 1;
  devs[1].port = 3027;
  devs[1].onlineTime = 458612;
  
  memcpy(devs[2].devName,"yyyyyyyyyy",64);
  printf("%s,dev:%s\n",__FUNCTION__,devs[2].devName);
  devs[2].macAddr[0] = 65;
  devs[2].macAddr[1] = 67;
  devs[2].macAddr[2] = 68;
  devs[2].macAddr[3] = 60;
  devs[2].macAddr[4] = 63;
  devs[2].macAddr[5] = 64;
  memcpy(devs[2].ipAddr,"192.168.1.111",64);
  memcpy(devs[2].brandName,"",64);
  memcpy(devs[2].osName,"linux",64);
  devs[2].devType = 3;
  devs[2].connType = 1;
  devs[2].port = 3029;
  devs[2].onlineTime = 5458612;
  printf("%s finish\n",__FUNCTION__);
   return 0;
}

int ctSgw_lanUpdateDevInfo(const CtLanDevInfo *devInfo){
   printf("%s,devName:%s\n",__FUNCTION__,devInfo->devName);
   
 
   return 0;
}

int ctSgw_lanGetDevInternetAccessBlacklist(CtMacAddr **macAddrList, int *macAddrNum){

  *macAddrNum = 2;
  *macAddrList = (CtMacAddr *)malloc(2 * 6 + 1);
  unsigned char (* test)[6] = (unsigned char (*)[6])(*macAddrList);
   
  test[0][0] = 75;
  test[0][1] = 67;
  test[0][2] = 68;
  test[0][3] = 60;
  test[0][4] = 63;
  test[0][5] = 64;
  
  test[1][0] = 85;
  test[1][1] = 67;
  test[1][2] = 68;
  test[1][3] = 60;
  test[1][4] = 63;
  test[1][5] = 64;

  return 0;
}

int ctSgw_lanGetDevStorageAccessBlacklist(CtMacAddr **macAddrList, int *macAddrNum){

  *macAddrNum = 2;
  *macAddrList = (CtMacAddr *)malloc(2 * 6 + 1);
  unsigned char (* test)[6] = (unsigned char (*)[6])(*macAddrList);
   
  test[0][0] = 75;
  test[0][1] = 67;
  test[0][2] = 68;
  test[0][3] = 60;
  test[0][4] = 63;
  test[0][5] = 64;
  
  test[1][0] = 85;
  test[1][1] = 67;
  test[1][2] = 68;
  test[1][3] = 60;
  test[1][4] = 63;
  test[1][5] = 64;

  return 0;
}

int ctSgw_lanSetInternetAccessPermission(const CtMacAddr *macAddr, int internetAccess){

   return 0;
}

int ctSgw_lanSetStorageAccessPermission(const CtMacAddr *macAddr, int storageAccess){

  return 0;
}

int ctSgw_sysPushWeb(int enable, int width, int height, int place, const char *url, int time){

  return 0;
}

int ctSgw_lanSetDevStatsStatus(int enable)
{
  return 0;
}

int ctSgw_wlanSetState(int ssidIndex, int enable)
{
  return 0;  
}

int ctSgw_lanSetDevAccessPermission(const CtMacAddr *macAddr, int internetAccess, int storageAccess)
{
  return 0;  
}

int ctSgw_handleEvent(CtEventType *eventType, void **eventInfo)
{
  return 0;
}

int ctSgw_lanGetPortStatus(int *portStatus)
{
  return 0;
}