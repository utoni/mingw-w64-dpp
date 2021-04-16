/*
 * Shameless copy pasta from: https://github.com/sidyhe/dxx
 */

#include <cstdio>
#include <cstdlib>

#include <ntddk.h>

#define KCRT_POOL_DEFAULT_TAG 0xDEADBEEF

typedef struct _MALLOC_HEADER
{
    ULONG32 Tags;
    ULONG32 _Resv0;
    ULONG_PTR Size;
} MALLOC_HEADER, *PMALLOC_HEADER;
C_ASSERT(sizeof(MALLOC_HEADER) % sizeof(void *) == 0);

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

// new & delete

void * __cdecl operator new(std::size_t size)
{
    return malloc(size);
}

void * __cdecl operator new[](size_t size)
{
    return malloc(size);
}

void __cdecl operator delete(void * ptr)
{
    free(ptr);
}

void __cdecl operator delete(void * ptr, size_t)
{
    free(ptr);
}

void __cdecl operator delete[](void * ptr, long long unsigned int)
{
    free(ptr);
}

void __cdecl operator delete[](void * ptr)
{
    free(ptr);
}

// EASTL

void * operator new[](size_t size, const char *, int, unsigned, const char *, int)
{
    return malloc(size);
}
void * operator new[](size_t size, size_t, size_t, const char *, int, unsigned, const char *, int)
{
    return malloc(size);
}

extern "C" void __cxa_pure_virtual(void)
{
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

extern "C" float ceilf(float x)
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
