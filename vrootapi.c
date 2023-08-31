
#pragma GCC diagnostic ignored "-Wbuiltin-requires-header"

#define EXPORT __attribute__ ((visibility ("default")))

#define VROOT_TRAMPOLINE(NAME) EXPORT __attribute__((naked)) void NAME() {\
    asm("b _vroot_"#NAME);\
}

#define VROOT_API_DEF(RET,NAME,ARGTYPES) VROOT_TRAMPOLINE(NAME)
#define VROOTAT_API_DEF(RET,NAME,ARGTYPES) VROOT_TRAMPOLINE(NAME)
#define VROOT_API_WRAP(RET,NAME,ARGTYPES,ARGS,PATHARG) VROOT_TRAMPOLINE(NAME)
#define VROOTAT_API_WRAP(RET,NAME,ARGTYPES,ARGS,FD,PATHARG,ATFLAG) VROOT_TRAMPOLINE(NAME)

#define VROOT_INTERNAL
#include "vroot.h"


