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

static void threadRoutine(PVOID threadContext)
{
    DbgPrint("ThreadRoutine %p, ThreadContext: %p\n", threadRoutine, threadContext);
    for (size_t i = 3; i > 0; --i)
    {
        DbgPrint("ThreadLoop: %zu\n", i);
    }
    DbgPrint("Fin.\n");
    DriverThread::Semaphore * const sem = (DriverThread::Semaphore *)threadContext;
    sem->Release();
    TERMINATE_MYSELF(STATUS_SUCCESS);
}

static void test_cplusplus(void)
{
    TestSmth t;
    t.doSmth();

    DriverThread::Semaphore sem;
    DriverThread::Thread dt;
    dt.Start(threadRoutine, (PVOID)&sem);
    sem.Wait();
    DbgPrint("Thread signaled semaphore.\n");
    dt.WaitForTermination();
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
