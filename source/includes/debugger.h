#ifndef DEBUGGER_H
#define DEBUGGER_H

// 3DO includes
#include "types.h"
#include "mem.h"

void profile_time_start(void);

void profile_time_stop(void);

uint32 get_profile_time(void);

void dump_mem_info(char *label, uint32 mem_type);

#endif // DEBUGGER_H