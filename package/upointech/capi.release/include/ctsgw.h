/************************************************************************************************/
/*																								*/
/* Copyright (c) 2015-2016	Shanghai Upointech Information Techology Co. All Rights Reserved.	*/
/*																								*/
/*				上海有点信息科技有限公司	   版权所�?2015-2016								*/
/*											   													*/
/* This program is the proprietary software of Upointech Co., and may only be used,				*/
/* duplicated, modified or distributed pursuant to the terms and conditions of a separate,		*/
/* written license agreement executed between you and Upointech(an "Authorized License").		*/
/* The recipient of this software implicitly accepts the terms of the license.					*/
/*																								*/
/* 本程序的版权属于上海有点信息科技有限公司，任何人士阅读、使用、复制、修改或者发行都�?		*/
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

typedef struct {
    char name[MAX_LENGTH];
    char value[MAX_LENGTH];
} Parameter;


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


#ifdef __cplusplus
}
#endif

#endif
