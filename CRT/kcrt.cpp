/*
 * Shameless copy pasta from: https://github.com/sidyhe/dxx
 * and: https://github.com/liupengs/Mini-CRT
 * and some minor modifications.
 */

#include <cstdio>
#include <cstdlib>

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
