#ifndef DDK_THREAD
#define DDK_THREAD 1

#include <ntddk.h>

#define TERMINATE_MYSELF(ntstatus) PsTerminateSystemThread(ntstatus);

namespace DriverThread
{

class Thread
{
public:
    Thread();
    NTSTATUS Start(PKSTART_ROUTINE threadRoutine, PVOID threadContext);
    NTSTATUS WaitForTermination(LONGLONG timeout = 0);

private:
    PETHREAD m_threadObject;
};

class Spinlock
{
public:
    Spinlock(void);
    NTSTATUS Acquire(KIRQL * const oldIrql);
    void Release(KIRQL * const oldIrql);

private:
    KSPIN_LOCK m_spinLock;
};

class Semaphore
{
public:
    explicit Semaphore(LONG initialValue = 0, LONG maxValue = MAXLONG);
    NTSTATUS Wait(LONGLONG timeout = 0);
    LONG Release(LONG adjustment = 1);

private:
    KSEMAPHORE m_semaphore;
};

}; // namespace DriverThread

#endif
