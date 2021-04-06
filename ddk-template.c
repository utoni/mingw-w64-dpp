#include <ntddk.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

NTSTATUS DriverEntry(
	_In_  struct _DRIVER_OBJECT *DriverObject,
	_In_  PUNICODE_STRING RegistryPath
)
{
    DbgPrint("%s\n", "Hello ring0!");

    /* support for service stopping */
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}

VOID
DriverUnload(
    _In_ struct _DRIVER_OBJECT  *DriverObject
)
{
    DbgPrint("%s\n", "Bye ring0!");
}
