/*
 * Shameless copy pasta from: https://github.com/sidyhe/dxx
 * and: https://github.com/liupengs/Mini-CRT
 * and some minor modifications.
 */

#include <ntddk.h>

#define KCRT_POOL_DEFAULT_TAG 0xDEADBEEF

extern void (*__CTOR_LIST__)();
extern void (*__DTOR_LIST__)();
extern NTSTATUS __cdecl DriverEntry(_In_ struct _DRIVER_OBJECT * DriverObject, _In_ PUNICODE_STRING RegistryPath);
extern void __cdecl DriverUnload(_In_ struct _DRIVER_OBJECT * DriverObject);

DRIVER_INITIALIZE __cdecl _CRT_DriverEntry;
DRIVER_UNLOAD __cdecl _CRT_DriverUnload;

typedef void (*__cdecl init_and_deinit_fn)(void);
typedef void (*__cdecl atexit_func_t)(void);

typedef struct _func_node
{
    atexit_func_t func;
    struct _func_node * next;
} func_node;

typedef struct _MALLOC_HEADER
{
    ULONG32 Tags;
    ULONG32 _Resv0;
    ULONG_PTR Size;
} MALLOC_HEADER, *PMALLOC_HEADER;
C_ASSERT(sizeof(MALLOC_HEADER) % sizeof(void *) == 0);

static func_node * atexit_list = NULL;

// dynamic memory mgmt

PMALLOC_HEADER GET_MALLOC_HEADER(PVOID ptr)
{
    return (MALLOC_HEADER *)((PUCHAR)ptr - sizeof(MALLOC_HEADER));
}

PVOID GET_MALLOC_ADDRESS(PMALLOC_HEADER header)
{
    return (PVOID)((PUCHAR)header + sizeof(MALLOC_HEADER));
}

ULONG_PTR GET_MALLOC_SIZE(PVOID ptr)
{
    PMALLOC_HEADER header = GET_MALLOC_HEADER(ptr);

    if (header->Tags != KCRT_POOL_DEFAULT_TAG)
        KeBugCheckEx(BAD_POOL_HEADER, 0, 0, 0, 0);

    return header->Size;
}

// c runtime

static int register_atexit(atexit_func_t func)
{
    func_node * node;
    if (!func)
        return -1;

    node = (func_node *)malloc(sizeof(func_node));

    if (node == 0)
        return -1;

    node->func = func;
    node->next = atexit_list;
    atexit_list = node;
    return 0;
}

int __cdecl atexit(atexit_func_t func)
{
    return register_atexit(func);
}

void __cdecl free(void * ptr)
{
    if (ptr)
    {
        MALLOC_HEADER * mhdr = GET_MALLOC_HEADER(ptr);

        if (mhdr->Tags != KCRT_POOL_DEFAULT_TAG)
            KeBugCheckEx(BAD_POOL_HEADER, 0, 0, 0, 0);

        ExFreePool(mhdr);
    }
}

void * __cdecl malloc(size_t size)
{
    PMALLOC_HEADER mhdr = NULL;
    const size_t new_size = size + sizeof(MALLOC_HEADER);

    mhdr = (PMALLOC_HEADER)ExAllocatePoolWithTag(NonPagedPool, new_size, KCRT_POOL_DEFAULT_TAG);
    if (mhdr)
    {
        RtlZeroMemory(mhdr, new_size);

        mhdr->Tags = KCRT_POOL_DEFAULT_TAG;
        mhdr->Size = size;
        return GET_MALLOC_ADDRESS(mhdr);
    }

    return NULL;
}

void * __cdecl realloc(void * ptr, size_t new_size)
{
    if (!ptr)
    {
        return malloc(new_size);
    }
    else if (new_size == 0)
    {
        free(ptr);
        return NULL;
    }
    else
    {
        size_t old_size = GET_MALLOC_SIZE(ptr);

        if (new_size <= old_size)
        {
            return ptr;
        }
        else
        {
            void * new_ptr = malloc(new_size);

            if (new_ptr)
            {
                memcpy(new_ptr, ptr, old_size);
                free(ptr);
                return new_ptr;
            }
        }
    }

    return NULL;
}

void __cdecl __cxa_pure_virtual(void)
{
    // definitly not perfect, but we get at least a notification
    while (1)
    {
        DbgPrint("Pure virtual function call..\n");
        LARGE_INTEGER li = {.QuadPart = -10000000};
        KeDelayExecutionThread(KernelMode, TRUE, &li);
    }
}

// stolen from musl: https://elixir.bootlin.com/musl/v1.1.9/source/src/math/ceilf.c
#define FORCE_EVAL(x)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (sizeof(x) == sizeof(float))                                                                                \
        {                                                                                                              \
            volatile float __x __attribute__((unused));                                                                \
            __x = (x);                                                                                                 \
        }                                                                                                              \
        else if (sizeof(x) == sizeof(double))                                                                          \
        {                                                                                                              \
            volatile double __x __attribute__((unused));                                                               \
            __x = (x);                                                                                                 \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            volatile long double __x __attribute__((unused));                                                          \
            __x = (x);                                                                                                 \
        }                                                                                                              \
    } while (0)

float __cdecl ceilf(float x)
{
    union {
        float f;
        UINT32 i;
    } u = {x};
    int e = (int)(u.i >> 23 & 0xff) - 0x7f;
    UINT32 m;

    if (e >= 23)
        return x;
    if (e >= 0)
    {
        m = 0x007fffff >> e;
        if ((u.i & m) == 0)
            return x;
        FORCE_EVAL(x + 0x1p120f);
        if (u.i >> 31 == 0)
            u.i += m;
        u.i &= ~m;
    }
    else
    {
        FORCE_EVAL(x + 0x1p120f);
        if (u.i >> 31)
            u.f = -0.0;
        else if (u.i << 1)
            u.f = 1.0;
    }
    return u.f;
}

// functions called in DRIVER_INITIALIZE and DRIVER_UNLOAD

static void __cdecl __ctors(void)
{
    unsigned long long int const * const * const l = (unsigned long long int const * const * const)&__CTOR_LIST__;
    unsigned long long int i = (unsigned long long int)*l;
    init_and_deinit_fn const * p;

    if (i == (unsigned long long int)-1)
    {
        for (i = 1; l[i] != NULL; i++)
            ;
        i--;
    }

    p = (init_and_deinit_fn *)&l[i];

    while (i--)
    {
        (**p--)();
    }
}

static void __cdecl __dtors(void)
{
    func_node * p = atexit_list;
    for (; p != NULL; p = p->next)
    {
        p->func();
        free(p);
    }
    atexit_list = NULL;
}

void __cdecl KCRT_OnDriverEntry(void)
{
    __ctors();
}

void __cdecl KCRT_OnDriverUnload(void)
{
    __dtors();
}

void __cdecl _CRT_DriverUnload(_In_ struct _DRIVER_OBJECT * DriverObject)
{
    DriverUnload(DriverObject);

    KCRT_OnDriverUnload();
}

NTSTATUS __cdecl _CRT_DriverEntry(_In_ struct _DRIVER_OBJECT * DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    KCRT_OnDriverEntry();

    /* support for service stopping and CRT de-init */
    DriverObject->DriverUnload = _CRT_DriverUnload;

    return DriverEntry(DriverObject, RegistryPath);
}
