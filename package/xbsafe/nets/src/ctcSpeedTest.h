#ifndef CtcSpeedTestH
#define CtcSpeedTestH

#include "Common.h"
int GetCtcDownSpeedTestStrategy(TestAddrInfo *nAddrInfo);
int CtcStartHttpDownloadTest(void);
int CtcGetHttpDownloadTestResult(void);
char* getCtcTestResult();
int getCtcTestStatus();
char* getCtcTestURL();
int getCtcTestLastRet();
char* getCtcTestLastFun();
int convert_ctc_result(int time, char* data, std::vector<double>& speedList);
void ComputeResultFromSpeedList(LINETESTRESULT &result, std::vector<double>& speedList);

//our UDP Upload test

int StartUdpUpTest(const char* host, int port, int ntimes, int user_bandwith, int testMethod, int* totalTime, int* bps, int* pps);
char* getUdpUpTestResult();

#endif
