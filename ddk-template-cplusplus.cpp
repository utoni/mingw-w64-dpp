#include <ntddk.h>

#include <DriverThread.hpp>

class TestSmth
{
public:
    TestSmth()
    {
    }
    void doSmth(void)
    {
        DbgPrint("%s\n", "Hello Class!");
    }
};

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

    struct threadContext ctx;
    ctx.dth.Start(threadRoutine, (PVOID)&ctx);
    ctx.sem.Wait();
    DbgPrint("MainThread semaphore signaled.\n");
    ctx.dth.WaitForTermination();
    ctx.dth.WaitForTermination();
    DbgPrint("MainThread EOF\n");
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

        /* support for service stopping */
        DriverObject->DriverUnload = DriverUnload;

        test_cplusplus();

        return STATUS_SUCCESS;
    }

    VOID DriverUnload(_In_ struct _DRIVER_OBJECT * DriverObject)
    {
        (void)DriverObject;

        DbgPrint("%s\n", "Bye ring0!");
    }
}
