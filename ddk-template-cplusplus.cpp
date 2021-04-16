#include <ntddk.h>

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

static void test_cplusplus(void)
{
    TestSmth t;
    t.doSmth();
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
