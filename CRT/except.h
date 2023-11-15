#ifndef EXCEPT_H
#define EXCEPT_H 1

#ifndef __SEH__
#error "SEH not supported by your toolchain!"
#endif

#ifdef __try1
#undef __try1
#endif

#ifdef __except1
#undef __except1
#endif

#define __dpptry(handler, counter)                                                                                     \
    __asm__ goto(                                                                                                      \
        ".seh_handler __C_specific_handler, @except\n\t"                                                               \
        ".seh_handlerdata\n\t"                                                                                         \
        ".long 1\n\t"                                                                                                  \
        ".rva .l_startw" #counter ", .l_endw" #counter ", " #handler ", .l_exceptw" #counter                           \
        "\n\t"                                                                                                         \
        ".section .text\n"                                                                                             \
        ".l_startw" #counter ":" :: ::except);

#define __dppexcept(counter)                                                                                           \
    goto end;                                                                                                          \
    except:                                                                                                            \
    __asm__(".l_exceptw" #counter ":");

#define __dpptryend(counter)                                                                                           \
    end:                                                                                                               \
    __asm__(".l_endw" #counter ":");

#endif
