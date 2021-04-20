#include <DriverThread.hpp>

// Thread

DriverThread::Thread::Thread(void)
{
}

DriverThread::Thread::~Thread(void)
{
    WaitForTermination();
}

extern "C" void InterceptorThreadRoutine(PVOID threadContext)
{
    DriverThread::Thread * self = (DriverThread::Thread *)threadContext;

    self->m_threadId = PsGetCurrentThreadId();
    PsTerminateSystemThread(self->m_routine(self->m_threadContext));
}

NTSTATUS DriverThread::Thread::Start(threadRoutine routine, PVOID threadContext)
{
    HANDLE threadHandle;
    NTSTATUS status;

    LockGuard lock(m_mutex);
    m_routine = routine;
    m_threadContext = threadContext;
    status = PsCreateSystemThread(&threadHandle, (ACCESS_MASK)0, NULL, (HANDLE)0, NULL, InterceptorThreadRoutine, this);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *)&m_threadObject, NULL);
    return ZwClose(threadHandle);
}

NTSTATUS DriverThread::Thread::WaitForTermination(LONGLONG timeout)
{
    LockGuard lock(m_mutex);
    if (m_threadObject == nullptr || PsGetCurrentThreadId() == m_threadId)
    {
        return STATUS_UNSUCCESSFUL;
    }

    LARGE_INTEGER li_timeout = {.QuadPart = timeout};
    NTSTATUS status =
        KeWaitForSingleObject(m_threadObject, Executive, KernelMode, FALSE, (timeout == 0 ? NULL : &li_timeout));

    ObDereferenceObject(m_threadObject);
    m_threadObject = nullptr;
    return status;
}

HANDLE DriverThread::Thread::GetThreadId(void)
{
    return m_threadId;
}

// Spinlock

DriverThread::Spinlock::Spinlock(void)
{
    KeInitializeSpinLock(&m_spinLock);
}

NTSTATUS DriverThread::Spinlock::Acquire(KIRQL * const oldIrql)
{
    return KeAcquireSpinLock(&m_spinLock, oldIrql);
}

void DriverThread::Spinlock::Release(KIRQL * const oldIrql)
{
    KeReleaseSpinLock(&m_spinLock, *oldIrql);
}

// Semaphore

DriverThread::Semaphore::Semaphore(LONG initialValue, LONG maxValue)
{
    KeInitializeSemaphore(&m_semaphore, initialValue, maxValue);
}

NTSTATUS DriverThread::Semaphore::Wait(LONGLONG timeout)
{
    LARGE_INTEGER li_timeout = {.QuadPart = timeout};
    return KeWaitForSingleObject(&m_semaphore, Executive, KernelMode, FALSE, (timeout == 0 ? NULL : &li_timeout));
}

LONG DriverThread::Semaphore::Release(LONG adjustment)
{
    return KeReleaseSemaphore(&m_semaphore, 0, adjustment, FALSE);
}

// Mutex

DriverThread::Mutex::Mutex(void)
{
}

DriverThread::Mutex::~Mutex(void)
{
}

void DriverThread::Mutex::Lock(void)
{
    while (m_interlock == 1 || InterlockedCompareExchange(&m_interlock, 1, 0) == 1) {}
}

void DriverThread::Mutex::Unlock(void)
{
    m_interlock = 0;
}

// LockGuard

DriverThread::LockGuard::LockGuard(Mutex & m) : m_Lock(m)
{
    m_Lock.Lock();
}

DriverThread::LockGuard::~LockGuard(void)
{
    m_Lock.Unlock();
}
