//---------------------------------------------------------------------------
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "TThread.h"
//---------------------------------------------------------------------------
 
static void * threadFuncBody(void *ptr)
{
    TCThread *pThread = (TCThread *)ptr;
	if (!pThread->IsTerminated()) {
		pThread->Execute();
	}

    pThread->SetFinish();
    if(pThread->GetFreeOnTerminate())
    {
        delete pThread;
		pThread = NULL;
    }
	pthread_exit(0);
    return 0;
}
//---------------------------------------------------------------------------

TCThread::TCThread() :isRun(false), FTerminated(false), FFinished(false),FFreeOnTerminate(false)
{
	
}
//---------------------------------------------------------------------------

TCThread::~TCThread()
{
	if (!IsTerminated())
	{
		Terminated(true);
	}

	if(isRun)
	{

		void *statuscode = NULL;
		pthread_cancel(lthread);
		pthread_join(lthread,(void **)(&statuscode));
	}
}
//---------------------------------------------------------------------------
int TCThread::start()
{
  int result = -1;
  isRun = true;
//Luan
  result = pthread_create(&lthread, NULL, &threadFuncBody, (void *)this);
  if (0 != result) {
	isRun = false;
    perror("pthread_create failed");
    return -1;
  }  
  return 0;
}
