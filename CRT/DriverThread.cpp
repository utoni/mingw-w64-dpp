#include <DriverThread.hpp>

class WorkQueueArgs : public DriverThread::ThreadArgs
{
    friend class WorkQueue;

public:
    WorkQueueArgs(DriverThread::WorkQueue * wq) : m_wq(wq){};
    WorkQueueArgs(const WorkQueueArgs &) = delete;
    DriverThread::WorkQueue * getWorkQueue()
    {
        return m_wq;
    }

private:
    DriverThread::WorkQueue * m_wq;
};

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
    NTSTATUS threadReturn;
    DriverThread::Thread * self = (DriverThread::Thread *)threadContext;

    self->m_threadId = PsGetCurrentThreadId();
    threadReturn = self->m_routine(self->m_threadContext);
    self->m_threadId = nullptr;
    self->m_threadContext = nullptr;
    PsTerminateSystemThread(threadReturn);
}

NTSTATUS DriverThread::Thread::Start(ThreadRoutine routine, eastl::shared_ptr<ThreadArgs> args)
{
    HANDLE threadHandle;
    NTSTATUS status;

    LockGuard lock(m_mutex);
    if (m_threadObject != nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    m_routine = routine;
    m_threadContext = args;
    status = PsCreateSystemThread(&threadHandle, (ACCESS_MASK)0, NULL, (HANDLE)0, NULL, InterceptorThreadRoutine, this);

    if (!NT_SUCCESS(status))
    {
        threadHandle = nullptr;
        return status;
    }

    status =
        ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *)&m_threadObject, NULL);

    if (!NT_SUCCESS(status))
    {
        threadHandle = nullptr;
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
        KeWaitForSingleObject(m_threadObject, Executive, KernelMode, FALSE, &li_timeout);

    ObDereferenceObject(m_threadObject);
    m_threadObject = nullptr;
    return status;
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
    return KeWaitForSingleObject(&m_semaphore, Executive, KernelMode, FALSE, &li_timeout);
}

LONG DriverThread::Semaphore::Release(LONG adjustment)
{
    return KeReleaseSemaphore(&m_semaphore, 0, adjustment, FALSE);
}

// Event

DriverThread::Event::Event()
{
    KeInitializeEvent(&m_event, NotificationEvent, FALSE);
}

NTSTATUS DriverThread::Event::Wait(LONGLONG timeout)
{
    LARGE_INTEGER li_timeout = {.QuadPart = timeout};
    return KeWaitForSingleObject(&m_event, Executive, KernelMode, FALSE, &li_timeout);
}

NTSTATUS DriverThread::Event::Notify()
{
    return KeSetEvent(&m_event, 0, FALSE);
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

DriverThread::WorkQueue::WorkQueue(void)
    : m_mutex(), m_queue(), m_wakeEvent(), m_stopWorker(false), m_worker(), m_workerRoutine(nullptr)
{
}

DriverThread::WorkQueue::~WorkQueue(void)
{
    Stop();
}

NTSTATUS DriverThread::WorkQueue::Start(WorkerRoutine routine)
{
    NTSTATUS status;

    {
        LockGuard lock(m_mutex);
        m_workerRoutine = routine;
        auto wqa = eastl::make_shared<WorkQueueArgs>(this);
        status = m_worker.Start(WorkerInterceptorRoutine, wqa);
    }

    if (!NT_SUCCESS(status) && status != STATUS_UNSUCCESSFUL)
    {
        Stop();
    }

    return status;
}

void DriverThread::WorkQueue::Stop(bool wait)
{
    LockGuard lock(m_mutex);
    if (m_stopWorker == true)
    {
        return;
    }
    m_stopWorker = true;
    m_wakeEvent.Notify();
    if (wait)
    {
        m_worker.WaitForTermination();
    }
}

void DriverThread::WorkQueue::Enqueue(WorkItem & item)
{
    {
        LockGuard lock(m_mutex);
        m_queue.emplace_back(item);
    }
    m_wakeEvent.Notify();
}

void DriverThread::WorkQueue::Enqueue(eastl::deque<WorkItem> & items)
{
    {
        LockGuard lock(m_mutex);
        m_queue.insert(m_queue.end(), items.begin(), items.end());
    }
    m_wakeEvent.Notify();
}

NTSTATUS DriverThread::WorkQueue::WorkerInterceptorRoutine(eastl::shared_ptr<ThreadArgs> args)
{
    auto wqa = eastl::static_pointer_cast<WorkQueueArgs>(args);
    WorkQueue * wq = wqa->getWorkQueue();

    PAGED_CODE();

    for (;;)
    {
        eastl::deque<WorkItem> doQueue;
        std::size_t nItems;

        {
            LockGuard lock(wq->m_mutex);
            nItems = wq->m_queue.size();
        }

        if (nItems == 0)
        {
            if (wq->m_stopWorker == true)
            {
                break;
            }

            wq->m_wakeEvent.Wait();
            continue;
        }

        {
            LockGuard lock(wq->m_mutex);
            doQueue = wq->m_queue;
            wq->m_queue.clear();
        }

        while (doQueue.size() > 0)
        {
            WorkItem & item = doQueue.front();

            if (wq->m_workerRoutine(item) != STATUS_SUCCESS)
            {
                wq->m_stopWorker = true;
            }

            doQueue.pop_front();
        }
    }

    return STATUS_SUCCESS;
}
