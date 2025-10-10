#ifndef BUILD_USERMODE
#include <ntddk.h>
#endif

#include <cstdint>
#include <stdexcept>

#include <eastl_compat.hpp>
#include <EASTL/functional.h>
#include <EASTL/hash_map.h>
#include <EASTL/random.h>
#include <EASTL/scoped_ptr.h>
#include <EASTL/set.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/sort.h>
#include <EASTL/string.h>
#include <EASTL/map.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/vector.h>

#ifdef BUILD_USERMODE
#include <cstdio>

#define DbgPrint printf
typedef struct
{
} DRIVER_OBJECT;
typedef DRIVER_OBJECT * PDRIVER_OBJECT;
typedef struct
{
} UNICODE_STRING;
typedef UNICODE_STRING * PUNICODE_STRING;
typedef int NTSTATUS;
#else
extern "C" NTSTATUS NTAPI ZwQuerySystemInformation(_In_ int SystemInformationClass,
                                                   _Inout_ PVOID SystemInformation,
                                                   _In_ ULONG SystemInformationLength,
                                                   _Out_opt_ PULONG ReturnLength);
extern "C" NTSTATUS NTAPI WrapperZwQuerySystemInformation(_In_ int SystemInformationClass,
                                                          _Inout_ PVOID SystemInformation,
                                                          _In_ ULONG SystemInformationLength,
                                                          _Out_opt_ PULONG ReturnLength);
#endif

struct GeneratorUint32
{
    uint32_t mValue;
    GeneratorUint32() : mValue(0)
    {
    }
    uint32_t operator()()
    {
        return mValue++;
    }
};

using namespace eastl;

// C&P from: https://raw.githubusercontent.com/sidyhe/dxx/ed06aba3b91fe8e101d08c33c26ba73db96acef0/README.md
void stl_test()
{
    make_unique<DRIVER_OBJECT>();
    make_shared<UNICODE_STRING>();
    scoped_ptr<double> dptr(new double(3.6));

    set<int> set_test;
    set_test.insert(1);
    set_test.insert(3);
    set_test.insert(5);
    set_test.erase(1);

    map<int, int> map_test;
    map_test[0] = 1;
    map_test[10] = 11;
    map_test[20] = 12;
    map_test.erase(11);

    vector<int> vec_test;
    vec_test.push_back(2);
    vec_test.push_back(3);
    vec_test.push_back(1);
    stable_sort(vec_test.begin(), vec_test.end(), less<int>());
    for (auto e : vec_test)
    {
        DbgPrint("%d\n", e);
    }

    string s;
    s = "This a string";
    s.append(" ");
    s.append("any");
    DbgPrint("%s\n", s.c_str());

    wstring ws;
    ws = L"wide string";
    ws.clear();

    unordered_set<float> us_test;
    us_test.insert(333);

    unordered_map<double, string> um_test;
    um_test.insert(make_pair(6.6, "9.9"));
}

void more_stl_test()
{
#ifndef BUILD_USERMODE
    hash_map<int, string> hm;

    hm[0] = "test1";
    hm[10] = "test2";
    hm[20] = "test3";
    for (auto s : hm)
    {
        DbgPrint("%s\n", s.second.c_str());
    }
#endif

    uniform_int_distribution<std::uint32_t> uid(1, UINT32_MAX);
    GeneratorUint32 g;
    DbgPrint("PRNG: %u\n", uid(g));

    auto lambda = [] { DbgPrint("Hello lambda!\n"); };
    function<void(void)> fn = lambda;
    fn();

    auto lambda2 = [](int n)
    {
        DbgPrint("Hello lambda2, %u!\n", n);
        return n;
    };
    function<int(int)> fn2 = lambda2;
    fn2(1337);

    vector<std::uint32_t> fill_me;
    for (auto i = UINT16_MAX; i > 0; --i)
    {
        fill_me.push_back(i);
    }
    DbgPrint("fill_me size: %zu\n", fill_me.size());
}

void fmt_test()
{
    eastl::string name = "Toni";
    int count = 5;
    double temp = 21.37;
    auto msg1 = ::format("Format: Hello {} ('Toni'), you have {} (5) new messages ({}C (21.37))", name, count, temp);

    char buf1[] = "test1";
    char const buf2[] = "test2";
    auto msg2 = ::format("Format: {} ('test1'), {} ('test2')", buf1, buf2);

    void * ptr = (void *)0xDEADC0DEDEADCAFE;
    float val = 1337.1337;
    unsigned over = (unsigned)-1;
    auto msg3 = ::format("Format: {} (0xDEADC0DEDEADCAFE), {} (1337.1337), {} (-1u)", ptr, val, over);

    DbgPrint("%s\n", msg1.c_str());
    DbgPrint("%s\n", msg2.c_str());
    DbgPrint("%s\n", msg3.c_str());
}

#ifndef BUILD_USERMODE
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
#endif

extern "C"
{
#ifndef BUILD_USERMODE
    DRIVER_INITIALIZE DriverEntry;
    DRIVER_UNLOAD DriverUnload;

    NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
    {
        (void)DriverObject;
        (void)RegistryPath;

        DbgPrint("%s\n", "Hello ring0!");

        zw_test();
        stl_test();
        more_stl_test();
        fmt_test();

        return STATUS_SUCCESS;
    }

    void DriverUnload(PDRIVER_OBJECT DriverObject)
    {
        (void)DriverObject;

        DbgPrint("%s\n", "Bye ring0!");
    }
#else
    int main()
    {
        DbgPrint("%s\n", "Hello user!");

        stl_test();
        more_stl_test();

        return 0;
    }
#endif
}
