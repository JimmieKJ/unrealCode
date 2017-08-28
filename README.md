# unrealCode

D:\Work\UE_sourceCode\ue414Change\UnrealEngineGitHub\Engine\Source\Runtime\Core\Public\Async\AsyncWork.h中的
template<typename TTask>
class FAutoDeleteAsyncTask
	: public IQueuedWork
{
	/** User job embedded in this task */
public: TTask Task;

	/* Generic start function, not called directly
		* @param bForceSynchronous if true, this job will be started synchronously, now, on this thread
	**/
	void Start(bool bForceSynchronous)
	{
		FPlatformMisc::MemoryBarrier();
		FQueuedThreadPool* QueuedPool = GThreadPool;
		if (bForceSynchronous)
		{
			QueuedPool = 0;
		}
		if (QueuedPool)
		{
			QueuedPool->AddQueuedWork(this);
		}
		else
		{
			// we aren't doing async stuff
			DoWork();
		}
	}

	/**
	* Tells the user job to do the work, sometimes called synchronously, sometimes from the thread pool. Calls the event tracker.
	**/
	void DoWork()
	{
		FScopeCycleCounter Scope(Task.GetStatId(), true);

		Task.DoWork();
		delete this;
	}

	/**
	* Always called from the thread pool. Just passes off to DoWork
	**/
	virtual void DoThreadedWork()
	{
		DoWork();
	}

	/**
	 * Always called from the thread pool. Called if the task is removed from queue before it has started which might happen at exit.
	 * If the user job can abandon, we do that, otherwise we force the work to be done now (doing nothing would not be safe).
	 */
	virtual void Abandon(void)
	{
		if (Task.CanAbandon())
		{
			Task.Abandon();
			delete this;
		}
		else
		{
			DoWork();
		}
	}

public:
	/** Forwarding constructor. */
	template<typename...T>
	explicit FAutoDeleteAsyncTask(T&&... Args) : Task(Forward<T>(Args)...)
	{
	}

	/** 
	* Run this task on this thread, now. Will end up destroying myself, so it is not safe to use this object after this call.
	**/
	void StartSynchronousTask()
	{
		Start(true);
	}

	/** 
	* Run this task on the lo priority thread pool. It is not safe to use this object after this call.
	**/
	void StartBackgroundTask()
	{
		Start(false);
	}

};
改为public
