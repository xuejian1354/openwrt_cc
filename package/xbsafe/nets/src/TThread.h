//---------------------------------------------------------------------------

#ifndef TThreadH
#define TThreadH
//---------------------------------------------------------------------------

class TCThread
{
private:
	bool isRun;
    bool FTerminated;
	bool FFinished;
    bool FFreeOnTerminate;
	pthread_t lthread;
   
public:
	virtual void Execute()=0;
public:
    TCThread();
    virtual ~TCThread();
	
    bool IsTerminated() { return FTerminated; }
    void Terminated(bool bValue) { FTerminated = bValue; }
	void SetFinish() { FFinished = true;}
    bool IsFinished() { return FFinished; }

    bool GetFreeOnTerminate() { return FFreeOnTerminate; }
    void SetFreeOnTerminate(bool bValue) { FFreeOnTerminate = bValue; }  
	int start();
};
//---------------------------------------------------------------------------

#endif
