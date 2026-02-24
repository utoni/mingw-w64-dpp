#include <DriverThread.hpp>

extern "C" {
    extern PEX_TIMER WrapperExAllocateTimer (_In_opt_ PEXT_CALLBACK Callback,
                                             _In_opt_ PVOID CallbackContext, _In_ ULONG Attributes);
    extern BOOLEAN WrapperExCancelTimer (_In_ PEX_TIMER Timer, _In_opt_ PEXT_CANCEL_PARAMETERS Parameters);
    extern BOOLEAN WrapperExSetTimer (_In_ PEX_TIMER Timer, _In_ LONGLONG DueTime, _In_ LONGLONG Period,
                                      _In_opt_ PEXT_SET_PARAMETERS Parameters);
    extern BOOLEAN WrapperExDeleteTimer (_In_ PEX_TIMER Timer, _In_ BOOLEAN Cancel,
                                         _In_ BOOLEAN Wait, _In_ PEXT_DELETE_PARAMETERS Parameters);

    extern void ExInitializeDeleteTimerParameters(PEXT_DELETE_PARAMETERS);
};

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

// PerformanceCounter

DriverThread::PerformanceCounter::PerformanceCounter()
    : m_Start{}, m_Frequency{}, m_Elapsed{0}
{
}

DriverThread::PerformanceCounter::~PerformanceCounter()
{
}

void DriverThread::PerformanceCounter::Start()
{
    m_Start = KeQueryPerformanceCounter(&m_Frequency);
}

void DriverThread::PerformanceCounter::Stop()
{
    const LARGE_INTEGER end = KeQueryPerformanceCounter(NULL);
    LARGE_INTEGER elapsed_us;
    elapsed_us.QuadPart = end.QuadPart - m_Start.QuadPart;
    elapsed_us.QuadPart *= 1000000;
    elapsed_us.QuadPart /= m_Frequency.QuadPart;
    m_Elapsed += elapsed_us.QuadPart;
}

uint64_t DriverThread::PerformanceCounter::MeasureElapsedMs(uint64_t iterations)
{
    float iter_per_us = (float)m_Elapsed / 1000.0f;
    if (iterations > 0)
        iter_per_us = (float)iterations / iter_per_us;
    m_Elapsed = 0;
    return iter_per_us;
}

// Thread

DriverThread::Thread::Thread()
{
}

DriverThread::Thread::~Thread()
{
    WaitForTerminationIndefinitely();
}

extern "C" void InterceptorThreadRoutine(PVOID threadContext)
{
    NTSTATUS threadReturn;
    DriverThread::Thread * self = (DriverThread::Thread *)threadContext;

    if (!self)
        return;

    {
        DriverThread::LockGuard lock(self->m_mutex);
        self->m_threadId = PsGetCurrentThreadId();
    }
    threadReturn = self->m_routine(self->m_threadContext);
    {
        DriverThread::LockGuard lock(self->m_mutex);
        self->m_threadId = nullptr;
        self->m_threadContext = nullptr;
    }
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
        ObReferenceObjectByHandle(threadHandle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                  *PsThreadType, KernelMode, (PVOID *)&m_threadObject, NULL);

    if (!NT_SUCCESS(status))
    {
        threadHandle = nullptr;
        return status;
    }

    return ZwClose(threadHandle);
}

NTSTATUS DriverThread::Thread::WaitForTermination(LONGLONG timeout)
{
    PETHREAD localThreadObject = nullptr;

    {
        LockGuard lock(m_mutex);
        if (PsGetCurrentThreadId() == m_threadId)
        {
            return STATUS_UNSUCCESSFUL;
        }
        if (m_threadObject == nullptr)
        {
            return STATUS_UNSUCCESSFUL;
        }
        localThreadObject = m_threadObject;
        m_threadObject = nullptr;
    }

    LARGE_INTEGER li_timeout = {.QuadPart = timeout};
    NTSTATUS status =
        KeWaitForSingleObject(localThreadObject, Executive, KernelMode, FALSE, &li_timeout);
    ObDereferenceObject(localThreadObject);
    return status;
}

NTSTATUS DriverThread::Thread::WaitForTerminationIndefinitely()
{
    PETHREAD localThreadObject = nullptr;

    {
        LockGuard lock(m_mutex);
        if (PsGetCurrentThreadId() == m_threadId)
        {
            return STATUS_UNSUCCESSFUL;
        }
        if (m_threadObject == nullptr)
        {
            return STATUS_UNSUCCESSFUL;
        }
        localThreadObject = m_threadObject;
        m_threadObject = nullptr;
    }

    NTSTATUS status =
        KeWaitForSingleObject(localThreadObject, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(localThreadObject);
    return status;
}

// Spinlock

DriverThread::Spinlock::Spinlock()
{
    KeInitializeSpinLock(&m_spinLock);
}

NTSTATUS DriverThread::Spinlock::Acquire()
{
    return KeAcquireSpinLock(&m_spinLock, &m_oldIrql);
}

void DriverThread::Spinlock::Release()
{
    KeReleaseSpinLock(&m_spinLock, m_oldIrql);
}

KIRQL DriverThread::Spinlock::GetOldIrql()
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

NTSTATUS DriverThread::Event::WaitIndefinitely()
{
    return KeWaitForSingleObject(&m_event, Executive, KernelMode, FALSE, NULL);
}

NTSTATUS DriverThread::Event::Notify()
{
    return KeSetEvent(&m_event, 0, FALSE);
}

LONG DriverThread::Event::Reset()
{
    return KeResetEvent(&m_event);
}

// Mutex

DriverThread::Mutex::Mutex()
{
}

DriverThread::Mutex::~Mutex()
{
}

void DriverThread::Mutex::Lock()
{
    while (m_interlock == 1 || InterlockedCompareExchange(&m_interlock, 1, 0) == 1) {}
}

void DriverThread::Mutex::Unlock()
{
    m_interlock = 0;
}

// LockGuard

DriverThread::LockGuard::LockGuard(Mutex & m) : m_Lock(m)
{
    m_Lock.Lock();
}

DriverThread::LockGuard::~LockGuard()
{
    m_Lock.Unlock();
}

// WorkQueue

DriverThread::WorkQueue::WorkQueue()
    : m_mutex(), m_queue(), m_wakeEvent(), m_stopWorker(false), m_worker(), m_workerRoutine(nullptr)
{
}

DriverThread::WorkQueue::~WorkQueue()
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
    {
        LockGuard lock(m_mutex);
        if (m_stopWorker == true)
        {
            return;
        }
        m_stopWorker = true;
    }

    m_wakeEvent.Notify();
    if (wait)
    {
        m_worker.WaitForTerminationIndefinitely();
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

            if (wq->m_wakeEvent.Reset() == 0) {
                wq->m_wakeEvent.WaitIndefinitely();
                continue;
            }
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

DriverThread::DpcTimer::DpcTimer()
{
    KeInitializeTimer(&m_Timer);
}

DriverThread::DpcTimer::~DpcTimer()
{
    StopAndWait();
}

extern "C" void DpcTimerInterceptor(PKDPC Dpc, PVOID Context, PVOID Arg1, PVOID Arg2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Arg1);
    UNREFERENCED_PARAMETER(Arg2);

    auto self = (DriverThread::DpcTimer *)Context;
    self->m_Callback();
}

bool DriverThread::DpcTimer::Start(const DpcRoutine & routine, LONGLONG timeout, bool periodic)
{
    LONG periodMs = 0;
    LARGE_INTEGER expiration;
    expiration.QuadPart = timeout;
    m_Callback = routine;
    if (periodic)
        periodMs = (LONG)((-timeout) / 10000);
    KeInitializeDpc(&m_TimerDpc, DpcTimerInterceptor, this);
    return KeSetTimerEx(&m_Timer, expiration, periodMs, &m_TimerDpc);
}

bool DriverThread::DpcTimer::StopAndWait()
{
    auto queued = KeCancelTimer(&m_Timer);
    KeFlushQueuedDpcs();
    return queued;
}

DriverThread::ExTimer::ExTimer() : m_Timer{}, m_Callback{nullptr}
{
}

DriverThread::ExTimer::~ExTimer()
{
    StopAndWait();
}

extern "C" void ExTimerInterceptor(PEX_TIMER Timer, PVOID Context)
{
    UNREFERENCED_PARAMETER(Timer);

    auto self = (DriverThread::ExTimer *)Context;
    self->m_Callback();
}

bool DriverThread::ExTimer::Start(const ExTimerRoutine & routine, LONGLONG timeout,
                                  bool periodic, bool high_precision)
{
    m_Callback = routine;
    m_Timer = WrapperExAllocateTimer(ExTimerInterceptor, this,
                                     (high_precision ? EX_TIMER_HIGH_RESOLUTION : 0));
    if (m_Timer == NULL)
        return false;

    if (WrapperExSetTimer(m_Timer, timeout, (periodic ? -timeout : 0), NULL) == FALSE)
        return false;

    return true;
}

bool DriverThread::ExTimer::StopAndWait()
{
    bool rv = false;

    if (m_Timer != NULL) {
        rv = WrapperExCancelTimer(m_Timer, NULL);
        EXT_DELETE_PARAMETERS params = {};
        WrapperExDeleteTimer(m_Timer, TRUE, TRUE, &params);
        m_Timer = NULL;
    }

    return rv;
}
