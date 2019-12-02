#ifndef __CT_SGW_API_H__
#define __CT_SGW_API_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#define CT_LAN_DEV_BRAND_NAME_MAX_LEN (64)
#define CT_LAN_DEV_NAME_MAX_LEN (64)
#define CT_LAN_DEV_OS_NAME_MAX_LEN (64)
#define CT_IP_ADDR_STR_MAX_LEN (16)
#define CT_IPV6_ADDR_STR_MAX_LEN (64)
#define CT_MAC_LEN (16)
#define CT_USB_DEV_MOUNT_PATH_MAX_LEN (256)
#define CT_USB_DEV_NAME_MAX_LEN (128)
#define CT_SSN_LEN (64)
#define CT_WLAN_PASSWORD_MAX_LEN (32)
#define CT_WLAN_SSID_MAX_LEN (32)


typedef enum {
  CT_EVENT_USB_DEV_ACTION,
  CT_EVENT_WAN_IP_CHANGE,
  CT_EVENT_WLAN_DEV_ONLINE
} CtEventType;

typedef enum {
  CT_LAN_DEV_CONN_UNKNOWN,
  CT_LAN_DEV_CONN_WIRED,
  CT_LAN_DEV_CONN_WLAN,
} CtLanDevConnType;

typedef enum {
  CT_LAN_DEV_UNKNOWN,
  CT_LAN_DEV_PHONE,
  CT_LAN_DEV_PAD,
  CT_LAN_DEV_PC,
  CT_LAN_DEV_STB,
  CT_LAN_DEV_OTHER
} CtLanDevType;

typedef enum {
  CT_PARITY_NONE,
  CT_PARITY_EVEN,
  CT_PARITY_ODD,
  CT_PARITY_MARK,
  CT_PARITY_SPACE,
} CtParityType;

typedef enum {
  CT_USB_DEV_ACTION_INSERT,
  CT_USB_DEV_ACTION_PULL,
} CtUsbDevActionType;

typedef enum {
  CT_USB_DEV_SERIAL = 0x1,
  CT_USB_DEV_CDC_ACM = 0x2,
  CT_USB_DEV_HID = 0x4,
  CT_USB_DEV_STORAGE = 0x8,
} CtUsbDevType;

typedef struct {
  char ipAddr[CT_IP_ADDR_STR_MAX_LEN];
} CtIpAddr;

typedef struct {
  char devName[CT_LAN_DEV_NAME_MAX_LEN];
  CtLanDevType devType;
  unsigned char macAddr[6];
  char ipAddr[CT_IP_ADDR_STR_MAX_LEN];
  CtLanDevConnType connType;
  int port;
  char brandName[CT_LAN_DEV_BRAND_NAME_MAX_LEN];
  char osName[CT_LAN_DEV_OS_NAME_MAX_LEN];
  unsigned int onlineTime;
} CtLanDevInfo;

typedef struct {
  unsigned char macAddr[6];
} CtMacAddr;


typedef struct CtSgwTupleInfo_ {
  unsigned char direct;
  unsigned char proto;
  unsigned int sipv4;
  unsigned int dipv4;
  unsigned int sipv6[4];
  unsigned int dipv6[4];
  unsigned short sport;
  unsigned short dport;
  unsigned long in_iif;
  unsigned long out_iif;
} CtSgwTupleInfo;

typedef struct {
  CtUsbDevActionType actionType;
  CtUsbDevType devType;
  int devId;
  char devName[CT_USB_DEV_NAME_MAX_LEN];
  char mountPath[CT_USB_DEV_MOUNT_PATH_MAX_LEN];
} CtUsbDevEvent;

typedef struct {
  int bandrate;
  CtParityType parity;
  int dataBits;
  int stopBits;
  int hwFlowCtrl;
  int swFlowCtrl;
} CtUsbSerialCfg;

typedef struct {
  unsigned char macAddr[6];
} CtWlanDevOnlineEvent;

typedef struct {
  char ipAddr[CT_IP_ADDR_STR_MAX_LEN];
  char subnetMask[CT_IP_ADDR_STR_MAX_LEN];
  char ipv6Addr[CT_IPV6_ADDR_STR_MAX_LEN];
  int prefixLen;
} CtWanIpChangeEvent;

typedef int(*ctSgw_appCtxCreate)(void** p_app_ctx, unsigned char flag);
typedef int(*ctSgw_appProcAppId)(unsigned char *layer2data, CtSgwTupleInfo *tupleinfo, void *p_app_ctx, unsigned int *layer7_id);
typedef void(*ctSgw_appCtxDestroy)(void** p_app_ctx);

typedef struct ctSgw_dpiFuncs_ {
  ctSgw_appCtxCreate ctSgw_appCtxCreateHook;
  ctSgw_appCtxDestroy ctSgw_appCtxDestroyHook;
  ctSgw_appProcAppId ctSgw_appProcAppIdHook;
} ctSgw_dpiFuncs;

int ctSgw_appRegisterFunc(ctSgw_dpiFuncs  *funcs);
int ctSgw_dpiCtxCreate(void** p_dpi_ctx, unsigned char flag);
int ctSgw_dpiProcAppId(unsigned char *layer2data, CtSgwTupleInfo *tupleinfo, void *dpi_ctx, unsigned int *layer7_id);
int ctSgw_errno();
int ctSgw_eventInform(const char *event);
int ctSgw_handleEvent(CtEventType *eventType, void **eventInfo);
int ctSgw_lanGetDevBandwidth(const CtMacAddr *macAddr, int *usBandwidth, int *dsBandwidth);
int ctSgw_lanGetDevInfo(CtLanDevInfo **devInfoList, int *devNum);
int ctSgw_lanGetDevInternetAccessBlacklist(CtMacAddr **macAddrList, int *macAddrNum);
int ctSgw_lanGetDevMaxBandwidth(const CtMacAddr *macAddr, int *usBandwidth, int *dsBandwidth);
int ctSgw_lanGetDevRealBytes(const CtMacAddr *macAddr, int *usBandwidth, int *dsBandwidth);
int ctSgw_lanGetDevStorageAccessBlacklist(CtMacAddr **macAddrList, int *macAddrNum);
int ctSgw_lanGetPortStatus(int *portStatus);
int ctSgw_wanGetPppoeAccount(char *pppoeAccount, int len);
int ctSgw_lanSetDevAccessPermission(const CtMacAddr *macAddr, int internetAccess, int storageAccess);
int ctSgw_lanSetDevMaxBandwidth(const CtMacAddr *macAddr, int usBandwidth, int dsBandwidth);
int ctSgw_lanSetDevStatsStatus(int enable);
int ctSgw_lanUpdateDevInfo(const CtLanDevInfo *devInfo);
int ctSgw_sysGetCpuUsage(int *percent);
int ctSgw_sysGetLoid(char *loid, int len);
int ctSgw_sysGetMemUsage(int *percent);
int ctSgw_sysPushWeb(int enable, int width, int height, int place, const char *url, int time);
int ctSgw_usbClose(int handle);
int ctSgw_usbDataAvailable(int handle);
int ctSgw_usbLock(int devId);
int ctSgw_usbOpen(int devId, int *handle);
int ctSgw_usbRead(int handle, void *buf, size_t *count, int timeout);
int ctSgw_usbRegister(CtUsbDevType devType);
int ctSgw_usbSetSerial(int handle, const CtUsbSerialCfg *serialCfg);
int ctSgw_usbUnlock(int devId);
int ctSgw_usbUnregister(void);
int ctSgw_usbWrite(int handle, const void *buf, size_t *count);
int ctSgw_wanAttachL2tpTunnel(const char *tunnelName, const CtIpAddr *ipAddrList, int ipAddrNum);
int ctSgw_wanCreateL2tpTunnel(const char *serverIpAddr, const char *userId, const char *username, const char *password, char *tunnelName, int len);
int ctSgw_wanDetachL2tpTunnel(const char *tunnelName, const CtIpAddr *ipAddrList, int ipAddrNum);
int ctSgw_wanGetIfStats(int *usStats, int *dsStats);
int ctSgw_wanGetL2tpTunnelStatus(const char *userId, const char *tunnelName, char *list, int listLen, int *status);
int ctSgw_wanRemoveL2tpTunnel(const char *tunnelName, const char *userId, int *status);
int ctSgw_wlanSetState(int ssidIndex, int enable);
void ctSgw_appUnRegisterFunc(void);
void ctSgw_dpiCtxDestroy(void** p_dpi_ctx);


typedef enum {
  CT_WL_AUTH_OPEN,
  CT_WL_AUTH_SHARED,
  CT_WL_AUTH_WPA_PSK,
  CT_WL_AUTH_WPA2_PSK,
  CT_WL_AUTH_WPA2_PSK_MIX
} CtWlanAuthMode;

typedef struct {
  char wanIpAddr[CT_IP_ADDR_STR_MAX_LEN];
  char wanSubnetMask[CT_IP_ADDR_STR_MAX_LEN];
  char ipv6WanAddr[CT_IPV6_ADDR_STR_MAX_LEN];
  int wanPrefixLen;
  char lanIpAddr[CT_IP_ADDR_STR_MAX_LEN];
  char lanSubnetMask[CT_IP_ADDR_STR_MAX_LEN];
  char ipv6LanAddr[CT_IPV6_ADDR_STR_MAX_LEN];
  int lanPrefixLen;
} CtSysIpAddrCfg;

typedef struct {
  int enable;
  CtWlanAuthMode authMode;
  char ssid[CT_WLAN_SSID_MAX_LEN];
  char password[CT_WLAN_PASSWORD_MAX_LEN];
} CtWlanSecurityCfg;

int ctSgw_init(int *socket);
int ctSgw_cleanup(void);
int ctSgw_sysGetIpAddr(CtSysIpAddrCfg *ctSysIpAddrCfg);
int ctSgw_sysGetMac(unsigned char mac[6]);
int ctSgw_sysGetSSN(char *ssn, int ssnLen);
int ctSgw_sysSetSSN(const char *ssn);
int ctSgw_wlanGetSecurityCfg(CtWlanSecurityCfg *ctWlanSecurityCfg);

#ifdef __cplusplus
}
#endif

#endif
