#include <ntddk.h>

#include <DriverThread.hpp>

class TestSmth
{
public:
    TestSmth()
    {
        DbgPrint("%s\n", "ctor");
    }
    ~TestSmth()
    {
        DbgPrint("%s\n", "dtor");
    }
    void doSmth(void)
    {
        DbgPrint("%s\n", "Hello Class!");
    }
};
static TestSmth * cdtor_test;

class Derived : public TestSmth
{
public:
    Derived()
    {
    }
    ~Derived()
    {
    }
    void doSmth(void)
    {
        DbgPrint("%s\n", "Hello Derived!");
    }
};

class DerivedWithCDtor : public Derived
{
public:
    explicit DerivedWithCDtor(unsigned int value)
    {
        some_value = value;
        DbgPrint("%s\n", "DerivedWithCDtor-Ctor.");
    }
    ~DerivedWithCDtor()
    {
        DbgPrint("%s\n", "DerivedWithCDtor-Dtor.");
    }
    void doSmth(void)
    {
        DbgPrint("SomeValue: %X\n", some_value);
    }

private:
    unsigned int some_value = 0;
};

class MyWorkItem
{
public:
    MyWorkItem()
    {
        DbgPrint("MyWorkItem ctor\n");
    }
    ~MyWorkItem()
    {
        DbgPrint("MyWorkItem dtor\n");
    }

    UINT32 counter = 0, another_counter = 0;
};

static DriverThread::WorkQueue global_work_queue;
static DerivedWithCDtor some_static(0xDEADC0DE);

class threadContext : public DriverThread::ThreadArgs
{
public:
    DriverThread::Semaphore sem;
    DriverThread::Thread dth;
};

static NTSTATUS threadRoutine(eastl::shared_ptr<DriverThread::ThreadArgs> args)
{
    DbgPrint("ThreadRoutine %p, ThreadContext: %p\n", threadRoutine, args);
    auto ctx = eastl::static_pointer_cast<threadContext>(args);
    for (size_t i = 3; i > 0; --i)
    {
        DbgPrint("ThreadLoop: %zu (isRunning: %u)\n", i, ctx->dth.isRunning());
    }
    DbgPrint("Fin. ThreadId: %p\n", ctx->dth.GetThreadId());
    ctx->sem.Release();
    DbgPrint("Thread WaitForTermination: 0x%X\n", ctx->dth.WaitForTermination()); // must return STATUS_UNSUCCESSFUL;

    return STATUS_SUCCESS;
}

static void test_cplusplus(void)
{
    TestSmth t;
    t.doSmth();
    Derived d;
    d.doSmth();

    auto ctx = eastl::make_shared<threadContext>();
    ctx->dth.Start(threadRoutine, ctx);
    ctx->sem.Wait();
    DbgPrint("MainThread semaphore signaled.\n");
    ctx->dth.WaitForTermination();
    ctx->dth.WaitForTermination();
    DbgPrint("MainThread EOF\n");

    DriverThread::WorkQueue work_queue;
    DbgPrint("WorkQueue test.\n");
    {
        DriverThread::WorkItem wi(eastl::make_shared<MyWorkItem>());

        auto user = wi.Get<MyWorkItem>();
        user->counter = 3;
        user->another_counter = 1;
        work_queue.Enqueue(wi);
        global_work_queue.Enqueue(wi);
    }
    {
        eastl::deque<DriverThread::WorkItem> items;

        for (size_t i = 1; i < 3; ++i)
        {
            DriverThread::WorkItem wi(eastl::make_shared<MyWorkItem>());

            auto user = wi.Get<MyWorkItem>();
            user->counter = 3 + i;
            user->another_counter = 1 + i;
            items.emplace_back(wi);
        }

        work_queue.Enqueue(items);
        global_work_queue.Enqueue(items);
    }

    work_queue.Start(
        [](DriverThread::WorkItem & item)
        {
            DbgPrint("Worker callback.\n");

            eastl::shared_ptr<MyWorkItem> wi;
            item.Get<MyWorkItem>(wi);
            while (wi->counter-- > 0)
            {
                DbgPrint("WorkItem Counter: %u\n", wi->counter);
            }

            DbgPrint("Worker finished.\n");
            return STATUS_SUCCESS;
        });
    work_queue.Stop();

    global_work_queue.Start(
        [](DriverThread::WorkItem & item)
        {
            DbgPrint("Global Worker callback.\n");

            eastl::shared_ptr<MyWorkItem> wi;
            item.Get<MyWorkItem>(wi);
            while (wi->another_counter-- > 0)
            {
                DbgPrint("WorkItem Another Counter: %u\n", wi->another_counter);
            }

            DbgPrint("Global Worker finished.\n");
            return STATUS_SUCCESS;
        });

    some_static.doSmth();
}

extern "C"
{

    DRIVER_INITIALIZE DriverEntry;
    DRIVER_UNLOAD DriverUnload;

    NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
    {
        (void)DriverObject;
        (void)RegistryPath;

        DbgPrint("%s\n", "Hello ring0!");
        cdtor_test = new TestSmth();

        test_cplusplus();

        return STATUS_SUCCESS;
    }

    VOID DriverUnload(PDRIVER_OBJECT DriverObject)
    {
        (void)DriverObject;

        delete cdtor_test;
        DbgPrint("%s\n", "Bye ring0!");
    }
}
