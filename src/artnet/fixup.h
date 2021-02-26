#ifndef ARTNET_FIXUP_H
#define ARTNET_FIXUP_H

#include <zephyr.h>
#include <net/socket.h>

typedef uint32_t in_addr_t;

// Fixup for <time.h>
typedef int64_t time_t;
time_t time (time_t *__timer);

#endif //ARTNET_FIXUP_H