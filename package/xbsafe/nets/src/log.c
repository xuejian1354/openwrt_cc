#include "log.h"

#ifdef FILE_LOG
static FILE *logFp = NULL;
static int lineNumber = 0;
#endif

int closeLogFile(void) {
 #ifdef FILE_LOG
  int retVal = 0;

  if (NULL == logFp) {
    fprintf(stderr, "%s(%d) - logFp Is NULL!\n", __FILE__, __LINE__);
    return -1;
  }
  retVal= fclose(logFp);
  if (0 != retVal) {
    fprintf(stderr, "%s(%d) - fclose() Failed!\n", __FILE__, __LINE__);
    return -1;
  }
 #endif
  return 0;
}

int logOut(char *content, int level, char *file, int line) {
  int retVal = 0;

 #ifdef FILE_LOG
  if (NULL == logFp) {
    fprintf(stderr, "%s(%d) - logFp Is NULL!\n", file, line);
    return -1;
  }
  if (NULL == content) {
    fprintf(stderr, "%s(%d) - content Is NULL!\n", file, line);
    return -1;
  }
  if (LOG_LEVEL < level) {
    lineNumber = lineNumber + 1;
    retVal = fprintf(logFp, "%s(%d) - %s", file, line, content);
  //FIXME
    fflush(logFp);
  }
  if (lineNumber > MAX_LINE) {
    lineNumber = 0;
    rewind(logFp);
  }
 #else
  if (LOG_LEVEL < level) {
    retVal = printf("%s(%d) - %s", file, line, content);
  }
 #endif
  retVal = (0 < retVal) ? -1 : 0;
  return retVal;
}

int openLogFile(char *fileName) {
 #ifdef FILE_LOG
  logFp = fopen(fileName, "w");
  if (NULL == logFp) {
    fprintf(stderr, "%s(%d) - fopen() Failed!\n", __FILE__, __LINE__);
    return -1;
  }
 #endif
  return 0;
}
