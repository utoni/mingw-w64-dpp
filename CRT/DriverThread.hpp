#ifndef DDK_THREAD
#define DDK_THREAD 1

#include <ntddk.h>
#include <stdint.h>

#include <EASTL/deque.h>
#include <EASTL/functional.h>
#include <EASTL/shared_ptr.h>

extern "C" void InterceptorThreadRoutine(PVOID threadContext);
extern "C" void DpcTimerInterceptor(PKDPC Dpc, PVOID Context, PVOID Arg1, PVOID Arg2);
extern "C" void ExTimerInterceptor(PEX_TIMER Timer, PVOID Context);

namespace DriverThread
{
class PerformanceCounter {
public:
    PerformanceCounter();
    ~PerformanceCounter();

    void Start();
    void Stop();
    uint64_t MeasureElapsedMs(uint64_t iterations = 0);

private:
    LARGE_INTEGER m_Start;
    LARGE_INTEGER m_Frequency;
    uint64_t m_Elapsed;
};

class Mutex
{
public:
    Mutex();
    ~Mutex();

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
    ~LockGuard();

private:
    Mutex m_Lock;
};

class ThreadArgs : public virtual eastl::enable_shared_from_this<ThreadArgs>
{
public:
    ThreadArgs()
    {
    }
    ThreadArgs(const ThreadArgs &) = delete;
    virtual ~ThreadArgs()
    {
    }
};

using ThreadRoutine = eastl::function<NTSTATUS(eastl::shared_ptr<ThreadArgs> args)>;

class Thread
{
public:
    Thread();
    Thread(const Thread &) = delete;
    ~Thread();
    NTSTATUS Start(ThreadRoutine routine, eastl::shared_ptr<ThreadArgs> args);
    NTSTATUS WaitForTermination(LONGLONG timeout = 0);
    NTSTATUS WaitForTerminationIndefinitely();
    HANDLE GetThreadId()
    {
        return m_threadId;
    }
    bool isRunning()
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
    Spinlock();
    NTSTATUS Acquire();
    void Release();
    KIRQL GetOldIrql();

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
    NTSTATUS WaitIndefinitely();
    NTSTATUS Notify();
    LONG Reset();

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
    virtual ~WorkItem()
    {
    }
    template <class T>
    eastl::shared_ptr<T> Get()
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
    WorkQueue();
    WorkQueue(const WorkQueue &) = delete;
    ~WorkQueue();
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

using DpcRoutine = eastl::function<void()>;

class DpcTimer
{
public:
    DpcTimer();
    ~DpcTimer();
    bool Start(const DpcRoutine & routine, LONGLONG timeout,
               bool periodic = false);
    bool StopAndWait();

private:
    friend void ::DpcTimerInterceptor(PKDPC Dpc, PVOID Context, PVOID Arg1, PVOID Arg2);

    KTIMER m_Timer;
    KDPC m_TimerDpc;
    DpcRoutine m_Callback;
};

using ExTimerRoutine = eastl::function<void()>;

class ExTimer
{
public:
    ExTimer();
    ~ExTimer();
    bool Start(const ExTimerRoutine & routine, LONGLONG timeout,
               bool periodic = false, bool high_precision = false);
    bool StopAndWait();

private:
    friend void ::ExTimerInterceptor(PEX_TIMER Timer, PVOID Context);

    PEX_TIMER m_Timer;
    ExTimerRoutine m_Callback;
};
}; // namespace DriverThread

#endif
