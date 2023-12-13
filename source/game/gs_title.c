#include "game_globals.h"
#include "gs_options.h"

#define MIN_LOGO_SCALE_X 1048576
#define MIN_LOGO_SCALE_Y 65536

#define MAX_LOGO_SCALE_X 1835008
#define MAX_LOGO_SCALE_Y 114688

#define MAX_COLOR_CYCLES 4

typedef enum sel_typ
{
    SEL_PLAY = 0,
    SEL_OPTIONS,
    SEL_MAX
} sel_typ;

typedef enum title_phase_typ
{
    PHASE_ZOOM = 0,
    PHASE_READY
} title_phase_typ;

// Cel Indexs into title cel collection
enum 
{
    CI_LOGO = 0,
    CI_CREDIT,
    CI_PLAY,
    CI_OPT,
    CI_MAX
};

static skewable_cel_typ skewable_cels[SEL_MAX];
static sel_typ sel_index;
static CCB *title_cel_list;
static int32 icon_ypos[SEL_MAX] = {-2200, -9000};
static uint32 player_rot_amount;
static uint32 color_index;
static uint16 cycle_colors[MAX_COLOR_CYCLES] = {24576, 24, 25344, 25368};
static title_phase_typ phase;
static Boolean grow;
static CCB *logo_cel;

static void update_logo(void)
{
    int32 scaled_width;

    if (phase == PHASE_ZOOM)
    {
        // Come towards player

        logo_cel->ccb_HDX += 20971;
        logo_cel->ccb_VDY += 1310;

        if (logo_cel->ccb_HDX > MAX_LOGO_SCALE_X)
        {
            logo_cel->ccb_HDX = MAX_LOGO_SCALE_X;
            logo_cel->ccb_VDY = MAX_LOGO_SCALE_Y;
            phase = PHASE_READY;
            grow = FALSE;
        }
    }
    else 
    {
        // Pulse

        if (grow)
        {
            logo_cel->ccb_HDX += 20971;
            logo_cel->ccb_VDY += 1310;

            if (logo_cel->ccb_HDX > MAX_LOGO_SCALE_X)
            {
                logo_cel->ccb_HDX = MAX_LOGO_SCALE_X;
                logo_cel->ccb_VDY = MAX_LOGO_SCALE_Y;
                grow = FALSE;
            }
        }
        else 
        {
            logo_cel->ccb_HDX -= 20971;
            logo_cel->ccb_VDY -= 1310;

            if (logo_cel->ccb_HDX < MIN_LOGO_SCALE_X)
            {
                logo_cel->ccb_HDX = MIN_LOGO_SCALE_X;
                logo_cel->ccb_VDY = MIN_LOGO_SCALE_Y;
                grow = TRUE;
            }
        }
    }

    scaled_width = MulSF16(logo_cel->ccb_Width << FRACBITS_16, logo_cel->ccb_HDX >> 4) >> FRACBITS_16;

    logo_cel->ccb_XPos = (160 - scaled_width / 2) << FRACBITS_16;   
}

void title_start(void)
{
    uint32 i;
    CCB *ccb;
    rez_envelope_typ rez_envelope;

    load_resource("Assets/Graphics/UI/THUD.cel", REZ_CEL_LIST, &rez_envelope);

    // This holds all title cels
    title_cel_list = (CCB*) rez_envelope.data;

    // For transparency
    for (i = 0; i < CI_MAX; i++)
    {
        ccb = seek_cel_list(title_cel_list, i);
        ccb->ccb_Flags &= ~CCB_BGND;
    }

    // Play and Option cels
    ccb = seek_cel_list(title_cel_list, CI_PLAY);
    skewable_cels[SEL_PLAY] = get_skewable_cel(ccb);
    set_cel_position(skewable_cels[SEL_PLAY].ccb, 116, 120);

    ccb = seek_cel_list(title_cel_list, CI_OPT);
    skewable_cels[SEL_OPTIONS] = get_skewable_cel(ccb);
    set_cel_position(skewable_cels[SEL_OPTIONS].ccb, 116, 146);
    
    // Logo
    ccb = seek_cel_list(title_cel_list, CI_LOGO);
    ccb->ccb_Flags |= (CCB_MARIA);
    ccb->ccb_HDX = 157286;
    ccb->ccb_VDY = 9830;    
    set_cel_position(ccb, 0, 40);
    logo_cel = ccb;

    // Credits
    ccb = seek_cel_list(title_cel_list, CI_CREDIT);
    set_cel_position(ccb, 113, 210);

    UNLAST_CEL(stars[MAX_STARS-1].ccb);
    LINK_CEL(stars[MAX_STARS-1].ccb, seek_cel_list(title_cel_list, CI_LOGO));

    scale_obj(player.obj, 16384);
    set_obj_xyz(player.obj, -17000, 0, 1 << FRACBITS_16);

    player_rot_amount = 0;   
    color_index = 0;
    sel_index = SEL_PLAY;

    phase = PHASE_ZOOM;
}

int32 title_update(uint32 delta_time)
{   
    vec3f16 rot_angles;
    int32 ret = 1;    

    read_device_inputs();
    
    // Title screen selector 

    update_logo();

    if (phase == PHASE_READY)
    {
        if ((BUTTONS & ControlUp) && !(PREV_BUTTONS & ControlUp))
        {
            reset_skewable_cel(&skewable_cels[sel_index]);

            sel_index = (sel_typ) (sel_index - 1);    

            if (sel_index < SEL_PLAY || sel_index >= SEL_MAX) // Unsigned
                sel_index = (sel_typ) (SEL_MAX - 1);
        }
        else if ((BUTTONS & ControlDown) && !(PREV_BUTTONS & ControlDown))
        {
            reset_skewable_cel(&skewable_cels[sel_index]);

            sel_index = (sel_typ) (sel_index + 1);

            if (sel_index >= SEL_MAX)
                sel_index = (sel_typ) 0;
        }          

        #if 1
        rot_angles[X] = 0;
        rot_angles[Y] = rot_angles[Z] = -131072;
        rotate_obj(player.obj, rot_angles);
        player_rot_amount += 131072;
        player.obj->world_x = -17000;
        #endif

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

        player.obj->world_y = icon_ypos[sel_index];

        skew_cel(&skewable_cels[sel_index]);
    }

    SetVRAMPages(sport_io, sc->sc_Bitmaps[sc->sc_CurrentScreen]->bm_Buffer, 0, sc->sc_NumBitmapPages, ~0);

    if (phase == PHASE_READY)
    {
        update_stars();

        UNLAST_CEL(logo_cel);

        begin_3d();
        add_obj(player.obj, TRUE);
        end_3d();

        raster_scene_wireframe(cycle_colors[color_index], stars[0].ccb);
        
        if (++color_index >= MAX_COLOR_CYCLES)
            color_index = 0;
    }
    else 
    {
        LAST_CEL(logo_cel);
        DrawCels(sc->sc_BitmapItems[sc->sc_CurrentScreen], logo_cel);        
    }    

    DisplayScreen(SCONTEXT_SITEM, 0);
    sc->sc_CurrentScreen = 1 - sc->sc_CurrentScreen;

    WaitVBL(vbl_io, 1);

    if (phase == PHASE_READY)
    {
        if (BUTTONS)
        {
            if (((BUTTONS & ControlStart) && !(PREV_BUTTONS & ControlStart)) ||               
                ((BUTTONS & ControlA) && !(PREV_BUTTONS & ControlA)) ||
                ((BUTTONS & ControlB) && !(PREV_BUTTONS & ControlB)) ||
                ((BUTTONS & ControlC) && !(PREV_BUTTONS & ControlC)))
            {
                if (sel_index == SEL_PLAY)
                {
                    ret = 0;
                    mouse_sens = mouse_sens_ops[mouse_sens_index];
                    // printf("use mouse sens %d at index %d\n", mouse_sens, mouse_sens_index);
                }
                else 
                {
                    run_gstate_loop(options_start, options_update, options_stop);    
                    // Reset player rotation
                    copy_vertex_def(&player.obj->vertex_def, &player.obj->vertex_def_copy);
                    scale_obj(player.obj, 16384);
                    player_rot_amount = 0;
                }
            }
        }
    }

    return(ret);
}

void title_stop(void)
{
    rez_envelope_typ rez_envelope;
    rez_envelope.data = (void*) title_cel_list;
    unload_resource(&rez_envelope, REZ_CEL_LIST);
}