#include "eastl_compat.hpp"

#define NANOPRINTF_VISIBILITY_STATIC 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#define NANOPRINTF_IMPLEMENTATION 1
#include "nanoprintf.h"

#ifdef DPP_KERNEL_DRIVER
#include <wdm.h>
#endif

/*
 * eastl::to_string(...) does not work yet event if with a provided Vsnprintf/Vsnprintf8
 * The issue seems to be caused by a broken va_list/va_copy.
 */

eastl::string to_string(int value)
{
    int nbytes = npf_snprintf(nullptr, 0, "%d", value);
    if (nbytes > 0)
    {
        char result[nbytes + 1] = {};
        npf_snprintf(result, nbytes, "%d", value);
        return result;
    }
    else
        return "";
}

eastl::string to_string(long value)
{
    int nbytes = npf_snprintf(nullptr, 0, "%ld", value);
    if (nbytes > 0)
    {
        char result[nbytes + 1] = {};
        npf_snprintf(result, nbytes, "%ld", value);
        return result;
    }
    else
        return "";
}

eastl::string to_string(long long value)
{
    int nbytes = npf_snprintf(nullptr, 0, "%lld", value);
    if (nbytes > 0)
    {
        char result[nbytes + 1] = {};
        npf_snprintf(result, nbytes, "%lld", value);
        return result;
    }
    else
        return "";
}

eastl::string to_string(unsigned int value)
{
    int nbytes = npf_snprintf(nullptr, 0, "%u", value);
    if (nbytes > 0)
    {
        char result[nbytes + 1] = {};
        npf_snprintf(result, nbytes, "%u", value);
        return result;
    }
    else
        return "";
}

eastl::string to_string(unsigned long int value)
{
    int nbytes = npf_snprintf(nullptr, 0, "%lu", value);
    if (nbytes > 0)
    {
        char result[nbytes + 1] = {};
        npf_snprintf(result, nbytes, "%lu", value);
        return result;
    }
    else
        return "";
}

eastl::string to_string(unsigned long long int value)
{
    int nbytes = npf_snprintf(nullptr, 0, "%llu", value);
    if (nbytes > 0)
    {
        char result[nbytes + 1] = {};
        npf_snprintf(result, nbytes, "%llu", value);
        return result;
    }
    else
        return "";
}

eastl::string to_string(float value)
{
    int nbytes = npf_snprintf(nullptr, 0, "%f", value);
    if (nbytes > 0)
    {
        char result[nbytes + 1] = {};
        npf_snprintf(result, nbytes, "%f", value);
        return result;
    }
    else
        return "";
}

eastl::string to_string(double value)
{
    int nbytes = npf_snprintf(nullptr, 0, "%lf", value);
    if (nbytes > 0)
    {
        char result[nbytes + 1] = {};
        npf_snprintf(result, nbytes, "%lf", value);
        return result;
    }
    else
        return "";
}

template <typename T>
static eastl::string to_string_hex(T w, size_t hex_len = sizeof(T) << 1)
{
    static const char * const digits = "0123456789ABCDEF";
    const size_t len = sizeof(T) << 1;

    if (hex_len < len)
        hex_len = len;
    eastl::string rc(hex_len, '0');
    for (size_t i = hex_len - len, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
        rc[i] = digits[(w >> j) & 0x0f];
    return rc;
}

eastl::string to_string_hex(int value, size_t fill_width)
{
    return to_string_hex<int>(value, fill_width);
}

eastl::string to_string_hex(long value, size_t fill_width)
{
    return to_string_hex<long>(value, fill_width);
}

eastl::string to_string_hex(long long value, size_t fill_width)
{
    return to_string_hex<long long>(value, fill_width);
}

eastl::string to_string_hex(unsigned int value, size_t fill_width)
{
    return to_string_hex<unsigned int>(value, fill_width);
}

eastl::string to_string_hex(unsigned long int value, size_t fill_width)
{
    return to_string_hex<unsigned long int>(value, fill_width);
}

eastl::string to_string_hex(unsigned long long int value, size_t fill_width)
{
    return to_string_hex<unsigned long long int>(value, fill_width);
}

#ifdef DPP_KERNEL_DRIVER
eastl::string from_unicode(wchar_t * wstr, unsigned short wlen, unsigned short wmax)
{
    ANSI_STRING ansi;
    UNICODE_STRING unicode;

    unicode.Buffer = wstr;
    unicode.Length = wlen;
    unicode.MaximumLength = (wmax > 0 ? wmax : wlen);

    if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&ansi, &unicode, TRUE)))
    {
        eastl::string result(ansi.Buffer, ansi.Length);
        RtlFreeAnsiString(&ansi);
        return result;
    }

    return "";
}
#endif
