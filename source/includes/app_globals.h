#ifndef APP_GLOBALS_H
#define APP_GLOBALS_H

// 3DO includes
#include "types.h"
#include "displayutils.h"
#include "mem.h"
#include "event.h"

#define DEBUG_MODE 0            // Set this to zero for production builds
#define SHOW_FPS 0
#define FRACBITS_16 16          // For 16.16 fixed point shifting
#define FRACBITS_20 20          // For 12.20 fixed point shifting
#define ONE_F16 65536           // 2^16
#define MAX_MOUSE_SENS_OPS 3

// Control pad
#define BUTTONS (device_event_data.pad_data.cped_ButtonBits)
#define PREV_BUTTONS (prev_device_event_data.pad_data.cped_ButtonBits)

// Mouse
#define MOUSE_BUTTONS (device_event_data.mouse_data.med_ButtonBits)
#define PREV_MOUSE_BUTTONS (prev_device_event_data.mouse_data.med_ButtonBits)
#define MOUSE_X (device_event_data.mouse_data.med_HorizPosition)
#define MOUSE_Y (device_event_data.mouse_data.med_VertPosition)
#define MOUSE_DX (device_event_data.mouse_data.med_HorizPosition - prev_device_event_data.mouse_data.med_HorizPosition)
#define MOUSE_DY (device_event_data.mouse_data.med_VertPosition - prev_device_event_data.mouse_data.med_VertPosition)

#define try __trycatch_error = FALSE;
#define catch exjmp:if(__trycatch_error)
#define throw(msg, err) {__trycatch_error=TRUE;__trycatch_code=err;strcpy(__trycatch_msg, msg);goto exjmp;}
#define SCONTEXT_SITEM sc->sc_ScreenItems[sc->sc_CurrentScreen]

typedef struct device_event_data_typ
{
    ControlPadEventData pad_data;
    MouseEventData mouse_data;
} device_event_data_typ;

extern ScreenContext *sc;
extern uint32 display_type;
extern uint32 display_width;
extern uint32 display_height;
extern uint32 display_width2;
extern uint32 display_height2;
extern uint32 display_width2_f16;
extern uint32 display_height2_f16;
extern Item vbl_io;
extern Item sport_io;
extern Item time_io;
extern uint32 fields_sec;
extern device_event_data_typ device_event_data;
extern device_event_data_typ prev_device_event_data;
extern int32 mouse_sens;
extern int32 mouse_sens_ops[MAX_MOUSE_SENS_OPS];
extern uint32 mouse_sens_index;
extern Boolean __trycatch_error;
extern int32 __trycatch_code;
extern char __trycatch_msg[50];

// app.c
extern void init_app(void);
extern void read_device_inputs(void);
extern void run_gstate_loop(void (*begin)(void), int32 (*tick)(uint32 delta_time), void (*end)(void));

#endif // APP_GLOBALS_H