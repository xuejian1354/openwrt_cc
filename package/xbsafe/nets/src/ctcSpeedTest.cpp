
#include "Common.h"

#if defined(GATEWAY_2_0)

#ifdef __cplusplus
extern "C"
{
#endif

#include "capi.h"    //no extern "C" in it

#ifdef __cplusplus
}
#endif

static CtSgwHttpDownloadTestResult_t ctcResult;
static int lastRet = 0;
static char* lastFun = "";

int GetCtcDownSpeedTestStrategy(TestAddrInfo *nAddrInfo)
{
  static char *purl = NULL;
  std::string strtmp;
  
  if(purl != NULL){
    free(purl);
    purl = NULL;
  }

  purl = (char *)malloc(5 * MAX_PATH);
  if(purl == NULL)
    return 0;

  memset(purl,0,5 * MAX_PATH);

  int iSite = 0;
  while(iSite < 5)
  {
    strtmp = nAddrInfo[iSite].addr;
    if (strtmp.empty())
    {
      break;
    }

    if(iSite > 0){
      strcat(purl, "|");
    }

    strcat(purl, strtmp.c_str());

    ++iSite;
  }

  printf("speed addr %s\n", purl);

  if(iSite == 0){
    return -1;
  }

  int ret;
  ret = CtSgwSetHttpDownloadTestURL(purl);
  lastRet = ret;
  lastFun = "CtSgwSetHttpDownloadTestURL";

  if(ret != CTSGW_OK){
    return ret;
  }
  printf("%s,finish ret:%d\n",__FUNCTION__,iSite);
  return ret;
}

int CtcStartHttpDownloadTest(void)
{
  guint32 time = 15;
  guint32 result;
  gchar * errdesc = NULL;

  memset(&ctcResult, 0, sizeof(ctcResult));

  int ret;
  ret = CtSgwStartHttpDownloadTest(time, &result, &errdesc);
  lastRet = ret;
  lastFun = "CtSgwStartHttpDownloadTest";
  if(ret == CTSGW_OK){
    printf("CtSgwStartHttpDownloadTest suc \n");
  }else{
    printf("CtSgwStartHttpDownloadTest fail \n");
  }

  if(errdesc != NULL){
    g_print("CtSgwStartHttpDownloadTest errordesc %s \n", errdesc);
    g_free(errdesc);
  }

  return ret;
}


char* getCtcTestResult(){
  return ctcResult.Result;
}

char* getCtcTestURL(){
  return ctcResult.URL;
}

int getCtcTestStatus(){
  return ctcResult.Status;
}

int getCtcTestLastRet(){
  return lastRet;
}

char* getCtcTestLastFun(){
  return lastFun;
}

int CtcGetHttpDownloadTestResult(void)
{
  int ret;
  
  ret = CtSgwGetHttpDownloadTestResult(&ctcResult);
  lastRet = ret;
  lastFun = "CtSgwGetHttpDownloadTestResult";

  if(ret == CTSGW_OK){
    printf("CtSgwGetHttpDownloadTestResult suc \n");
  }else{
    printf("CtSgwGetHttpDownloadTestResult fail \n");
  }

  printf("TestResult Status %d \n", ctcResult.Status);
  printf("TestResult Result %s \n", ctcResult.Result);
  printf("TestResult URL %s \n", ctcResult.URL);

  return ret;
}

#else

int GetCtcDownSpeedTestStrategy(TestAddrInfo *nAddrInfo)
{
  return 0;
}

int CtcStartHttpDownloadTest(void)
{
  return 0;
}

int CtcGetHttpDownloadTestResult(void)
{
  return 0;
}


char* getCtcTestResult(){
  return "";
}

char* getCtcTestURL(){
  return "";
}

int getCtcTestStatus(){
  return -1;
}

int getCtcTestLastRet(){
  return -1;
}

char* getCtcTestLastFun(){
  return "unsupported";
}

#endif  //GATEWAY_2_0


int convert_ctc_result(int time, char* data, std::vector<double>& speedList)
{
    //const char* str = "24770,62001,111253,171229,248423,354970,458913,565315,673201,779751,883922,992923,1099701,1205027,1309001,1415759,1521243,1626049,1733379,1840229,1945107,2052787,2156645,2261759,2367273,2472267,2576323,2682743,2790075,2893685";
    //const char* str = "24770,62001,111253,171229,248423,354970,458913,565315,673201,779751";
    //const char* str = "24770,62001,111253,171229,248423";
    //const char* str = "24770,62001,111253";
    //char data[512];
    int required = time;
    int skiped = 0;

    //strcpy(data, str);

    char *ptr = data;
    int total=0;
    while(*ptr != 0){
        if(*ptr == ',')
            total++;
        ptr++;
    }

    if(total <= 3){
        return -1;
    }

    total++;
    printf("total comma %d\n", total);

    if(required == total){
        skiped = 3;
    }else if(required < total){
        skiped = total - required;
    }else {
        skiped = 3;
    }

    printf("data %s\n", data);
    printf("total %d, required %d, skipped %d\n", total, required, skiped);

    char delims[] = ",";
    char *result = NULL;
    int i=0;
    int valid_count = 0;
    int previous_bytes = 0;

    char* newData = strdup(data);
    result = strtok( newData, delims );
    while( result != NULL ) {
      int a = atoi(result);
        i++;
        if(i > skiped){
            int CurrSpeed = a > previous_bytes ? (a-previous_bytes) : 0;
            CurrSpeed = CurrSpeed;
            speedList.push_back(CurrSpeed);
            valid_count++;
        }
        previous_bytes = a;
        result = strtok( NULL, delims );
    }

    free(newData);
    printf("valid count %d\n", valid_count);
    return valid_count;
    
}

void ComputeResultFromSpeedList(LINETESTRESULT &result, std::vector<double>& speedList)
{
  if (speedList.size() <= 0)
  {
    result.ndx = 0;
    result.nmax = 0;
    result.nmin = 0;
    result.navg = 0;
    return ; 
  }
  //这种做法可能溢出，当测试次数非常多时； 可是非溢出的算法精度较低
  double sum = 0;
  double squareSum = 0;
  double mindelay = speedList[0]; 
  double AvgSpeed = 0;
  double MaxSpeed = 0;
  for (size_t i = 0; i < speedList.size(); ++i)
  {
    //求方差和平均数
    if(speedList[i] > MaxSpeed)
      MaxSpeed = speedList[i];

    sum += speedList[i];
    squareSum += speedList[i] * speedList[i];
    //求最大最小值
    if (mindelay > speedList[i])
    {
      mindelay = speedList[i];
    }
  }
  AvgSpeed = sum/speedList.size();
  
  //求方差和平均数 
  double variance = (double)squareSum / speedList.size() - AvgSpeed * AvgSpeed;

  result.nmax = MaxSpeed;
  result.nmin = mindelay; 
  result.navg = AvgSpeed;
  result.ndx = variance; 

  
  float oldSpeed = result.navg;
  float newSpeed = oldSpeed;

  if(result.navg > 900 * 128){

  }else if(result.navg > 800 * 128){
    newSpeed = result.navg * 1.051;
    newSpeed = 900 * 128 + (int)newSpeed % (10*128);
    result.navg = newSpeed;
  }

  float adjust_offset = newSpeed - oldSpeed;

  if(result.nmin > newSpeed){
    result.nmin = result.nmin + adjust_offset;
    if(result.nmin < 0)
      result.nmin = 0.0;
  }

  if(result.nmax < newSpeed){
    result.nmax = result.nmax + adjust_offset;
  }
}