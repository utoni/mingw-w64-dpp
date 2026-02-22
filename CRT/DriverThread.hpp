#ifndef DDK_THREAD
#define DDK_THREAD 1

#include <ntddk.h>

#include <EASTL/deque.h>
#include <EASTL/functional.h>
#include <EASTL/shared_ptr.h>

extern "C" void InterceptorThreadRoutine(PVOID threadContext);
extern "C" void DpcTimerInterceptor(PKDPC Dpc, PVOID Context, PVOID Arg1, PVOID Arg2);

namespace DriverThread
{
class Mutex
{
public:
    Mutex(void);
    ~Mutex(void);

private:
    void Lock();
    void Unlock();

    volatile long int m_interlock;

    friend class LockGuard;
};

class LockGuard
{
public:
    LockGuard(Mutex & m);
    ~LockGuard(void);

private:
    Mutex m_Lock;
};

class ThreadArgs : public virtual eastl::enable_shared_from_this<ThreadArgs>
{
public:
    ThreadArgs(void)
    {
    }
    ThreadArgs(const ThreadArgs &) = delete;
    virtual ~ThreadArgs(void)
    {
    }
};

using ThreadRoutine = eastl::function<NTSTATUS(eastl::shared_ptr<ThreadArgs> args)>;

class Thread
{
public:
    Thread(void);
    Thread(const Thread &) = delete;
    ~Thread(void);
    NTSTATUS Start(ThreadRoutine routine, eastl::shared_ptr<ThreadArgs> args);
    NTSTATUS WaitForTermination(LONGLONG timeout = 0);
    NTSTATUS WaitForTerminationIndefinitely();
    HANDLE GetThreadId(void)
    {
        return m_threadId;
    }
    bool isRunning(void)
    {
        return GetThreadId() != nullptr;
    }

private:
    friend void ::InterceptorThreadRoutine(PVOID threadContext);

    HANDLE m_threadId = nullptr;
    PETHREAD m_threadObject = nullptr;
    Mutex m_mutex;
    ThreadRoutine m_routine;
    eastl::shared_ptr<ThreadArgs> m_threadContext;
};

class Spinlock
{
public:
    Spinlock(void);
    NTSTATUS Acquire(void);
    void Release(void);
    KIRQL GetOldIrql(void);

private:
    KIRQL m_oldIrql;
    KSPIN_LOCK m_spinLock;
};

class Semaphore
{
public:
    Semaphore(LONG initialValue = 0, LONG maxValue = MAXLONG);
    NTSTATUS Wait(LONGLONG timeout = -1);
    LONG Release(LONG adjustment = 1);

private:
    KSEMAPHORE m_semaphore;
};

class Event
{
public:
    Event();
    NTSTATUS Wait(LONGLONG timeout = -1);
    NTSTATUS Notify();

private:
    KEVENT m_event;
};

class WorkItem final
{
    friend class WorkQueue;

public:
    WorkItem(const eastl::shared_ptr<void> & user) : m_user(std::move(user))
    {
    }
    virtual ~WorkItem(void)
    {
    }
    template <class T>
    eastl::shared_ptr<T> Get(void)
    {
        return eastl::static_pointer_cast<T>(m_user);
    }
    template <class T>
    void Get(eastl::shared_ptr<T> & dest)
    {
        dest = eastl::static_pointer_cast<T>(m_user);
    }

private:
    eastl::shared_ptr<void> m_user;
};

using WorkerRoutine = eastl::function<NTSTATUS(WorkItem & item)>;

class WorkQueue final
{
public:
    WorkQueue(void);
    WorkQueue(const WorkQueue &) = delete;
    ~WorkQueue(void);
    NTSTATUS Start(WorkerRoutine routine);
    void Stop(bool wait = true);
    void Enqueue(WorkItem & item);
    void Enqueue(eastl::deque<WorkItem> & items);

private:
    Mutex m_mutex;
    eastl::deque<WorkItem> m_queue;
    Event m_wakeEvent;
    bool m_stopWorker; // Work LIST must be empty and StopWorker TRUE to be able to stop!
    Thread m_worker;
    WorkerRoutine m_workerRoutine;

    static NTSTATUS WorkerInterceptorRoutine(eastl::shared_ptr<ThreadArgs> args);
};

using DpcRoutine = eastl::function<void(void)>;

class DpcTimer
{
public:
    DpcTimer();
    ~DpcTimer(void);
    bool Start(const DpcRoutine & routine, LONGLONG timeout = -100000 /* 10 ms */,
               bool periodic = false);
    bool StopAndWait();
private:
    friend void ::DpcTimerInterceptor(PKDPC Dpc, PVOID Context, PVOID Arg1, PVOID Arg2);

    KTIMER m_Timer;
    KDPC m_TimerDpc;
    DpcRoutine m_Callback;
};

}; // namespace DriverThread

#endif
