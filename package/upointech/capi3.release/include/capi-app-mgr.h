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


#ifndef _CAPI_AM_H_
#define _CAPI_AM_H_


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include <gio/gio.h>
#include <syslog.h>

#include "capi-util.h"
#include "capi-dbus-service.h"
#include "capi-sys-service.h"

//#define CT_TEST_STR "CT2018"

extern CtSgwDbusServiceContext_t *sgw_dbus_service_context;

#define OPT_INFO_PATH "/opt/info"
#define DBUS_BUS_ADDRESS "/var/run/dbus/server_address"

#define DBUS_BUS_NAME "DBUS_SYSTEM_BUS_ADDRESS"
#define DBUS_CONF "/etc/dbus-1/system.conf"
#define DBUS_CONF_DIR "/etc/dbus-1/system.d"
#define DBUS_TYPE G_BUS_TYPE_SYSTEM

#define MAX_UCI_OPT_NAME_LEN 15
#define MAX_UCI_OPT_VAL_LEN 255

#define APP_CALLBACK_TIMEOUT 10000

typedef char CtSgwUCIOptName_t[MAX_UCI_OPT_NAME_LEN + 1];
typedef char CtSgwUCIOptVal_t[MAX_UCI_OPT_VAL_LEN + 1];
typedef struct {
	CtSgwUCIOptName_t name;
	CtSgwUCIOptVal_t value;
} CtSgwUCIOpt_t;

typedef struct {
	char name[STR_LEN_128];
	char appid[STR_LEN_128];
	char version[STR_LEN_16];
	unsigned int state;
} CtSgwAppInfo_t;

typedef struct {
	char path[MAX_OBJPATH_LEN + 1];
} CtSgwNodeInfo_t;


int CtSgwSetDbusEnv(void);

int CtSgwUCIReadOption(const char *pkg_name, const char *section_name, const char *option_name, CtSgwUCIOptVal_t option_val);
int CtSgwUCIReadSection(const char *pkg_name, const char *section_type, const char *section_name, CtSgwUCIOpt_t **opts, uint32_t *optsize);
int CtSgwUCIWriteOption(const char *pkg_name, const char *section_name, const char *option_name, const char *option_val);

void CtSgwLogOpen(int facility, const char *ident);
void CtSgwLogClose(void);
void CtSgwLog(int priority, const char *fmt, ...);
int CtSgwIsLogInitialized(void);

void CtSgwSetErrMsg(const char *msg);
gchar *CtSgwGetErrMsg(void);

typedef struct {
	char name[STR_LEN_32];
	char state[STR_LEN_16];
} CtSgwAppStatus_t;

int CtSgwStartApp(const char *app_name);

int CtSgwUninstallApp(const char *app_name);

int CtSgwInstallApp(const char *app_file, gpointer user_data, GAsyncReadyCallback cb);
int CtSgwUpgradeApp(const char *app_file, gpointer user_data, GAsyncReadyCallback cb);

int CtSgwStopApp(const char *app_name);
int CtSgwReloadApp(const char *app_name);

int CtSgwGetAppStatus(CtSgwAppStatus_t *app_status);
int CtSgwListApp(CtSgwAppInfo_t **apps, uint32_t *size);

int CtSgwHandleKernelModules(const char *pkg_name, gboolean insert, const char *mod_name, const char *mod_opts);

char *CtSgwPostMsg(const char *app_name, const char *msg);
char *CtSgwGetAppStatistics(const char *app_name);
int CtSgwAppSendAlarm(const char *value);
int CtSgwGetNumOfNodes(const char *service_name, const char *object_path, CtSgwNodeInfo_t **nodes, uint32_t *num);
char *CtSgwGetAppNameByObjpath(const char *obj_path);

int CtSgwFactoryPlugin(const char *app_name);

CtSgwDBusNodeInfo_t *CtSgwAppRegisterMgtFuncs(CtSgwAppMgtCallbacks_t *cbs);

int CtSgwAppMountUsbDevice(char *mountCmd, char *mountSrc, char *mountDest, char *opts);

int CtSgwAppUnmountUsbDevice(char *umountCmd, char *umountPath, char *opts);

#endif
