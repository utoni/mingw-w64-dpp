#include <ntddk.h>

#include <except.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

extern NTSTATUS NTAPI ZwProtectVirtualMemory(_In_ HANDLE ProcessHandle,
                                             _In_ _Out_ PVOID * BaseAddress,
                                             _In_ _Out_ PULONG NumberOfBytesToProtect,
                                             _In_ ULONG NewAccessProtection,
                                             _Out_ PULONG OldAccessProtection);

int example_exception_handler(_In_ EXCEPTION_POINTERS * lpEP)
{
    (void)lpEP;
    DbgPrint("Exception handler called!\n");
    return EXCEPTION_EXECUTE_HANDLER;
}

static void another_seh_test()
{
    DbgPrint("Another SEH test..\n");
    __dpptry(example_exception_handler, anotherseh)
    {
        *(int *)0 = 0;
    }
    __dppexcept(anotherseh)
    {
        DbgPrint("Success!\n");
    }
    __dpptryend(anotherseh);
}

NTSTATUS DriverEntry(struct _DRIVER_OBJECT * DriverObject, PUNICODE_STRING RegistryPath)
{
    (void)DriverObject;
    (void)RegistryPath;

    DbgPrint("%s\n", "Hello ring0!");

    DbgPrint("Testing SEH..\n");
    __dpptry(example_exception_handler, testseh)
    {
        *(int *)0 = 0;
        DbgPrint("You should never see this text!\n");
    }
    __dppexcept(testseh)
    {
        DbgPrint("Success! SEH seems to work.\n");
    }
    __dpptryend(testseh);

    another_seh_test();

    DbgPrint("%s\n", "Disable/Enable Interrupts!");
    _disable();
    _enable();
    DbgPrint("%s\n", "Done with Disable/Enable Interrupts!");

    return STATUS_SUCCESS;
}

VOID DriverUnload(struct _DRIVER_OBJECT * DriverObject)
{
    (void)DriverObject;

    DbgPrint("%s\n", "Bye ring0!");
}
