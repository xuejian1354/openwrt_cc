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
#ifndef _CAPI_UTIL_H_
#define _CAPI_UTIL_H_

#include <stdio.h>
#include <gio/gio.h>

#define IS_EMPTY_STRING(s)  ((s == NULL) || (*s == '\0'))

#define IS_HEX(c)   ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))

#define IS_DIGIT(c) ((c >= '0' && c <='9'))

gboolean is_valid_ctc_mac(char *macaddr);
gboolean is_valid_digit_str(char *digstr);
gboolean is_valid_lan_devtype(char *devtype);
gboolean is_valid_ipaddr(char *ipaddr);
gboolean is_valid_loid_prov(char *province);
gboolean is_valid_tl_faultcategory(const char *faultcategory);
gboolean is_valid_ip6_address(char* address);



/*
 * Valid time format '08:30'
 *
 */
gboolean is_valid_time(char *tmstr);

/*
 * Valid date time format 'YYYY-MM-DD HH:MM:SS'
 *
 */
gboolean is_valid_datetime(char *dtstr);



#endif
