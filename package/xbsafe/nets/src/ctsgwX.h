#ifndef CTSGWX_H
#define CTSGWX_H 1

#include <string.h>
#include "ct_sgw_api.h"

//#define DEBUG_CTSGW_X 1
//#define PC 1

#ifdef __cplusplus
extern "C"
{
#endif

char ip[CT_IP_ADDR_STR_MAX_LEN];
char mac[CT_MAC_LEN];
char ssnGet[CT_SSN_LEN];
char ssnSet[CT_SSN_LEN];
char wlanSSID[CT_WLAN_SSID_MAX_LEN];
char wlanPassWord[CT_WLAN_PASSWORD_MAX_LEN];

extern int ctsgwInitialize();
extern int ctsgwGetIP(char *ip, int level, char *file, int line);
extern int ctsgwGetMAC(char *mac, int level, char *file, int line);
extern int ctsgwGetSSN(char *ssn, int level, char *file, int line);
extern int ctsgwSetSSN(char *ssn, int level, char *file, int line);
extern int ctsgwGetWLan(char *ssid, char *password, int level, char *file, int line);
//extern int SNOrSSN();

#ifdef __cplusplus
}
#endif

#endif