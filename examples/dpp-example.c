#include <ntddk.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

NTSTATUS DriverEntry(struct _DRIVER_OBJECT * DriverObject, PUNICODE_STRING RegistryPath)
{
    (void)DriverObject;
    (void)RegistryPath;

    DbgPrint("%s\n", "Hello ring0!");

    // This is bad. Please do not call _disable/_enable in the DriverEntry.
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
