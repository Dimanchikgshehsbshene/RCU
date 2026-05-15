#pragma once
#include <switch.h>

#ifndef R_UNLESS
#define R_UNLESS(rc)        \
    do {                    \
        if (R_FAILED(rc)) { \
            return;         \
        }                   \
    } while (0)
#endif

#ifndef R_ABORT_UNLESS
#define R_ABORT_UNLESS(rc)  \
    do {                    \
        if (R_FAILED(rc)) { \
            diagAbortWithResult(rc); \
        }                   \
    } while (0)
#endif
