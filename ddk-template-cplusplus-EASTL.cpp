#include <ntddk.h>

#include <cstdint>

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

// C&P from: https://raw.githubusercontent.com/sidyhe/dxx/ed06aba3b91fe8e101d08c33c26ba73db96acef0/README.md
void stl_test()
{
    eastl::make_unique<DRIVER_OBJECT>();
    eastl::make_shared<UNICODE_STRING>();
    eastl::scoped_ptr<double> dptr(new double(3.6));

    eastl::set<int> set_test;
    set_test.insert(1);
    set_test.insert(3);
    set_test.insert(5);
    set_test.erase(1);

    eastl::map<int, int> map_test;
    map_test[0] = 1;
    map_test[10] = 11;
    map_test[20] = 12;
    map_test.erase(11);

    eastl::vector<int> vec_test;
    vec_test.push_back(2);
    vec_test.push_back(3);
    vec_test.push_back(1);
    eastl::stable_sort(vec_test.begin(), vec_test.end(), eastl::less<int>());
    for (auto e : vec_test)
    {
        DbgPrint("%d\n", e);
    }

    eastl::string s;
    s = "This a string";
    s.append("any");
    DbgPrint("%s\n", s.c_str());

    eastl::wstring ws;
    ws = L"wide string";
    ws.clear();

    eastl::unordered_set<float> us_test;
    us_test.insert(333);

    eastl::unordered_map<double, eastl::string> um_test;
    um_test.insert(eastl::make_pair(6.6, "9.9"));
}

void more_stl_test()
{
    eastl::hash_map<int, eastl::string> hm;

    hm[0] = "test1";
    hm[10] = "test2";
    hm[20] = "test3";
    for (auto s : hm)
    {
        DbgPrint("%s\n", s.second.c_str());
    }

    eastl::uniform_int_distribution<std::uint32_t> uid(1, UINT32_MAX);
    DbgPrint("PRNG: %u\n", uid);

    auto lambda = [] { DbgPrint("Hello lambda!\n"); };
    eastl::function<void(void)> fn = lambda;
    fn();

    auto lambda2 = [](int n) {
        DbgPrint("Hello lambda2, %u!\n", n);
        return n;
    };
    eastl::function<int(int)> fn2 = lambda2;
    fn2(1337);

    eastl::vector<std::uint32_t> fill_me;
    for (auto i = UINT16_MAX; i > 0; --i)
    {
        fill_me.push_back(i);
    }
    DbgPrint("fill_me size: %zu\n", fill_me.size());
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

        stl_test();
        more_stl_test();

        return STATUS_SUCCESS;
    }

    VOID DriverUnload(_In_ struct _DRIVER_OBJECT * DriverObject)
    {
        (void)DriverObject;

        DbgPrint("%s\n", "Bye ring0!");
    }
}
