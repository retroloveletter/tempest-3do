#include "game_globals.h"

#define MAX_COLOR_CYCLES 4

typedef enum sel_typ
{
    SEL_MUSIC = 0,
    SEL_MODE,
    SEL_MOUSE,
    SEL_EXIT,
    SEL_MAX
} sel_typ;

enum 
{
    CI_ON = 0,
    CI_OFF,
    CI_EXIT,
    CI_MUSIC,
    CI_MODE,
    CI_NORMAL,
    CI_HARD,
    CI_LOW,
    CI_MED,
    CI_HIGH,
    CI_MOUSE,
    CI_MAX
};

static sel_typ sel_index;
static skewable_cel_typ skewable_cels[SEL_MAX];
static uint16 cycle_colors[MAX_COLOR_CYCLES] = {24576, 24, 25344, 25368};
static uint32 color_index;
static int32 icon_ypos[SEL_MAX] = {16000, 9000, 2000, -5800};
static uint32 player_rot_amount;
static CCB *music_toggles[2];
static CCB *mode_toggles[2];
static CCB *sens_toggles[3];
static CCB *op_cel_list;

void options_start(void)
{
    uint32 i;
    CCB *ccb;
    rez_envelope_typ rez_envelope;

    load_resource("Assets/Graphics/UI/OHUD.cel", REZ_CEL_LIST, &rez_envelope);

    // This holds all option cels
    op_cel_list = (CCB*) rez_envelope.data;

    // For transparency
    for (i = 0; i < CI_MAX; i++)
    {
        ccb = seek_cel_list(op_cel_list, i);
        ccb->ccb_Flags &= ~CCB_BGND;
    }

    ccb = seek_cel_list(op_cel_list, CI_MUSIC);
    skewable_cels[SEL_MUSIC] = get_skewable_cel(ccb);
    set_cel_position(skewable_cels[SEL_MUSIC].ccb, 70, 50);

    ccb = seek_cel_list(op_cel_list, CI_MODE);
    skewable_cels[SEL_MODE] = get_skewable_cel(ccb);
    set_cel_position(skewable_cels[SEL_MODE].ccb, 70, 76); 

    ccb = seek_cel_list(op_cel_list, CI_MOUSE);
    skewable_cels[SEL_MOUSE] = get_skewable_cel(ccb);
    set_cel_position(skewable_cels[SEL_MOUSE].ccb, 70, 102);   

    ccb = seek_cel_list(op_cel_list, CI_EXIT);
    skewable_cels[SEL_EXIT] = get_skewable_cel(ccb);
    set_cel_position(skewable_cels[SEL_EXIT].ccb, 70, 138);    

    ccb = seek_cel_list(op_cel_list, CI_OFF);
    set_cel_position(ccb, 242, 50);
    music_toggles[0] = ccb;

    ccb = seek_cel_list(op_cel_list, CI_ON);
    set_cel_position(ccb, 251, 50);
    music_toggles[1] = ccb;

    ccb = seek_cel_list(op_cel_list, CI_NORMAL);
    set_cel_position(ccb, 186, 76);
    mode_toggles[0] = ccb;

    ccb = seek_cel_list(op_cel_list, CI_HARD);
    set_cel_position(ccb, 217, 76);
    mode_toggles[1] = ccb;    

    ccb = seek_cel_list(op_cel_list, CI_LOW);
    set_cel_position(ccb, 265, 102);
    sens_toggles[0] = ccb;    

    ccb = seek_cel_list(op_cel_list, CI_MED);
    set_cel_position(ccb, 265, 102);
    sens_toggles[1] = ccb;    

    ccb = seek_cel_list(op_cel_list, CI_HIGH);
    set_cel_position(ccb, 265, 102);
    sens_toggles[2] = ccb;    

    color_index = 0;
    sel_index = SEL_MUSIC;
    player_rot_amount = 0;

    // Reset player rotation
    copy_vertex_def(&player.obj->vertex_def, &player.obj->vertex_def_copy);
    scale_obj(player.obj, 16384);

    zero_camera();

    #if 0
    dump_mem_info("VRAM", MEMTYPE_VRAM);
    dump_mem_info("DRAM", MEMTYPE_DRAM);
    #endif
}

int32 options_update(uint32 delta_time)
{       
    vec3f16 rot_angles;
    int32 ret = 1;    

    read_device_inputs();

    if ((BUTTONS & ControlUp) && !(PREV_BUTTONS & ControlUp))
    {
        reset_skewable_cel(&skewable_cels[sel_index]);

        sel_index = (sel_typ) (sel_index - 1);    

        if (sel_index < SEL_MUSIC || sel_index >= SEL_MAX) // Unsigned
            sel_index = (sel_typ) (SEL_MAX - 1);
    }
    else if ((BUTTONS & ControlDown) && !(PREV_BUTTONS & ControlDown))
    {
        reset_skewable_cel(&skewable_cels[sel_index]);

        sel_index = (sel_typ) (sel_index + 1);

        if (sel_index >= SEL_MAX)
            sel_index = (sel_typ) 0;
    }          
    else 
    {
        if (((BUTTONS & ControlStart) && !(PREV_BUTTONS & ControlStart)) ||               
            ((BUTTONS & ControlX) && !(PREV_BUTTONS & ControlX)) ||
            ((BUTTONS & ControlA) && !(PREV_BUTTONS & ControlA)) ||
            ((BUTTONS & ControlB) && !(PREV_BUTTONS & ControlB)) ||
            ((BUTTONS & ControlC) && !(PREV_BUTTONS & ControlC)) ||
            ((BUTTONS & ControlLeftShift) && !(PREV_BUTTONS & ControlLeftShift)) ||
            ((BUTTONS & ControlRightShift) && !(PREV_BUTTONS & ControlRightShift)))
        {
            if (sel_index == SEL_MUSIC)
            {
                TOGGLE_GAME_SETTING(GAME_SET_MUSIC_MASK);
            }
            else if (sel_index == SEL_MODE)
            {
                TOGGLE_GAME_SETTING(GAME_SET_HARD_MASK);
            }
            else if (sel_index == SEL_MOUSE)
            {
                ++mouse_sens_index;
                
                if (mouse_sens_index >= MAX_MOUSE_SENS_OPS)
                    mouse_sens_index = 0;
            }
        }
    }

    rot_angles[X] = 0;
    rot_angles[Y] = rot_angles[Z] = -131072;
    rotate_obj(player.obj, rot_angles);
    player_rot_amount += 131072;

    /*  We need to reset the vertices once a full rotation has completed, otherwise, 
        over time, precision errors will alter the vertices just enough to alter
        the model's appearance. */

    if (player_rot_amount >= 11796480)
    {
        // Reset player rotation
        copy_vertex_def(&player.obj->vertex_def, &player.obj->vertex_def_copy);
        scale_obj(player.obj, 16384);
        player_rot_amount = 0;
    }

    player.obj->world_x = -29000;
    player.obj->world_y = icon_ypos[sel_index];

    skew_cel(&skewable_cels[sel_index]);

    // Toggle display logic

    SKIP_CEL(music_toggles[0]);
    SKIP_CEL(music_toggles[1]);
    SKIP_CEL(mode_toggles[0]);
    SKIP_CEL(mode_toggles[1]);
    SKIP_CEL(sens_toggles[0]);
    SKIP_CEL(sens_toggles[1]);
    SKIP_CEL(sens_toggles[2]);

    if (game_settings & GAME_SET_MUSIC_MASK)
        UNSKIP_CEL(music_toggles[1]);
    else 
        UNSKIP_CEL(music_toggles[0]);

    if (game_settings & GAME_SET_HARD_MASK)
        UNSKIP_CEL(mode_toggles[1]);
    else 
        UNSKIP_CEL(mode_toggles[0]);

    UNSKIP_CEL(sens_toggles[mouse_sens_index]);

    begin_3d();
    add_obj(player.obj, TRUE);
    end_3d();

    SetVRAMPages(sport_io, sc->sc_Bitmaps[sc->sc_CurrentScreen]->bm_Buffer, 0, sc->sc_NumBitmapPages, ~0);

    raster_scene_wireframe(cycle_colors[color_index], op_cel_list);

    DisplayScreen(SCONTEXT_SITEM, 0);
    sc->sc_CurrentScreen = 1 - sc->sc_CurrentScreen;

    if (++color_index >= MAX_COLOR_CYCLES)
        color_index = 0;

    if (sel_index == SEL_EXIT)
    {
        if (BUTTONS)
        {
            if (((BUTTONS & ControlStart) && !(PREV_BUTTONS & ControlStart)) ||               
                ((BUTTONS & ControlA) && !(PREV_BUTTONS & ControlA)) ||
                ((BUTTONS & ControlB) && !(PREV_BUTTONS & ControlB)) ||
                ((BUTTONS & ControlC) && !(PREV_BUTTONS & ControlC)))
            {
                ret = 0;
            }
        }
    }

    WaitVBL(vbl_io, 1);    
    
    return(ret);
}

void options_stop(void)
{
    rez_envelope_typ rez_envelope;
    rez_envelope.data = (void*) op_cel_list;
    unload_resource(&rez_envelope, REZ_CEL_LIST);    
}