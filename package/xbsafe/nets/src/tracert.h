#ifndef TRACERT_H
#define TRACERT_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _static_tracert_result{
	char dstIp[32];
	double time;
}tracert_result;

//返回值：返回-1表示执行tracert失败，1表示追踪完成，（2接收数据包不完整，3表示接受超时），4表示目标不可达
//pIpReasult存储路由ip,最大30跳，即30个ip。number[in/out]返回实际的ip个数
int tracert(const char *DestHost,tracert_result* pIpReasult,int *number,int *brun);

#ifdef __cplusplus
}
#endif

#endif