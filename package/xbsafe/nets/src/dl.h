#ifndef DL_H
#define DL_H 1

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include "log.h"

//#define PC 1

#ifdef __cplusplus
extern "C"
{
#endif
	
typedef int (*ctSgw_sysGetSSN_T)(char *ssn, int ssnLen);
typedef int (*ctSgw_sysSetSSN_T)(const char *ssn);

extern int deviceVersion(void);


#ifdef __cplusplus
}
#endif

#endif