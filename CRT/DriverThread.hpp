#ifndef DDK_THREAD
#define DDK_THREAD 1

#include <ntddk.h>

extern "C" void InterceptorThreadRoutine(PVOID threadContext);

typedef NTSTATUS (*threadRoutine_t)(PVOID);
typedef NTSTATUS (*workerRoutine_t)(PSLIST_ENTRY);

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

class Thread
{
public:
    Thread(void);
    ~Thread(void);
    NTSTATUS Start(threadRoutine_t routine, PVOID threadContext);
    NTSTATUS WaitForTermination(LONGLONG timeout = 0);
    HANDLE GetThreadId(void);

private:
    friend void ::InterceptorThreadRoutine(PVOID threadContext);

    HANDLE m_threadId = nullptr;
    PETHREAD m_threadObject = nullptr;
    Mutex m_mutex;
    threadRoutine_t m_routine;
    PVOID m_threadContext;
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
    NTSTATUS Wait(LONGLONG timeout = 0);
    LONG Release(LONG adjustment = 1);

private:
    KSEMAPHORE m_semaphore;
};

class WorkQueue
{
public:
    WorkQueue(void);
    ~WorkQueue(void);
    NTSTATUS Start(workerRoutine_t workerRoutine);
    void Stop(void);
    void Enqueue(PSLIST_ENTRY workItem);

private:
    Mutex m_mutex;
    SLIST_HEADER m_work;
    KEVENT m_wakeEvent;
    BOOLEAN m_stopWorker; // Work LIST must be empty and StopWorker TRUE to be able to stop!
    Thread m_worker;
    workerRoutine_t m_workerRoutine;

    static NTSTATUS WorkerInterceptorRoutine(PVOID workerContext);
};

}; // namespace DriverThread

#endif
