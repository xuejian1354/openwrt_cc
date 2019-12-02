#ifndef PERIOD_TASK_H_INCLUDED
#define PERIOD_TASK_H_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	PERIOD_INTERVAL,
	PERIOD_CALENDAR,
}PERIOD_POLICY_TYPE;

typedef struct {
  	PERIOD_POLICY_TYPE type;
  	char start_time[64];
  	char interval_time[64];
  	int interval_time_in_second;
  	int start_time_in_second;
}PeriodPolicy;

int start_simple_period_task(char* name, int interval, int start_time, void (*handler)(void* ptr), void (*close_handler)(void* ptr), void* user_ptr);
int start_period_task(char* name, PeriodPolicy* policy, void (*handler)(void* ptr), void (*close_handler)(void* ptr), void* user_ptr);
int stop_period_task(char* name);


#ifdef __cplusplus
}
#endif
#endif