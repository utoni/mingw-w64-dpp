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

NTSTATUS DriverThread::Thread::Start(threadRoutine_t routine, PVOID threadContext)
{
    HANDLE threadHandle;
    NTSTATUS status;

    LockGuard lock(m_mutex);
    if (m_threadObject != nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    m_routine = routine;
    m_threadContext = threadContext;
    status = PsCreateSystemThread(&threadHandle, (ACCESS_MASK)0, NULL, (HANDLE)0, NULL, InterceptorThreadRoutine, this);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status =
        ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *)&m_threadObject, NULL);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return ZwClose(threadHandle);
}

NTSTATUS DriverThread::Thread::WaitForTermination(LONGLONG timeout)
{
    if (PsGetCurrentThreadId() == m_threadId)
    {
        return STATUS_UNSUCCESSFUL;
    }

    LockGuard lock(m_mutex);
    if (m_threadObject == nullptr)
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

NTSTATUS DriverThread::Spinlock::Acquire(void)
{
    return KeAcquireSpinLock(&m_spinLock, &m_oldIrql);
}

void DriverThread::Spinlock::Release(void)
{
    KeReleaseSpinLock(&m_spinLock, m_oldIrql);
}

KIRQL DriverThread::Spinlock::GetOldIrql(void)
{
    return m_oldIrql;
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

// WorkQueue

DriverThread::WorkQueue::WorkQueue(void) : m_worker()
{
    InitializeSListHead(&m_work);
    KeInitializeEvent(&m_wakeEvent, SynchronizationEvent, FALSE);
    m_stopWorker = FALSE;
}

DriverThread::WorkQueue::~WorkQueue(void)
{
    Stop();
}

NTSTATUS DriverThread::WorkQueue::Start(workerRoutine_t workerRoutine)
{
    NTSTATUS status;

    {
        LockGuard lock(m_mutex);
        m_workerRoutine = workerRoutine;
        status = m_worker.Start(WorkerInterceptorRoutine, this);
    }

    if (!NT_SUCCESS(status) && status != STATUS_UNSUCCESSFUL)
    {
        Stop();
    }

    return status;
}

void DriverThread::WorkQueue::Stop(void)
{
    LockGuard lock(m_mutex);
    if (m_stopWorker == TRUE)
    {
        return;
    }
    m_stopWorker = TRUE;
    KeSetEvent(&m_wakeEvent, 0, FALSE);
}

void DriverThread::WorkQueue::Enqueue(PSLIST_ENTRY workItem)
{
    if (InterlockedPushEntrySList(&m_work, workItem) == NULL)
    {
        // Work queue was empty. So, signal the work queue event in case the
        // worker thread is waiting on the event for more operations.
        KeSetEvent(&m_wakeEvent, 0, FALSE);
    }
}

NTSTATUS DriverThread::WorkQueue::WorkerInterceptorRoutine(PVOID workerContext)
{
    DriverThread::WorkQueue * wq = (DriverThread::WorkQueue *)workerContext;
    PSLIST_ENTRY listEntryRev, listEntry, next;

    PAGED_CODE();

    for (;;)
    {
        // Flush all the queued operations into a local list
        listEntryRev = InterlockedFlushSList(&wq->m_work);

        if (listEntryRev == NULL)
        {

            // There's no work to do. If we are allowed to stop, then stop.
            if (wq->m_stopWorker == TRUE)
            {
                break;
            }

            // Otherwise, wait for more operations to be enqueued.
            KeWaitForSingleObject(&wq->m_wakeEvent, Executive, KernelMode, FALSE, 0);
            continue;
        }

        // Need to reverse the flushed list in order to preserve the FIFO order
        listEntry = NULL;
        while (listEntryRev != NULL)
        {
            next = listEntryRev->Next;
            listEntryRev->Next = listEntry;
            listEntry = listEntryRev;
            listEntryRev = next;
        }

        // Now process the correctly ordered list of operations one by one
        while (listEntry)
        {
            PSLIST_ENTRY arg = listEntry;
            listEntry = listEntry->Next;
            if (wq->m_workerRoutine(arg) != STATUS_SUCCESS)
            {
                wq->m_stopWorker = TRUE;
            }
        }
    }

    return STATUS_SUCCESS;
}
