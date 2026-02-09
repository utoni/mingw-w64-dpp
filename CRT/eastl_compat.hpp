#ifndef EASTL_COMPAT
#define EASTL_COMPAT 1

#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/type_traits.h>
#include <EASTL/utility.h>

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

#ifdef DPP_KERNEL_DRIVER
eastl::string from_unicode(wchar_t * wstr, unsigned short wlen, unsigned short wmax = 0);
#endif

constexpr size_t cstr_len_limited(const char * s, size_t max) noexcept
{
    size_t i = 0;
    for (; i < max; ++i)
    {
        if (s[i] == '\0')
            return i;
    }
    return max;
}

template <typename T>
eastl::string to_string_any(const T & value);

template <>
inline eastl::string to_string_any<const char *>(const char * const & s)
{
    return s ? eastl::string{s} : eastl::string{};
}

template <size_t N>
inline eastl::string to_string_any(const char (&s)[N])
{
    const size_t len = cstr_len_limited(s, N);
    return eastl::string(s, len);
}

template <size_t N>
inline eastl::string to_string_any(const char * const (&s)[N]) = delete;

template <size_t N>
inline eastl::string to_string_any(const unsigned char (&s)[N]) = delete;

template <size_t N>
inline eastl::string to_string_any(const unsigned char * const (&s)[N]) = delete;

template <>
inline eastl::string to_string_any<eastl::string>(const eastl::string & s)
{
    return s;
}

template <>
inline eastl::string to_string_any<eastl::string_view>(const eastl::string_view & s)
{
    return eastl::string{s};
}

template <>
inline eastl::string to_string_any<char>(const char & c)
{
    return eastl::string(1, c);
}

template <>
inline eastl::string to_string_any<int>(const int & v)
{
    return ::to_string(v);
}

template <>
inline eastl::string to_string_any<unsigned>(const unsigned & v)
{
    return ::to_string(v);
}

template <>
inline eastl::string to_string_any<long>(const long & v)
{
    return ::to_string(v);
}

template <>
inline eastl::string to_string_any<unsigned long>(const unsigned long & v)
{
    return ::to_string(v);
}

template <>
inline eastl::string to_string_any<float>(const float & v)
{
    return ::to_string(v);
}

template <>
inline eastl::string to_string_any<double>(const double & v)
{
    return ::to_string(v);
}

template <typename T>
eastl::string to_string_any(const T & value)
{
    if constexpr (eastl::is_pointer_v<T>)
    {
        eastl::string out{"0x"};
        if (!value)
            return "nullptr";
        out.append(::to_string_hex((unsigned long long int)value));
        return out;
    }
    else if constexpr (eastl::is_arithmetic_v<T>)
    {
        return to_string_any(static_cast<double>(value));
    }
    else
    {
        static_assert(sizeof(T) == 0, "::to_string_any(): unsupported type");
    }
    return {};
}

inline void format_impl(eastl::string & out, eastl::string_view fmt)
{
    out.append(fmt.data(), fmt.size());
}

template <typename Arg, typename... Args>
void format_impl(eastl::string & out, eastl::string_view fmt, Arg && arg, Args &&... args)
{
    size_t pos = fmt.find("{}");
    if (pos == eastl::string_view::npos)
    {
        out.append(fmt.data(), fmt.size());
        return;
    }
    out.append(fmt.data(), pos);
    out.append(to_string_any(eastl::forward<Arg>(arg)));
    format_impl(out, fmt.substr(pos + 2), eastl::forward<Args>(args)...);
}

template <typename... Args>
eastl::string format(eastl::string_view fmt, Args &&... args)
{
    eastl::string out;
    out.reserve(fmt.size() + sizeof...(Args) * 16);
    format_impl(out, fmt, eastl::forward<Args>(args)...);
    return out;
}

#endif
