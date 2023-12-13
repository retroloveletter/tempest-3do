#include "app_globals.h"
#include "stimers.h"

// 3DO includes
#include "stdio.h"
#include "string.h"
#include "timerutils.h"
#include "io.h"
#include "operamath.h"

// Inverse of one millisecond
#define ONE_MSEC_INV_F16 65

// Globals
ScreenContext *sc = 0;
uint32 display_type;
uint32 display_width;
uint32 display_height;
uint32 display_width2;
uint32 display_height2;
uint32 display_width2_f16;
uint32 display_height2_f16;
Item vbl_io;
Item sport_io;
Item time_io;
device_event_data_typ device_event_data;
device_event_data_typ prev_device_event_data;
int32 mouse_sens;
int32 mouse_sens_ops[MAX_MOUSE_SENS_OPS] = {16000, 38000, 58000};
uint32 mouse_sens_index;

Boolean __trycatch_error;
int32 __trycatch_code;
char __trycatch_msg[50];

#if DEBUG_MODE
    MemInfo base_vram_info;
    MemInfo base_dram_info;
#endif

uint32 fields_sec;

// Private
static int32 frame_time;
static int32 start_time;
static uint32 frame_counter;
static simple_timer_typ fields_sec_timer;

void init_app(void)
{
    /*  Since I'm using the latest 3DO toolkit structs and the hardware isn't changing, the
        byte sizes below are fine to hardcode. */
    memset((void*) &device_event_data.pad_data, 0, 4);
    memset((void*) &device_event_data.mouse_data, 0, 12);

    mouse_sens_index = 1; // Mid
    mouse_sens = mouse_sens_ops[mouse_sens_index];
    // printf("default mouse sens %d\n", mouse_sens);
}

// This may have been better placed within a thread
void read_device_inputs(void)
{
    /*  Since I'm using the latest 3DO toolkit structs and the hardware isn't changing, the
        byte sizes below are fine to hardcode. */
    memcpy((void*) &prev_device_event_data.pad_data, (void*) &device_event_data.pad_data, 4);
    memcpy((void*) &prev_device_event_data.mouse_data, (void*) &device_event_data.mouse_data, 12);

    GetControlPad(1, FALSE, &device_event_data.pad_data);
	GetMouse(1, FALSE, &device_event_data.mouse_data);
}

void run_gstate_loop(void (*begin)(void), int32 (*tick)(uint32 delta_time), void (*end)(void))
{
    static Boolean first_run = TRUE;
    int32 run_gstate;

    if (first_run)
    {
        #if DEBUG_MODE
            printf("Create fields per second timer\n");
        #endif 

        first_run = FALSE;
        fields_sec_timer = create_simple_timer(1000);
    }

    (*begin)();

    frame_time = 16;
    frame_counter = 0;
    fields_sec = 0;

    reset_simple_timer(&fields_sec_timer, time_io);

    do
    {
        // WaitVBLDefer(vbl_io, 1);

        start_time = GetMSecTime(time_io);

        // Convert to seconds in 16.16 format. Multiply by inverse of 1000 to save a division.
        frame_time = MulSF16(frame_time << FRACBITS_16, ONE_MSEC_INV_F16);

        run_gstate = (*tick)((uint32)frame_time); // Assumes this will vsync.

        // Fields per second measurement

        if (is_simple_timer_ready(&fields_sec_timer, time_io))
        {
            fields_sec = frame_counter;
            frame_counter = 0;
            reset_simple_timer(&fields_sec_timer, time_io);
            /*
            #if DEBUG_MODE 
                printf("Fields per second %d\n", fields_sec);
            #endif
            */
        }
        else 
        {
            frame_counter++;
        }       

        // WaitIO(vbl_io); This will flicker!

        frame_time = GetMSecTime(time_io) - start_time;
    } while(run_gstate);

    (*end)();
}