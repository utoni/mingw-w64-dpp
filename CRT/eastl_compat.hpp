#ifndef EASTL_COMPAT
#define EASTL_COMPAT 1

#include <EASTL/string.h>

eastl::string to_string(int value);
eastl::string to_string(long value);
eastl::string to_string(long long value);
eastl::string to_string(unsigned int value);
eastl::string to_string(unsigned long int value);
eastl::string to_string(unsigned long long int value);
eastl::string to_string(float value);
eastl::string to_string(double value);

eastl::string to_string_hex(int value, size_t fill_width = 0);
eastl::string to_string_hex(long value, size_t fill_width = 0);
eastl::string to_string_hex(long long value, size_t fill_width = 0);
eastl::string to_string_hex(unsigned int value, size_t fill_width = 0);
eastl::string to_string_hex(unsigned long int value, size_t fill_width = 0);
eastl::string to_string_hex(unsigned long long int value, size_t fill_width = 0);

#ifndef NATIVE
eastl::string from_unicode(wchar_t * wstr, unsigned short wlen, unsigned short wmax = 0);
#endif

#endif
