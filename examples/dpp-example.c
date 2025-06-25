#include <ntddk.h>

#include <except.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

extern NTSTATUS NTAPI ZwProtectVirtualMemory(_In_ HANDLE ProcessHandle,
                                             _In_ _Out_ PVOID * BaseAddress,
                                             _In_ _Out_ PULONG NumberOfBytesToProtect,
                                             _In_ ULONG NewAccessProtection,
                                             _Out_ PULONG OldAccessProtection);
extern NTSTATUS NTAPI ZwQuerySystemInformation(_In_ int SystemInformationClass,
                                               _Inout_ PVOID SystemInformation,
                                               _In_ ULONG SystemInformationLength,
                                               _Out_opt_ PULONG ReturnLength);
extern NTSTATUS NTAPI WrapperZwQuerySystemInformation(_In_ int SystemInformationClass,
                                                      _Inout_ PVOID SystemInformation,
                                                      _In_ ULONG SystemInformationLength,
                                                      _Out_opt_ PULONG ReturnLength);
extern NTSTATUS NTAPI WrapperZwCreateFile(_Out_ PHANDLE FileHandle,
                                          _In_ ACCESS_MASK DesiredAccess,
                                          _In_ POBJECT_ATTRIBUTES ObjectAttributes,
                                          _Out_ PIO_STATUS_BLOCK StatusBlock,
                                          _In_ PLARGE_INTEGER AllocationSize,
                                          _In_ ULONG FileAttributes,
                                          _In_ ULONG ShareAccess,
                                          _In_ ULONG CreateDisposition,
                                          _In_ ULONG CreateOptions,
                                          _In_ PVOID EaBuffer,
                                          _In_ ULONG EaLength);
extern NTSTATUS NTAPI WrapperZwClose(_In_ HANDLE Handle);
extern NTSTATUS NTAPI WrapperZwWriteFile(_In_ HANDLE FileHandle,
                                         _In_ HANDLE Event,
                                         _In_ PIO_APC_ROUTINE ApcRoutine,
                                         _In_ PVOID ApcContext,
                                         _Out_ PIO_STATUS_BLOCK StatusBlock,
                                         _In_ PVOID Buffer,
                                         _In_ ULONG Length,
                                         _In_ PLARGE_INTEGER ByteOffset,
                                         _In_ PULONG Key);

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

static void zw_test()
{
    NTSTATUS ret;
    ULONG memoryNeeded = 0;

    ret = ZwQuerySystemInformation(0x5, NULL, 0, &memoryNeeded);
    if (ret != STATUS_INFO_LENGTH_MISMATCH || !memoryNeeded)
    {
        DbgPrint("ZwQuerySystemInformation failed with 0x%lX (memory needed: %lu)\n", ret, memoryNeeded);
    }

    memoryNeeded = 0;
    ret = WrapperZwQuerySystemInformation(0x5, NULL, 0, &memoryNeeded);
    if (ret != STATUS_INFO_LENGTH_MISMATCH || !memoryNeeded)
    {
        DbgPrint("ZwQuerySystemInformation failed 0x%lX (memory needed: %lu)\n", ret, memoryNeeded);
    }
}

static NTSTATUS WriteToFile()
{
    UNICODE_STRING fileName = RTL_CONSTANT_STRING(L"\\??\\C:\\dpp-example-text.log");
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE fileHandle;
    NTSTATUS status;

    InitializeObjectAttributes(&objAttr, &fileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = WrapperZwCreateFile(&fileHandle,
                                 GENERIC_WRITE,
                                 &objAttr,
                                 &ioStatusBlock,
                                 NULL,
                                 FILE_ATTRIBUTE_NORMAL,
                                 0,
                                 FILE_OVERWRITE_IF,
                                 FILE_SYNCHRONOUS_IO_NONALERT,
                                 NULL,
                                 0);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    CHAR data[] = "Test data from the kernel driver\n";
    status = WrapperZwWriteFile(fileHandle, NULL, NULL, NULL, &ioStatusBlock, data, sizeof(data) - 1, NULL, NULL);

    WrapperZwClose(fileHandle);
    return status;
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
    zw_test();

    DbgPrint("%s\n", "Disable/Enable Interrupts!");
    _disable();
    _enable();
    DbgPrint("%s\n", "Done with Disable/Enable Interrupts!");

    DbgPrint("%s\n", "WriteToFile");
    WriteToFile();

    return STATUS_SUCCESS;
}

VOID DriverUnload(struct _DRIVER_OBJECT * DriverObject)
{
    (void)DriverObject;

    DbgPrint("%s\n", "Bye ring0!");
}
