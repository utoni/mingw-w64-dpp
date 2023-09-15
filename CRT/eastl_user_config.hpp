#ifndef EASTL_USER_CONFIG_HPP
#define EASTL_USER_CONFIG_HPP 1

#include <string.h>

extern "C" {
static inline int snprintf_eastl_to_string_warning(char * out, unsigned long long int size)
{
    const char msg[] = "!!! DO NOT USE eastl::to_string !!!";
    const unsigned long long int msg_size = sizeof(msg);

    if (out == NULL || size < msg_size)
    {
        return sizeof(msg);
    }

    memcpy(out, msg, msg_size);
    return msg_size;
}

static inline int Vsnprintf8(char * out, unsigned long long int size, const char *, char *)
{
    return snprintf_eastl_to_string_warning(out, size);
}

static inline int Vsnprintf16(char * out, unsigned long long int size, const char *, char *)
{
    return snprintf_eastl_to_string_warning(out, size);;
}

static inline int Vsnprintf32(char * out, unsigned long long int size, const char *, char *)
{
    return snprintf_eastl_to_string_warning(out, size);
}
};

#endif
