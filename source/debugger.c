#include "debugger.h"
#include "timerutils.h"
#include "stdio.h"

static Item ioreq = -1;
static uint32 profile_time = 0;
static uint32 profile_start_time = 0;

void profile_time_start(void)
{
    static Boolean first_run = TRUE;

    if (first_run)
    {
        ioreq = GetTimerIOReq();
        first_run = FALSE;
    }

    if (ioreq > -1)
        profile_start_time = GetMSecTime(ioreq);
}

void profile_time_stop(void)
{
    if (ioreq > -1)
        profile_time = GetMSecTime(ioreq) - profile_start_time;
}

uint32 get_profile_time(void) 
{ 
    return(profile_time); 
}

void dump_mem_info(char *label, uint32 mem_type)
{
    MemInfo minfo;

    AvailMem(&minfo, mem_type);

    printf("-----------------------------------------------\n");
    printf("%s\n", label);

    printf("minfo_SysFree %d\n", minfo.minfo_SysFree);         // Bytes free in system memory (free pages)
	printf("minfo_SysLargest %d\n", minfo.minfo_SysLargest);   // Largest span of free system memory pages
    printf("-----------------------------------------------\n");
}