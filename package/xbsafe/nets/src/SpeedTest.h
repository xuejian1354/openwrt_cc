
#ifndef SpeedTestH
#define SpeedTestH
#include <vector>
#include <string>
#include "Common.h"
//---------------------------------------------------------------------------
//LuanSh
//#pragma pack(push,1)
typedef struct _Site
{
	int     type;                   //当为下载服务器信息时：1: http, 2: ftp；当为上传服务器信息时：1：http POST，2：ftp，3: TCP; 
	char url[MAX_PATH];               //测速URL
	int		port;					//该字段只用于上传测速，标示服务器的端口，只用type为3时 tcp模式时有效。
}TSITE;

typedef struct _CommInfo
{
    TSITE   Site[5];				//下载测速站点信息,第一个为省中心站点
	TSITE   upServer[5];			//上传测速站点信息，第一个为省中心站点

    _CommInfo()
    {
        memset(this, 0, sizeof(_CommInfo));
    }
}TCOMMINFO, *PCOMMINFO;

typedef struct _WebInfo
{
	int     AllComplated;           //0: 未完成，1：全部完成
    int     Index;                  //序号(0-ItemCount-1)，用以标识回调的是哪一个Url的结果
    int     Status;		            //结果状态, 1: 正常完成, 2: 访问失败  
	char 	IP[32];	                //域名解析后获取到得ip地址
	double     ResponceTime;           //服务器响应时间(ms)
	int     WebPageSize;            //页面大小(KB)
	double     LoadTime;               //页面加载完成时间(ms)
	double     DNSTime;			    //dns解析时间(ms)
	double     FstPkgTime;	    //http首包时延(ms)
	
    _WebInfo() { Init(); }
    void Init() { memset(this, 0, sizeof(_WebInfo)); }
}TWEBINFO, *PTWEBINFO;
//---------------------------------------------------------------------------

//#pragma pack(pop)

//查询用户信息回调，result: 结果，0：正确，其他为错误 
//在下载处也使用了该回调，通知程序最终使用的下载地址的id + 1000，
//typedef void (__stdcall *FQueryCallback)(int result, TCOMMINFO*);
//typedef void (*FQueryCallback)(int result, TCOMMINFO*);

//下载测速回调函数，Status: 1: 开始下载, 2: 下载过程中, 3: 下载完成, 4:下载失败； 当前速度(kb/秒)、最大速度、平均速度、已下载大小、完成百分比(0-100)
//typedef void (*FDownloadCallback)(int Status, double CurrSpeed, double MaxSpeed, double AvgSpeed, DWORD dwDownloadSize, int iPostion);
//网页测速回调函数
//typedef void (*FWebCallback)(TWEBINFO *pWebInfo);   

//上传的回调函数，Status: 0开始初始化,1: 开始载, 2: 下载过程中, 3: 下载完成, 4下载失败,iPostion已完成的百分比
//typedef void (*FUploadCallback)(int Status, double CurrSpeed, double MaxSpeed, double AvgSpeed, DWORD dwUpSize, int iPostion);
//---------------------------------------------------------------------------
class TSpeedTest
{
public:
	int NTimes;								//用于指定下载需要的时长
    int Status;                             //用于下载测速
	
//    FQueryCallback      QCallback;          //查询用户宽带信息回调
//    FDownloadCallback   DloadCallback;      //文件下载测速回调
//	FUploadCallback		ULoadCb;//上传的测速回调
//    FWebCallback        WebCallback;        //网页响应测速回调
    TCOMMINFO UInfo;
   
    int     UrlCount;
    std::vector<std::wstring> PageUrl;                    //网页测速URL
    unsigned int UDPTime;
	unsigned int UDPPPS;
	unsigned int UDPBPS;
	unsigned int TotalTime;
	unsigned int UserBandWidth;
	unsigned int WANBPS;
	char if_policy[128];

private:
	int *m_brun;
	double CurrSpeed;
	double MaxSpeed;
	double AvgSpeed;
	unsigned long TotalSize;
	
	unsigned int TotalSendPackets;
	unsigned int TotalReceivedPackets;

	

	EnumFuncTest m_EnumTest;
	std::string m_startTm;
	std::vector<double> m_speedList;
	std::vector<TWEBINFO> m_webspeedList;
	std::vector<LINETESTRESULT> m_vecResult;
	void ComputeToResult(LINETESTRESULT &result);
public:
    TSpeedTest();
    ~TSpeedTest();

    int GetUpSpeedTestStrategy(TestAddrInfo *nAddrInfo);
	int GetDownSpeedTestStrategy(TestAddrInfo *nAddrInfo);
  
	BOOL StartDownload(int ntms,int *brun);
    void StopDownload();    

	BOOL StartRouterUDPDownload(int ntms,int *brun, const char * server, int port, int bandWidth, int PacketSize, int realPacketSize);
	BOOL StartUDPDownload(int ntms,int *brun, const char * server, int port, int user_bandWidth, int bandWidth, int PacketSize, int realPacketSize);
	BOOL AutoStartUDPDownload(int ntms,int *brun, const char * server, int port, int bandWidth, int PacketSize, int realPacketSize);

    BOOL StartWebTest(TestAddrInfo *pAddrInfo,int *brun);
    void StopWebTest();

	BOOL StartUpLoad(int ntms,int *brun);
	void StopUpLoad();
	
	double GetMaxSpeed(){return MaxSpeed;}
	double GetAvgSpeed(){return AvgSpeed;}
	std::vector<double>& GetSpeedList(){ return m_speedList;}
	
	const std::string& GetStartTime(){ return m_startTm; }
	std::vector<LINETESTRESULT>& GetResult();
	unsigned long GetTotalSize();
	unsigned int GetTotalTime();

};
//---------------------------------------------------------------------------
extern TSpeedTest SpeedCls;
//---------------------------------------------------------------------------
#endif
