#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/statfs.h>
#include <ctype.h>
#include<sys/time.h>
#include<signal.h>
#include<sys/time.h>//itimerval结构体的定义
#include <time.h>
#include "period_task.h"

static int handle_count = 0;

static int running = 0;

typedef struct {
  const char* name;
  PERIOD_POLICY_TYPE policy_type;
  timer_t timerid;
  int signo_index;    //increase from SIGRTMIN
  PeriodPolicy policy;
  void (*handler)(void* ptr);  
  void (*close_handler)(void* ptr); 
  void* user_ptr;
  int status;   //0, uninitial, 1, start, 2. stop.
}MyTimerTask;

int start_signal_timer(MyTimerTask* task);
int stop_signal_timer(MyTimerTask* task);

int test_time_str() ;
time_t get_task_time();

static MyTimerTask task_tables[] = {
    {"Heartbeat", PERIOD_INTERVAL, 0, 0, {0}, NULL, NULL, NULL, 0},
    {"PeriodTask", PERIOD_INTERVAL, 0, 1, {0}, NULL, NULL, NULL, 0},
    {"CalendarTask", PERIOD_CALENDAR, 0, 2, {0}, NULL, NULL, NULL, 0},
};

MyTimerTask* find_task_by_name(char* name){
	int i;
	for(i=0; i< sizeof(task_tables)/sizeof(task_tables[0]); i++){
	  if(strcmp(task_tables[i].name, name) == 0){
	    return &task_tables[i];
	   }
	}
	return NULL;
}

int start_period_task(char* name, PeriodPolicy* policy, void (*handler)(void* ptr), void (*close_handler)(void* ptr), void* user_ptr){
	int ret;

	test_time_str();

	MyTimerTask* task = find_task_by_name(name);
	if(task == NULL)
		return -1;

	if(task->status == 1)
		return -2;

	task->handler = handler;
	task->close_handler = close_handler;
	task->user_ptr = user_ptr;
	memcpy(&task->policy, policy, sizeof(task->policy));

	ret = start_signal_timer(task);
	if(ret != 0)
		return ret;

	task->status = 1;

	return 0;
}

int start_simple_period_task(char* name, int interval, int start_time, void (*handler)(void* ptr), void (*close_handler)(void* ptr), void* user_ptr)
{
	PeriodPolicy policy;
	policy.type = PERIOD_INTERVAL;
	policy.interval_time_in_second = interval;
	policy.start_time_in_second = start_time;

	return start_period_task(name, &policy, handler, close_handler,user_ptr);
}

int stop_period_task(char* name){
	MyTimerTask* task = find_task_by_name(name);
	if(task == NULL)
		return -1;

	if(task->status == 0)
		return -3;

   	if(task->close_handler != NULL)
   		task->close_handler(task->user_ptr);

	stop_signal_timer(task);

	task->status = 0;
}


///////////////////////////////////////////////////////////////////////////////////

//#define CLOCKID CLOCK_MONOTONIC
//#define SIG SIGRTMIN

#define errExit(msg)    do { perror(msg); \
                        } while (0)

static void
print_siginfo(siginfo_t *si)
{
    timer_t *tidp;
    int or;

   tidp = si->si_value.sival_ptr;

   printf("    sival_ptr = %p; ", si->si_value.sival_ptr);
    printf("    *sival_ptr = 0x%lx\n", (long) *tidp);

   or = timer_getoverrun(*tidp);
    if (or == -1)
        printf("timer_getoverrun");
    else
        printf("    overrun count = %d\n", or);
}

static void
handler(int sig, siginfo_t *si, void *uc)
{
   printf("Caught signal %d\n", sig);
   MyTimerTask* task = (MyTimerTask*)si->si_value.sival_ptr;
   if(task->handler != NULL)
   	task->handler(task->user_ptr);
}


int start_signal_timer(MyTimerTask* task)
{
    //timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    long long freq_nanosecs;
    sigset_t mask;
    struct sigaction sa;
    int signo = SIGRTMIN + task->signo_index;

   /* Establish handler for timer signal */

   printf("Establishing handler for signal %d\n", signo );
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(signo, &sa, NULL) == -1){
        errExit("sigaction");
        return -1;
    }

   /* Create the timer */

   int clockid = task->policy_type == PERIOD_INTERVAL ? CLOCK_MONOTONIC : CLOCK_REALTIME;
   sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = signo;
    sev.sigev_value.sival_ptr = (void*) task;
    if (timer_create(clockid, &sev, &task->timerid) == -1){
        errExit("timer_create");
        return -1;
    }


   /* Start the timer */

   	if(task->policy_type == PERIOD_INTERVAL)
   		its.it_value.tv_sec = task->policy.start_time_in_second;
   	else
   		its.it_value.tv_sec = task->policy.start_time_in_second;

    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = task->policy.interval_time_in_second;
    its.it_interval.tv_nsec = 0;

    //printf("timer it_value %ld, it_interval %ld\n", (long) its.it_value.tv_sec, (long) its.it_interval.tv_sec);

    int flags = clockid == CLOCK_REALTIME ? TIMER_ABSTIME : 0;
   if (timer_settime(task->timerid, flags, &its, NULL) == -1){
   		errExit("timer_settime");
   		return -1;
   }
         
   return 0;
}

int stop_signal_timer(MyTimerTask* task)
{
	timer_delete(task->timerid);
}


time_t get_task_time()
{
	time_t taskTime;
    struct tm tm_time;  
    strptime("2018-4-22 10:58:50", "%Y-%m-%d %H:%M:%S", &tm_time);  
    taskTime = mktime(&tm_time);
    printf("%ld\n", taskTime);  
    return taskTime;	
}

int test_time_str()
{  
    struct tm tm_time; 
    time_t taskTime;
    double diff_t; 
    strptime("2018-4-22 12:43:50", "%Y-%m-%d %H:%M:%S", &tm_time);  
    taskTime = mktime(&tm_time);
    printf("%ld\n", taskTime); 
    printf("-------------------------------------\n");  


    char szBuf[256] = {0};  
    time_t timer = time(NULL);  
    strftime(szBuf, sizeof(szBuf), "%Y-%m-%d %H:%M:%S", localtime(&timer));  
    printf("%s\n", szBuf);  

    diff_t = difftime(taskTime, timer);
    printf("will wait %f seconds\n", diff_t);

    return 0;  
} 
