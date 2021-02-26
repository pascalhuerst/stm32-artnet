#include "fixup.h"


// Fixup for <time.h>
time_t time (time_t *__timer)
{
    int64_t t = k_uptime_get();
    if (__timer != NULL) {
        *__timer = t;
    }

    return t;
}

