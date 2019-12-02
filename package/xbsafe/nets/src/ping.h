#ifndef PING_H
#define PING_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _static_ping_result{
	int nSend;		//发送的包总数
	int nRecv;		//接受到的包总数
	double totaltime;	//所有数据包总的时间
	double mintime;	//最小包的延时
	double maxtime;	//最大的包延时
	double avgtime;	//平均延时
	double mdev;	//算数平均数差值
	double mvari;	//方差
}static_ping_result;

int ping(const char *phost,int times,int interval,static_ping_result *presult,int *nrun);

#ifdef __cplusplus
}
#endif

#endif