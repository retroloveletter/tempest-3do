#include "stimers.h"
#include "timerutils.h"
#include "mem.h"

simple_timer_typ create_simple_timer(uint32 delay)
{
    // simple_timer_typ_ptr ptr = (simple_timer_typ_ptr) AllocMem(sizeof(simple_timer_typ), MEMTYPE_DRAM);
    simple_timer_typ stimer;
    stimer.msec_delay = delay;
    // printf("creating time with delay %d\n", delay);
    stimer.active = FALSE;
    return(stimer);
}

void reset_simple_timer(simple_timer_typ_ptr ptr, Item timeio)
{
    ptr->last_msec_time = GetMSecTime(timeio);
    ptr->active = TRUE;
}

Boolean is_simple_timer_ready(simple_timer_typ_ptr ptr, Item timeio)
{
    uint32 current_time;

    if (!ptr->active)
        return(FALSE);

    current_time = GetMSecTime(timeio);

    if (current_time - ptr->last_msec_time >= ptr->msec_delay)
    {
        ptr->active = FALSE;
        return(TRUE);
    }

    return(FALSE);
}

/*
void delete_simple_timer(simple_timer_typ_ptr ptr)
{
    if (ptr)
        FreeMem(ptr, sizeof(simple_timer_typ));
}
*/