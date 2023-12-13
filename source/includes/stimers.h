#ifndef STIMERS_H
#define STIMERS_H

#include "types.h"

typedef struct simple_timer_typ
{
    Boolean active;
    uint32 msec_delay;
    uint32 last_msec_time;
} simple_timer_typ, *simple_timer_typ_ptr;

simple_timer_typ create_simple_timer(uint32 delay);

// void delete_simple_timer(simple_timer_typ_ptr ptr);

void reset_simple_timer(simple_timer_typ_ptr ptr, Item timeio);

Boolean is_simple_timer_ready(simple_timer_typ_ptr ptr, Item timeio);

#endif // STIMERS_H