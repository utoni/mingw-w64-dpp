#include <DriverThread.hpp>

DriverThread::Thread::Thread(void)
{
}

NTSTATUS DriverThread::Thread::Start(PKSTART_ROUTINE threadRoutine, PVOID threadContext)
{
    HANDLE threadHandle;
    NTSTATUS status;

    status = PsCreateSystemThread(&threadHandle, (ACCESS_MASK)0, NULL, (HANDLE)0, NULL, threadRoutine, threadContext);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *)&m_threadObject, NULL);
    return ZwClose(threadHandle);
}

NTSTATUS DriverThread::Thread::WaitForTermination(LONGLONG timeout)
{
    LARGE_INTEGER li_timeout = {.QuadPart = timeout};
    NTSTATUS status =
        KeWaitForSingleObject(m_threadObject, Executive, KernelMode, FALSE, (timeout == 0 ? NULL : &li_timeout));

    ObDereferenceObject(m_threadObject);
    return status;
}

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
