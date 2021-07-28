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

static DerivedWithCDtor some_static(0xDEADC0DE);

struct threadContext
{
    DriverThread::Semaphore sem;
    DriverThread::Thread dth;
};

static NTSTATUS threadRoutine(PVOID threadContext)
{
    DbgPrint("ThreadRoutine %p, ThreadContext: %p\n", threadRoutine, threadContext);
    for (size_t i = 3; i > 0; --i)
    {
        DbgPrint("ThreadLoop: %zu\n", i);
    }
    struct threadContext * const ctx = (struct threadContext *)threadContext;
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

    struct threadContext ctx;
    ctx.dth.Start(threadRoutine, (PVOID)&ctx);
    ctx.sem.Wait();
    DbgPrint("MainThread semaphore signaled.\n");
    ctx.dth.WaitForTermination();
    ctx.dth.WaitForTermination();
    DbgPrint("MainThread EOF\n");

    some_static.doSmth();
}

extern "C"
{

    DRIVER_INITIALIZE DriverEntry;
    DRIVER_UNLOAD DriverUnload;

    NTSTATUS DriverEntry(_In_ struct _DRIVER_OBJECT * DriverObject, _In_ PUNICODE_STRING RegistryPath)
    {
        (void)DriverObject;
        (void)RegistryPath;

        DbgPrint("%s\n", "Hello ring0!");
        cdtor_test = new TestSmth();

        test_cplusplus();

        return STATUS_SUCCESS;
    }

    VOID DriverUnload(_In_ struct _DRIVER_OBJECT * DriverObject)
    {
        (void)DriverObject;

        delete cdtor_test;
        DbgPrint("%s\n", "Bye ring0!");
    }
}
