#ifndef LOG_H
#define LOG_H 1

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define FILE_LOG 1
#define LEVEL_HIDE 0
#define LEVEL_VERBOSE 1
#define LEVEL_INFORMATION 2
#define LEVEL_ERROR 3
#define LOG_LEVEL LEVEL_HIDE
#define MAX_LINE 256

extern int closeLogFile(void);
extern int logOut(char *content, int level, char *file, int line);
extern int openLogFile(char *fileName);


#ifdef __cplusplus
}
#endif

#endif