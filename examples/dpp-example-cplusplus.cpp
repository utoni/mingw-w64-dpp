#include <ntddk.h>

#include <EASTL/string.h>
#include <eastl_compat.hpp>
#include <DriverThread.hpp>

static DriverThread::Thread dpc_thread;
static DriverThread::Thread worker_thread;
static DriverThread::ExTimer timer;

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

        const auto & eastl_to_string = eastl::to_string(0xDEADC0DE);
        DbgPrint("Using eastl::to_string should return a warning: %s\n", eastl_to_string.c_str());

        eastl::wstring eastl_unicode = L"test_eastl_unicode_string";
        DbgPrint("eastl::wstring: %S\n", eastl_unicode.c_str());

        wchar_t test_unicode_str[] = L"test_unicode_string";
        unsigned short test_unicode_strlen = sizeof(test_unicode_str);
        const auto & eastl_from_unicode = ::from_unicode(test_unicode_str, test_unicode_strlen);
        DbgPrint("unicode2ansi: \"%s\"\n", eastl_from_unicode.c_str());

        const auto & number_ud = ::to_string(1337u);
        DbgPrint("Value 1337u to String: %s\n", number_ud.c_str());
        const auto & number_d = ::to_string(1337);
        DbgPrint("Value 1337 to String: %s\n", number_d.c_str());
        const auto & number_l = ::to_string(1337l);
        DbgPrint("Value 1337l to String: %s\n", number_l.c_str());
        const auto & number_lu = ::to_string(1337lu);
        DbgPrint("Value 1337lu to String: %s\n", number_lu.c_str());
        const auto & number_ll = ::to_string(1337ll);
        DbgPrint("Value 1337ll to String: %s\n", number_ll.c_str());
        const auto & number_llu = ::to_string(1337llu);
        DbgPrint("Value 1337llu to String: %s\n", number_llu.c_str());
        const auto & number_f = ::to_string(1337.1337f);
        DbgPrint("Value 1337f to String: %s\n", number_f.c_str());
        const auto & number_lf = ::to_string(1337.1337);
        DbgPrint("Value 1337lf to String: %s\n", number_lf.c_str());

        DbgPrint("Value 0x00000000DEADBEEF to Hex String: 0x%s\n", ::to_string_hex(0xdeadbeef, 16));
        DbgPrint("Value 0xDEADC0DEDEADBEEF to Hex String: 0x%s\n", ::to_string_hex(0xdeadc0dedeadbeef));
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

    dpc_thread.Start([](eastl::shared_ptr<DriverThread::ThreadArgs>)
    {
        static size_t dpc_calls = 0;
        DriverThread::DpcTimer timer;
        auto ret = timer.Start([]() {
            dpc_calls++;
        }, -166666, true);
        if (ret)
            DbgPrint("Dpc Timer start failed, already started?\n");
        LARGE_INTEGER li;
        li.QuadPart = -166666 * 60 * 3;
        KeDelayExecutionThread(KernelMode, FALSE, &li);
        timer.StopAndWait();
        DbgPrint("DPC Timer Routine calls: %zu\n", dpc_calls);
        return 0;
    }, nullptr);

    worker_thread.Start([](eastl::shared_ptr<DriverThread::ThreadArgs>)
    {
        DriverThread::PerformanceCounter perfcnt;
        perfcnt.Start();

        LARGE_INTEGER li;
        li.QuadPart = -166666 * 60 * 1;
        KeDelayExecutionThread(KernelMode, FALSE, &li);

        perfcnt.Stop();
        DbgPrint("PerformanceCounter measures: %llums\n", perfcnt.MeasureElapsedMs());

        perfcnt.Start();

        li.QuadPart = -166666 * 60 * 1;
        KeDelayExecutionThread(KernelMode, FALSE, &li);

        perfcnt.Stop();
        DbgPrint("PerformanceCounter measures: %llu/ms\n", perfcnt.MeasureElapsedMs(10000));

        return 0;
    }, nullptr);

    uint64_t run_count = 0;
    DriverThread::PerformanceCounter perfcnt;
    perfcnt.Start();
    timer.Start([&run_count]() {
        run_count++;
    }, -100000, true, true);
    LARGE_INTEGER li;
    li.QuadPart = -166666 * 60 * 3;
    KeDelayExecutionThread(KernelMode, FALSE, &li);
    perfcnt.Stop();
    DbgPrint("HPET PerformanceCounter measures: %llums\n", perfcnt.MeasureElapsedMs());
    timer.StopAndWait();
    DbgPrint("HPET Callback runs: %llu\n", run_count);
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

        dpc_thread.WaitForTerminationIndefinitely();
        worker_thread.WaitForTerminationIndefinitely();

        delete cdtor_test;
        DbgPrint("%s\n", "Bye ring0!");
    }
}
