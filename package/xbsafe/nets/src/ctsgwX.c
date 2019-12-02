#include "ctsgwX.h"
#include "log.h"

static char information[256];

static int fdctsgw;

int ctsgwInitialize() {
  int retVal = 0;

  retVal = ctSgw_init(&fdctsgw);
  if (0 != retVal) {
    logOut("ctSgw_init() Failed!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  return 0;
}

int ctsgwGetIP(char *ip, int level, char *file, int line) {
  CtSysIpAddrCfg ctSysIpAddrCfg;
  int retVal = 0;

  logOut("", level, file, line);
  if (NULL == ip) {
    logOut("ip Is NULL!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  bzero(&ctSysIpAddrCfg, sizeof(ctSysIpAddrCfg));
  retVal = ctSgw_sysGetIpAddr(&ctSysIpAddrCfg);
  if (0 != retVal) {
    logOut("ctSgw_sysGetIpAddr() Failed!\n", LEVEL_ERROR, __FILE__, __LINE__);
  //FIXME
    strcpy(ip, "192.168.1.1");
  //return -1;
  }
  else {
    bzero(ip, 16);
    strcpy(ip, ctSysIpAddrCfg.wanIpAddr);
    bzero(information, 256);
    sprintf(information, "Wan(Ip - %s, Mask - %s), Lan(Ip - %s, Mask - %s)\n", ip, ctSysIpAddrCfg.wanSubnetMask, ctSysIpAddrCfg.lanIpAddr, ctSysIpAddrCfg.lanSubnetMask);
    logOut(information, level, __FILE__, __LINE__);
  }
  return 0;
}
  
int ctsgwGetMAC(char *mac, int level, char *file, int line) {
  int retVal = 0;
  unsigned char gatewayMAC[12];

  logOut("", level, file, line);
  if (NULL == mac) {
    logOut("mac Is NULL!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  bzero(gatewayMAC, 12);
  retVal = ctSgw_sysGetMac(gatewayMAC);
  if (0 != retVal) {
    logOut("ctSgw_sysGetMac() Failed!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  else {
    bzero(mac, 16);
    sprintf(mac, "%02X%02X%02X%02X%02X%02X", gatewayMAC[0], gatewayMAC[1], gatewayMAC[2], gatewayMAC[3], gatewayMAC[4], gatewayMAC[5]);
    bzero(information, 256);
    sprintf(information, "ctSgw_sysGetMac() Successfully, MAC - %s!\n", mac);
    logOut(information, level, __FILE__, __LINE__);
  }
  return 0;
}

int ctsgwGetSSN(char *ssn, int level, char *file, int line) {
  int retVal = 0;

  logOut("", level, file, line);
  if (NULL == ssn) {
    logOut("ssn(Get) Is NULL!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  bzero(ssn, 64);
  retVal = ctSgw_sysGetSSN(ssn, 64);
  if (0 != retVal) {
    logOut("ctSgw_sysGetSSN() Failed!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  else {
    bzero(information, 256);
    sprintf(information, "ssn(Get) - %s\n", ssn);
    logOut(information, level, __FILE__, __LINE__);
  }
  return 0;
}

int ctsgwGetWLan(char *ssid, char *password, int level, char *file, int line) {
  CtWlanSecurityCfg ctWlanSecurityCfg;
  int retVal = 0;

  logOut("", level, file, line);
  if (NULL == ssid) {
    logOut("ssid Is NULL!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  if (NULL == password) {
    logOut("ssid Is NULL!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  bzero(&ctWlanSecurityCfg, sizeof(ctWlanSecurityCfg));
  retVal = ctSgw_wlanGetSecurityCfg(&ctWlanSecurityCfg);
  if (0 != retVal) {
    logOut("ctSgw_wlanGetSecurityCfg() Failed!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  else {
    bzero(password, 32);
    bzero(ssid, 32);
    strcpy(password, ctWlanSecurityCfg.password);
    strcpy(ssid, ctWlanSecurityCfg.ssid);
    bzero(information, 256);
    sprintf(information, "WLan(SSID - %s, PassWord - %s)\n", ssid, password);
    logOut(information, level, __FILE__, __LINE__);
  }
  return 0;
}

int ctsgwSetSSN(char *ssn, int level, char *file, int line) {
  int retVal = 0;

  logOut("", level, file, line);
  if (NULL == ssn) {
    logOut("ssn(Set) Is NULL!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
//FIXME
  if (0 == strcmp("Null", ssn)) {
    bzero(ssn, 64);
  }
  retVal = ctSgw_sysSetSSN(ssn);
  if (0 != retVal) {
    logOut("ctSgw_sysSetSSN() Failed!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  return 0;
}
/*
int SNOrSSN() {
  char ssn[CT_SSN_LEN];
  bzero(ssn, CT_SSN_LEN);
  int retVal = ctsgwGetSSN(ssn, LEVEL_INFORMATION, __FILE__, __LINE__);
  if (-1 == retVal) {
    logOut("ctsgwGetSSN Failed()!\n", LEVEL_ERROR, __FILE__, __LINE__);
    return -1;
  }
  if (0 == strlen(ssn) || (0 == strcmp("Null", ssn))) {
    logOut("SSN Is NULL - To Use SN!\n", LEVEL_INFORMATION, __FILE__, __LINE__);
    return 0;
  }
  logOut("SSN Is Not NULL - To Use SSN!\n", LEVEL_INFORMATION, __FILE__, __LINE__);
  return 1;
}
*/
#ifdef DEBUG_CTSGW_X
int main() {
  int retVal = 0;

  retVal = openLogFile("ssn.log");

  retVal = ctsgwInitialize();
  if (0 != retVal) {
    goto End;
  }

  retVal = ctsgwGetMAC(mac, LEVEL_INFORMATION, __FILE__, __LINE__);
  if (0 != retVal) {
    goto End;
  }

  retVal = ctsgwGetIP(ip, LEVEL_INFORMATION, __FILE__, __LINE__);
  if (0 != retVal) {
    goto End;
  }

  retVal = ctsgwGetSSN(ssnGet, LEVEL_INFORMATION, __FILE__, __LINE__);
  if (0 != retVal) {
    goto End;
  }

  retVal = ctsgwSetSSN("89ABCDEF89ABCDEF89ABCDEF89ABCDEF\0", LEVEL_INFORMATION, __FILE__, __LINE__);
  if (0 != retVal) {
    goto End;
  }

  retVal = ctsgwGetSSN(ssnGet, LEVEL_INFORMATION, __FILE__, __LINE__);
  if (0 != retVal) {
    goto End;
  }
End:
//FIXME
  ctSgw_cleanup();
  closeLogFile();

  return retVal;
}
#endif