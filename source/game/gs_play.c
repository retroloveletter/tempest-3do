#include "game_globals.h"
#include "gs_title.h"
#include "effects.h"

#define MAX_SCORE_DIGITS 6
#define DIGIT_ATLAS_WIDTH 140
#define DIGIT_ATLAS_HEIGHT 16
#define DIGIT_WIDTH 14
#define DIGIT_HEIGHT 16

// Powerup flags
#define PUP_NONE 0
#define PUP_ZAPPER_MASK 1

/* *************************************************************************************** */
/* ===================================== GLOBAL VARS ===================================== */
/* *************************************************************************************** */

Item *sfx[SFX_MAX];
star_typ stars[MAX_STARS];
uint32 game_settings;

/* *************************************************************************************** */
/* ============================ GLOBAL FUNCTION PROTOTYPES =============================== */
/* *************************************************************************************** */

void init_stars(void);
void update_stars(void);
void reset_star(star_typ *sptr);
void showcase_ship(void);

/* *************************************************************************************** */
/* ==================================== PRIVATE VARS ===================================== */
/* *************************************************************************************** */

static bullet_typ bullets[MAX_BULLETS];
static uint32 play_handler_index;
static void (*play_handlers[PLAY_HANDLER_MAX])(uint32);
static uint32 obj_velocity; // Reusable velocity value
static FontDescriptor *font_desc;
static uint32 score;
static Point volley_adj[MAX_BULLETS];
static uint32 volley_index;
static CCB *watch_out;
static skewable_cel_typ watch_skewable;
static int32 watch_out_dir;
static CCB *gover;
static skewable_cel_typ gover_skewable;
static CCB *end_msg;
static int32 base10_lut[MAX_SCORE_DIGITS] = {100000, 10000, 1000, 100, 10, 1};
static int32 score_digit_x_lut[10] = {0, 14, 28, 42, 56, 70, 84, 98, 112, 126};
static CCB *explode;
static uint32 powerup_flags;
static CCB *zapper;
static skewable_cel_typ zapper_skewable;
static time_delta_typ next_level_time;
static time_delta_typ enemy_spawn_time;
static time_delta_typ zapper_msg_time;
static uint32 tick_start_time; // Not accurate but selectively usable.
static simple_timer_typ bullet_rate_timer;

// Points earned by defeating enemy types
static uint32 score_table[MAX_ENEMY_TYPES] = {
    55,     // Missile
    150,    // Flipper
    100,    // Tanker
    50,     // Spiker
    250,    // Fuseball
    200,    // Pulsar
    150,    // Ftanker
    150     // Ptanker
};  

// Points displayed upon defeating enemy types
static CCB *score_cels[MAX_ENEMY_TYPES];
// Display of actual score itself
static CCB *score_num_cels[MAX_SCORE_DIGITS];
static CCB *score_atlas;

struct 
{
    time_delta_typ zap_time;
    Boolean active;
    Boolean zap_state;
} zapper_effect;

#if SHOW_FPS
    static TextCel *fps_tcel;
#endif

/* *************************************************************************************** */
/* =========================== PRIVATE FUNCTION PROTOTYPES =============================== */
/* *************************************************************************************** */

static void new_game(void);
static void flip_display(void);
static void clear_entities(void);
static void draw_guides(void);
static void update_life_hud(void);
// Audio
static void load_audio(void);
// 3DO fonts
static void load_fonts(void);
// Bullets
static void load_bullets(void);
static void fire_bullet(void);
static void update_bullets(uint32 delta_time);
static void add_bullets(void);
static void clear_bullets(void);
static void use_zapper(void);
// Scores
static void load_scores(void);
static void enemy_score(enemy_typ_ptr enemy);
static void update_score_hud(void);
static void update_score(void);
static void clear_score_cels(void);
// Play handlers
static void intro_handler(uint32 delta_time);   // Next level transitions into view
static void game_handler(uint32 delta_time);    // Playing the game
static void hit_handler(uint32 delta_time);     // Player dies
static void end_handler(uint32 delta_time);     // Completed game
static void grab_handler(uint32 delta_time);    // Enemy grabbed player
static void switch_handler(uint32 delta_time);  // Finished level
// Input
static void do_game_input(uint32 delta_time);
static void do_switch_input(uint32 delta_time);
static void do_over_input(uint32 delta_time);
static void do_game_input_pad(uint32 delta_time);
static void do_game_input_mouse(uint32 delta_time);

/* *************************************************************************************** */
/* =========================== PRIVATE FUNCTION DEFINITIONS ============================== */
/* *************************************************************************************** */

void update_life_hud(void)
{
    uint32 i;

    #if DEBUG_MODE 
        printf("update_life_hud() - Player lives: %d\n", player.lives);

        if (player.lives > MAX_LIVES)
            printf("Error - Player lives exceeded maximum.\n");
    #endif

    for (i = 0; i < MAX_LIVES; i++)
    {
        if (player.lives >= i + 1)
            lives[i]->ccb_Flags &= ~CCB_SKIP;
        else 
            lives[i]->ccb_Flags |= CCB_SKIP;
    }
}

void update_score_hud(void)
{
    int32 i;
    int32 digit;
    int32 base;
    int32 temp_score = score;    
    Rect sub_rect;   

    // Update score HUD
    for (i = 0; i < MAX_SCORE_DIGITS; i++)
    {
        base = base10_lut[i];

        digit = temp_score / base;
        
        // d will hold the atlas index for updated place digit
        sub_rect.rect_XLeft = score_digit_x_lut[digit];  
        sub_rect.rect_YTop = 0;
        sub_rect.rect_XRight = sub_rect.rect_XLeft + DIGIT_WIDTH - 1; // Inclusive
        sub_rect.rect_YBottom = DIGIT_HEIGHT - 1;

        // Be sure to reset the source pointer
        score_num_cels[i]->ccb_SourcePtr = score_atlas->ccb_SourcePtr;

        set_cel_subregion_bpp8(score_num_cels[i], &sub_rect, DIGIT_ATLAS_WIDTH);

        temp_score -= digit * base;
    }
}

void new_game(void)
{
    zero_camera();

    run_gstate_loop(title_start, title_update, title_stop);

    // Always relink cels in case another state uses them

    LINK_CEL(lives[MAX_LIVES-1], watch_out);
    LINK_CEL(watch_out, explode);
    LINK_CEL(explode, score_cels[0]);
    LINK_CEL(score_cels[MAX_ENEMY_TYPES-1], gover);
    LINK_CEL(gover, score_num_cels[0]);
    LINK_CEL(score_num_cels[MAX_SCORE_DIGITS-1], stars[0].ccb);    

    #if SHOW_FPS
        // Fields per second.
        fps_tcel = CreateTextCel(font_desc, 0, 20, 20);
        UpdateTextInCel(fps_tcel, TRUE, "FPS");
        fps_tcel->tc_CCB->ccb_XPos = 12 << FRACBITS_16;
        fps_tcel->tc_CCB->ccb_YPos = 220 << FRACBITS_16;
        SetTextCelColor(fps_tcel, 0, MakeRGB15(0, 31, 0));
        LINK_CEL(stars[MAX_STARS-1].ccb, fps_tcel->tc_CCB);
        LAST_CEL(fps_tcel->tc_CCB);
    #else 
        LAST_CEL(stars[MAX_STARS-1].ccb);
    #endif           

    gover->ccb_Flags |= CCB_SKIP;
    
    #if 0
    dump_mem_info("DRAM", MEMTYPE_DRAM);
    dump_mem_info("VRAM", MEMTYPE_VRAM);
    #endif

    reset_level_manager();

    /*  Main task is placed into hold state until the first two levels are loaded by cycle_levels().
        All subsequent calls will be asynchronous as new levels are buffered one at a time. */
    cycle_levels();      

    if (game_settings & GAME_SET_MUSIC_MASK)       
        start_music();
		
    score = 0;
    player.lives = 3;
    powerup_flags = PUP_NONE;

    update_life_hud();
    update_score_hud();      

    memset((void*) &next_level_time, 0, sizeof(time_delta_typ));
    memset((void*) &enemy_spawn_time, 0, sizeof(time_delta_typ));
    memset((void*) &zapper_msg_time, 0, sizeof(time_delta_typ));

    memset((void*) &zapper_effect, 0, sizeof(zapper_effect));

    next_level_time.delay = 30000;
    enemy_spawn_time.delay = 300;
    zapper_msg_time.delay = 2800;

    zapper_effect.zap_time.delay = 200;    

    set_play_handler(PLAY_HANDLER_INTRO);
}

void reset_star(star_typ *sptr)
{
    sptr->world[X] = (-2 + (rand() % 5)) << FRACBITS_16;
    sptr->world[Y] = (-2 + (rand() % 5)) << FRACBITS_16;
    sptr->world[Z] = (6 + rand() % 4) << FRACBITS_16;
}

void update_stars(void)
{
    uint32 i;
    Point screen_pos;
    star_typ *sptr;

    for (i = 0; i < MAX_STARS; i++)
    {   
        sptr = &stars[i];

        point_to_camera(sptr->cam, sptr->world);       

        point_to_screen(&screen_pos, sptr->cam, TRUE);

        sptr->ccb->ccb_XPos = screen_pos.pt_X << FRACBITS_16;
        sptr->ccb->ccb_YPos = screen_pos.pt_Y << FRACBITS_16;

        stars[i].world[Z] -= 16384;

        if (sptr->world[Z] <= camera.world_z)
            reset_star(sptr);
    }
}

void init_stars(void)
{
    uint32 i;

    stars[0].ccb = create_coded_colored_cel8(8, 8, MakeRGB15(28, 28, 28));
    
    for (i = 0; i < MAX_STARS; i++)
    {
        if (i > 0)
        {
            stars[i].ccb = create_coded_cel8(8, 8, FALSE);
            stars[i].ccb->ccb_SourcePtr = stars[0].ccb->ccb_SourcePtr;
            stars[i].ccb->ccb_PLUTPtr = stars[0].ccb->ccb_PLUTPtr;           
            LINK_CEL(stars[i-1].ccb, stars[i].ccb);
        }

        stars[i].ccb->ccb_HDX = 209715;
        stars[i].ccb->ccb_VDY = 13107;

        reset_star(&stars[i]);
    }
}

void clear_entities(void)
{
    clear_bullets();
    clear_hostiles();
}

void flip_display(void)
{
    #if SHOW_FPS
        static uint32 last_fields_sec = 0;
        char buffer[6];

        // Only update if changed
        if (fields_sec != last_fields_sec)
        {
            sprintf(buffer, "%d", fields_sec);
		    UpdateTextInCel(fps_tcel, TRUE, buffer);
            last_fields_sec = fields_sec;  
        }
    #endif 

    #if SHOW_LEVEL_NORMALS
        draw_obj_normals(SCONTEXT_BITEM, LCONTEXT_LEVEL.obj);
    #endif

        #if 0
        {
            static GrafCon gcon;
            char mouse_string_buf[25];
            
            gcon.gc_PenX = 8;
            gcon.gc_PenY = 8;
                       
            sprintf(mouse_string_buf, "SENS %d", mouse_sens);
            DrawText8(&gcon, sc->sc_BitmapItems[sc->sc_CurrentScreen],(uint8 *) mouse_string_buf);         
        }
    #endif

    DisplayScreen(SCONTEXT_SITEM, 0);
    sc->sc_CurrentScreen = 1 - sc->sc_CurrentScreen;

    WaitVBL(vbl_io, 1);
}

void end_handler(uint32 delte_time)
{       
    update_stars();    
    showcase_ship();

    SetVRAMPages(sport_io, SCONTEXT_BITMAP, 0, sc->sc_NumBitmapPages, ~0);

    begin_3d();
    add_obj(player.obj, FALSE);
    end_3d();
    
    raster_scene_wireframe(24, NULL);
    DrawCels(sc->sc_BitmapItems[sc->sc_CurrentScreen], end_msg);
    flip_display();
}

void switch_handler(uint32 delta_time)
{
    vec3f16 angles;
    int32 step;
    uint32 i;

    do_switch_input(delta_time);
    service_sample_player();
    update_score();
    update_stars();
    update_bullets(delta_time);

    SetVRAMPages(sport_io, SCONTEXT_BITMAP, 0, sc->sc_NumBitmapPages, ~0);

    begin_3d();

    add_obj_zclip(LCONTEXT_LEVEL.obj, CAM_NEAR);

    add_bullets();

    if (camera.world_z < LCONTEXT_LEVEL.obj->world_z + (4 << FRACBITS_16))
    {
        // Move camera and play into scene
        step = MulSF16(obj_velocity, delta_time);
        camera.world_z += step;
        player.obj->world_z += step;
        obj_velocity += 9830;

        // Check for spike collision
        for (i = 0; i < MAX_SPIKES; i++)
        {
            if (spikes[i].active)
            {
                if (spikes[i].corridor_index == player.corridor_index)
                {
                    if (player.obj->world_z >= spikes[i].end_pos[2])
                    {
                        // Boom!
                        player_hit(0);                        
                        break;
                    }
                }
            }
        }
    }
    else 
    {
        // Move player into distance 
        player.obj->world_z += MulSF16(obj_velocity, delta_time);
        obj_velocity += 9830;
        
        if (player.obj->world_z > 1988535)
        {
            ++current_level;

            // Check for end game
            if (current_level > MAX_LEVELS)
            {
                set_play_handler(PLAY_HANDLER_END);
            }
            else
            {
                // Check for super zapper recharge
                if (current_level > 1)
                {
                    // If player doesn't have a super zapper recharge, give it to him.
                    if ( !(game_settings & GAME_SET_HARD_MASK) && !(powerup_flags & PUP_ZAPPER_MASK) )
                    {
                        powerup_flags |= PUP_ZAPPER_MASK;
                        zapper_msg_time.previous_time = tick_start_time;
                        UNSKIP_CEL(zapper);

                        play_sample(*sfx[SFX_VOICE], 300, 0x40D8);
                    }    
                }

                // Only allow extra life in normal mode
                if ( !(game_settings & GAME_SET_HARD_MASK) )
                {
                    if (current_level == 32 || current_level == 64)
                    {
                        ++player.lives;
                        update_life_hud();
                    }
                }

                cycle_levels();
                set_play_handler(PLAY_HANDLER_INTRO);
            }
        }
    }

    angles[0] = 0;
    angles[1] = 0;
    angles[2] = obj_velocity;
    rotate_obj(player.obj, angles);

    add_obj(player.obj, FALSE);
    
    end_3d();
    
    raster_scene(NULL, NULL, lives[0]);
    draw_spikes();
    flip_display();
}

void over_handler(uint32 delta_time)
{    
    update_stars();

    skew_cel(&gover_skewable);

    SetVRAMPages(sport_io, SCONTEXT_BITMAP, 0, sc->sc_NumBitmapPages, ~0);

    begin_3d();
    raster_scene(NULL, NULL, gover);
    end_3d();
    
    flip_display();

    do_over_input(delta_time);
}

void grab_handler(uint32 delta_time)
{    
    // Flush inputs
    read_device_inputs();

    /*
    GetControlPad(1, FALSE, &cped);
	buttons = cped.cped_ButtonBits;
    prev_buttons = buttons;
    */

    update_stars();

    SetVRAMPages(sport_io, SCONTEXT_BITMAP, 0, sc->sc_NumBitmapPages, ~0);

    begin_3d();
    add_obj(LCONTEXT_LEVEL.obj, FALSE);
    add_enemies();
    add_obj(player.obj, FALSE);
    end_3d();
    raster_scene(NULL, NULL, lives[0]);
    draw_spikes();
    flip_display();

    if (player.obj->world_z > LEVEL_ZFAR)
    {
        --player.lives;
        update_life_hud();
        set_play_handler(PLAY_HANDLER_INTRO); // Reset level
    }
    else 
    {
        player.obj->world_z += MulSF16(obj_velocity, delta_time);
        obj_velocity += 13108;
        
        if (killshot_enemy)
            killshot_enemy->obj->world_z = player.obj->world_z;
    }
}

void hit_handler(uint32 delta_time)
{
    vec3f16 angles;
    Boolean reset = FALSE;

    // Flush inputs
    read_device_inputs();

    /*
    GetControlPad(1, FALSE, &cped);
	buttons = cped.cped_ButtonBits;
    prev_buttons = buttons;
    */

    angles[0] = 0;
    angles[1] = 0;
    angles[2] = MulSF16(50331648, delta_time);

    rotate_obj(player.obj, angles);

    player.obj->world_z -= MulSF16(131072, delta_time);

    if (player.obj->world_z <= camera.world_z + 32768)
    {
        player.obj->world_z = camera.world_z + 32768;
        reset = TRUE;
    }

    update_stars();

    SetVRAMPages(sport_io, SCONTEXT_BITMAP, 0, sc->sc_NumBitmapPages, ~0);

    begin_3d();
    add_obj_zclip(LCONTEXT_LEVEL.obj, CAM_NEAR);

    /*  If player dies below the rim then it was due to a spike during level transition. 
        During the transition phase, enemies are not rendered. Only add enemies if player
        dies while above or on the rim. This will mean all enemies should be visible
        since it happend during standard gameplay. */    
    if (player.obj->world_z <= LEVEL_ZNEAR)
        add_enemies();

    add_obj(player.obj, FALSE);
    end_3d();
    raster_scene(NULL, NULL, lives[0]);
    draw_spikes();
    flip_display();

    if (reset)
        set_play_handler(PLAY_HANDLER_INTRO); // Reset level
}

void intro_handler(uint32 delta_time)
{
    vec3f16 angles;
    
    if ( !(zapper->ccb_Flags & CCB_SKIP))
    {
        if (tick_start_time - zapper_msg_time.previous_time >= zapper_msg_time.delay)
            zapper->ccb_Flags |= CCB_SKIP;
        else 
            skew_cel(&zapper_skewable);
    }

    // Level model animation

    if (LCONTEXT_LEVEL.obj->world_z > (20 << FRACBITS_16))
    {
        // Move level closer until we are ready to rotate it around.

        LCONTEXT_LEVEL.obj->world_z -= MulSF16(obj_velocity, delta_time);
        obj_velocity += 98304;

        if (LCONTEXT_LEVEL.obj->world_z <= (20 << FRACBITS_16))
        {
            LCONTEXT_LEVEL.obj->world_z = 20 << FRACBITS_16;
            obj_velocity = 327680;
        }
    }
    else 
    {
        // Rotate level around

        if (LCONTEXT_LEVEL.obj->world_z < 0)
        {
            // Finished
            LCONTEXT_LEVEL.obj->world_z = 0;
            copy_vertex_def(&LCONTEXT_LEVEL.obj->vertex_def, &LCONTEXT_LEVEL.obj->vertex_def_copy);
            set_play_handler(PLAY_HANDLER_GAME);
        }
        else 
        {
            LCONTEXT_LEVEL.obj->world_z -= MulSF16(327680, delta_time);

            angles[0] = 0;
            angles[1] = MulSF16(1048576, delta_time);
            angles[2] = 0;

            rotate_obj(LCONTEXT_LEVEL.obj, angles);   
        }
    }

    // Flush events
    read_device_inputs();

    /*
    GetControlPad(1, FALSE, &cped);
	buttons = cped.cped_ButtonBits;
    prev_buttons = buttons;
    */

    SetVRAMPages(sport_io, SCONTEXT_BITMAP, 0, sc->sc_NumBitmapPages, ~0);

    begin_3d();
    add_obj(LCONTEXT_LEVEL.obj, FALSE);
    end_3d();

    raster_scene_wireframe(LCONTEXT_LEVEL.wireframe_color, zapper);

    flip_display();
}

void game_handler(uint32 delta_time)
{
    // Game logic    

    // Update screen effects
    if (zapper_effect.active)
    {
        if (tick_start_time - zapper_effect.zap_time.previous_time >= zapper_effect.zap_time.delay)
        {
            zapper_effect.active = FALSE;
            zapper_effect.zap_state = FALSE;
            reset_screen_colors(sc);
            enemy_spawn_time.previous_time = tick_start_time;
        }
        else 
        {
            if (zapper_effect.zap_state)
                reset_screen_colors(sc);
            else 
                apply_screen_luminance(sc, 86);

            zapper_effect.zap_state = !zapper_effect.zap_state;
        }        
    }

    // Update enemy explosions
    if ( !(explode->ccb_Flags & CCB_SKIP) )
    {
        explode->ccb_HDX += 209715;
        explode->ccb_VDY += 13107;

        explode->ccb_XPos -= 32768;
        explode->ccb_YPos -= 32768;

        if (explode->ccb_HDX > 7340032)
            explode->ccb_Flags |= CCB_SKIP;
    }
    
    // Spikers only appear on and after level 4
    if (current_level >= 4)
    {
        // Is watch-out message hidden?
        if (watch_out->ccb_Flags & CCB_SKIP)
        {
            // Is it almost time to switch levels?
            if (tick_start_time - next_level_time.previous_time >= (next_level_time.delay - 1800))
            {
                // Only show message if there are spikes
                if (get_spike_count() > 0)
                    watch_out->ccb_Flags &= ~CCB_SKIP;
            }
        }
        else 
        {
            // Stretch message around for effects
            skew_cel(&watch_skewable);         
        }
    }

    // Time to switch levels?
    if (tick_start_time - next_level_time.previous_time >= next_level_time.delay)
    {
        next_level_time.previous_time = tick_start_time;
        set_play_handler(PLAY_HANDLER_SWITCH);
    }

    // Time to release new enemy?
    if (tick_start_time - enemy_spawn_time.previous_time >= enemy_spawn_time.delay)
    {
        if (!zapper_effect.active)
        {
            spawn_next_enemy();
            enemy_spawn_time.delay = rand() % 900;
            enemy_spawn_time.previous_time = tick_start_time;
        }
    }

    service_sample_player();

    update_bullets(delta_time);
    do_game_input(delta_time);

    if (player.velocity_z != 0)
    {
        player.obj->world_z += MulSF16(player.velocity_z, delta_time);
        player.velocity_z += GRAVITY_Z;
        
        if (player.obj->world_z >= LEVEL_ZNEAR)
        {
            snap_player(Z_AXIS);
            player.velocity_z = 0;
        }
    }
   
    update_cam(delta_time);
    update_enemies(delta_time);    
    update_score();
    update_stars();

    // Draw 

    SetVRAMPages(sport_io, SCONTEXT_BITMAP, 0, sc->sc_NumBitmapPages, ~0);

    begin_3d();
        add_obj(LCONTEXT_LEVEL.obj, TRUE);
        add_bullets();
        add_obj(player.obj, TRUE);
        add_enemies();
    end_3d();

    raster_scene(NULL, NULL, lives[0]);
    draw_spikes();
    draw_guides();

    flip_display();
}

void load_audio(void)
{    
    uint32 i;
    rez_envelope_typ rez_envelope;
    char *sfx_names[SFX_MAX] = {"Zap", "Boom", "Pulse", "Whoa", "Clear", "Voice"};
    char sfx_path[40];

    for (i = 0; i < SFX_MAX; i++)
    {
        sprintf(sfx_path, "Assets/Audio/%s.aiff", sfx_names[i]);
        load_resource(sfx_path, REZ_SAMPLE, &rez_envelope);
        sfx[i] = (Item*) rez_envelope.data;
    }   
}

void do_game_input(uint32 delta_time)
{  
    read_device_inputs();   

    // Is player using control pad or mouse?

    if (BUTTONS)    
    {
        // Control pad takes precedence

        do_game_input_pad(delta_time);        
    }
    else            
    {
        // Try mouse 

        player_move_vel = 0;

        do_game_input_mouse(delta_time);
    }
}

void do_game_input_pad(uint32 delta_time)
{
    polygon_typ_ptr poly;
    vertex_typ_ptr verts;
    int32 step_amount;
    int32 angle;
    vec3f16 p1, p2;
    uint32 dist;    

    #if LEVEL_SKIP
        if ((BUTTONS & ControlX) && !(PREV_BUTTONS & ControlX))
            set_play_handler(PLAY_HANDLER_SWITCH);
    #endif

    poly = &player.obj->polygons[ player_poly_order[1] ];
    verts = player.obj->vertex_def.vertices;

    if ((BUTTONS & ControlRight) || (BUTTONS & ControlLeft))
	{	
        step_amount = MulSF16(BASE_MOVE_SPEED + player_move_vel, delta_time);

        if (BUTTONS & ControlRight)
        {
            angle = -player.rotation_angle;
            // printf("going right %d\n", angle);
        }
        else 
        {
            angle = -player.rotation_angle + ANG_128;
            // printf("going left %d\n", angle);
        }

        trans_vertex_xy_by(verts[poly->vertex_lut[0]].vertex, step_amount, angle);
        trans_vertex_xy_by(verts[poly->vertex_lut[1]].vertex, step_amount, angle);

        p1[0] = player.obj->world_x;
        p1[1] = player.obj->world_y;
        p1[2] = player.obj->world_z;

        p2[0] = verts[poly->vertex_lut[0]].vertex[0] + player.obj->world_x;
        p2[1] = verts[poly->vertex_lut[0]].vertex[1] + player.obj->world_y;
        p2[2] = verts[poly->vertex_lut[0]].vertex[2] + player.obj->world_z;

        dist = get_squared_dist(p1, p2);

        if (dist > 3600)
        {
            if (BUTTONS & ControlRight)
            {
                ++player.corridor_index;

                player.active_vdef = &player.left_weight_vdef;

                if (player.corridor_index >= LCONTEXT_LEVEL.obj->poly_count)
                {
                    if (LCONTEXT_LEVEL.wrap)
                    {
                        player.corridor_index = 0;
                    }
                    else 
                    {
                        // Invalid move
                        player.corridor_index = LCONTEXT_LEVEL.obj->poly_count - 1;
                        player.active_vdef = &player.right_weight_vdef;
                    }
                }
            }
            else // Left
            {
                --player.corridor_index;

                player.active_vdef = &player.right_weight_vdef;

                if (player.corridor_index < 0)
                {
                    if (LCONTEXT_LEVEL.wrap)
                    {
                        player.corridor_index = LCONTEXT_LEVEL.obj->poly_count - 1;
                    }
                    else 
                    {
                        // Invalid move
                        player.corridor_index = 0;
                        player.active_vdef = &player.left_weight_vdef;
                    }
                }
            }
            
            snap_player(X_AXIS|Y_AXIS);
        }        

        player_move_vel += MOVE_VEL_INC;

        if (player_move_vel > MAX_MOVE_VEL)
            player_move_vel = MAX_MOVE_VEL;
	}	

    // Fire 
    if (BUTTONS & ControlA)
        fire_bullet();

    // Super zapper 
    if ((BUTTONS & ControlB) && !(PREV_BUTTONS & ControlB))
        use_zapper();

    // Jump
    if ((BUTTONS & ControlC) && !(PREV_BUTTONS & ControlC))
    {
        if (player.obj->world_z == LEVEL_ZNEAR)
            player.velocity_z = JUMP_FORCE;
    }
}

void do_game_input_mouse(uint32 delta_time)
{
    polygon_typ_ptr poly;
    vertex_typ_ptr verts;
    int32 step_amount;
    int32 angle;
    int32 mdx;
    vec3f16 p1, p2;
    uint32 dist;        

    poly = &player.obj->polygons[ player_poly_order[1] ];
    verts = player.obj->vertex_def.vertices;

    mdx = MOUSE_DX;

    if (mdx != 0)
    {
        // Set mouse delta x limits
        if (mdx < -30) mdx = -30;
        else if (mdx > 30) mdx = 30;

        step_amount = MulSF16((mdx << FRACBITS_16), mouse_sens);
        step_amount = MulSF16(step_amount, delta_time);

        step_amount = ABS_VALUE(step_amount);

        if (mdx > 0)    // Right
            angle = -player.rotation_angle;
        else            // Left
            angle = -player.rotation_angle + ANG_128;

        trans_vertex_xy_by(verts[poly->vertex_lut[0]].vertex, step_amount, angle);
        trans_vertex_xy_by(verts[poly->vertex_lut[1]].vertex, step_amount, angle);

        p1[0] = player.obj->world_x;
        p1[1] = player.obj->world_y;
        p1[2] = player.obj->world_z;

        p2[0] = verts[poly->vertex_lut[0]].vertex[0] + player.obj->world_x;
        p2[1] = verts[poly->vertex_lut[0]].vertex[1] + player.obj->world_y;
        p2[2] = verts[poly->vertex_lut[0]].vertex[2] + player.obj->world_z;

        dist = get_squared_dist(p1, p2);

        if (dist > 3600)
        {
            if (MOUSE_DX > 0)
            {
                ++player.corridor_index;

                player.active_vdef = &player.left_weight_vdef;

                if (player.corridor_index >= LCONTEXT_LEVEL.obj->poly_count)
                {
                    if (LCONTEXT_LEVEL.wrap)
                    {
                        player.corridor_index = 0;
                    }
                    else 
                    {
                        // Invalid move
                        player.corridor_index = LCONTEXT_LEVEL.obj->poly_count - 1;
                        player.active_vdef = &player.right_weight_vdef;
                    }
                }
            }
            else // Left
            {
                --player.corridor_index;

                player.active_vdef = &player.right_weight_vdef;

                if (player.corridor_index < 0)
                {
                    if (LCONTEXT_LEVEL.wrap)
                    {
                        player.corridor_index = LCONTEXT_LEVEL.obj->poly_count - 1;
                    }
                    else 
                    {
                        // Invalid move
                        player.corridor_index = 0;
                        player.active_vdef = &player.left_weight_vdef;
                    }
                }
            }
            
            snap_player(X_AXIS|Y_AXIS);
        }        
    }

    // Fire 
    if (MOUSE_BUTTONS & MouseLeft)
        fire_bullet();

    // Super zapper 
    if ((MOUSE_BUTTONS & MouseMiddle) && !(PREV_MOUSE_BUTTONS & MouseMiddle))
        use_zapper();

    // Jump
    if ((MOUSE_BUTTONS & MouseRight) && !(PREV_MOUSE_BUTTONS & MouseRight))
    {
        if (player.obj->world_z == LEVEL_ZNEAR)
            player.velocity_z = JUMP_FORCE;
    }
}

void do_switch_input(uint32 delta_time)
{       
    read_device_inputs();   

    // The only thing you can do during level switch is fire on spikes.

    if (player.obj->world_z > LEVEL_ZFAR)
        return; // Nothing to possibly shoot at.

    // Is player using control pad or mouse?

    if (BUTTONS)    
    {
        // Control pad takes precedence

        if (BUTTONS & ControlA)
            fire_bullet();        
    }
    else            
    {
        // Try mouse

        if (MOUSE_BUTTONS & MouseLeft)
            fire_bullet();
    }
}

void do_over_input(uint32 delta_time)
{            
    read_device_inputs();

    if ( (BUTTONS && !(PREV_BUTTONS)) || (MOUSE_BUTTONS && !(PREV_MOUSE_BUTTONS)) )
    {
        if (game_settings & GAME_SET_MUSIC_MASK)
            stop_music();
            
        new_game();
    }
}

void load_bullets(void)
{    
    uint32 i;
    int32 angle;
    int32 angle_inc;

    memset((void*)bullets, 0, sizeof(bullet_typ) * MAX_BULLETS);

    bullets[0].obj = load_obj("Assets/Entities/Billboard");
    scale_obj(bullets[0].obj, 6553);

    bullets[0].obj->polygons[0].ccb = create_coded_colored_cel8(8, 8, MakeRGB15(31,31,0));
    bullets[0].obj->bsphere_radius = calc_bsphere_radius(bullets[0].obj);
    bullets[0].active = FALSE;

    FastMapCelInit(bullets[0].obj->polygons[0].ccb);

    // Set up remaining bullets
    for (i = 1; i < MAX_BULLETS; i++)
    {
        bullets[i].obj = copy_obj(bullets[0].obj);
        bullets[i].active = FALSE;
        FastMapCelInit(bullets[i].obj->polygons[0].ccb);
    }

    // Set up volley

    angle = 0;
    angle_inc = DivSF16(16777216, MAX_BULLETS << FRACBITS_16);

    for (i = 0; i < MAX_BULLETS; i++)
    {
        volley_adj[i].pt_X = MulSF16(CosF16(angle), 6000); // Scale it down
        volley_adj[i].pt_Y = MulSF16(SinF16(angle), 9000);
        angle += angle_inc;
    }
}

void fire_bullet(void)
{
    // uint32 i = MAX_BULLETS;
    uint32 i;
    bullet_typ_ptr bullet = bullets;

    if (!player.active)
        return;

    if (!is_simple_timer_ready(&bullet_rate_timer, time_io))
        return;

    // Hard mode gets 1 less bullet
    i = (game_settings & GAME_SET_HARD_MASK) ? (MAX_BULLETS - 2) : MAX_BULLETS;
    
    while (i--) 
    {
        if (!bullet->active)
        {
            bullet->obj->world_x = player.obj->world_x;
            bullet->obj->world_y = player.obj->world_y;
            bullet->obj->world_z = player.obj->world_z;
            bullet->corridor_index = player.corridor_index;
            bullet->active = TRUE;
            
            bullet->obj->world_x += volley_adj[volley_index].pt_X;
            bullet->obj->world_y += volley_adj[volley_index].pt_Y;

            play_sample(*sfx[SFX_ZAP], 200, 0x40D8);

            ++volley_index;

            if (volley_index >= MAX_BULLETS)
                volley_index = 0;

            break;
        }

        ++bullet;
    }

    reset_simple_timer(&bullet_rate_timer, time_io);
}

void use_zapper(void)
{
    if (!player.active)
        return;

    if ( !(powerup_flags & PUP_ZAPPER_MASK) )
        return;

    powerup_flags &= ~PUP_ZAPPER_MASK;

    play_sample(*sfx[SFX_CLEAR], 500, 0x40D8);

    zapper_effect.active = TRUE;
    zapper_effect.zap_time.previous_time = tick_start_time;

    zap_enemies();
}

void add_bullets(void)
{
    uint32 i;
    bullet_typ_ptr bullet_it;

    bullet_it = bullets;
    i = MAX_BULLETS;

    while (i--)
    {
        if (bullet_it->active)
            add_obj(bullet_it->obj, TRUE);

        ++bullet_it;
    }
}

void update_bullets(uint32 delta_time)
{
    uint32 i, j;
    int32 delta_x;
    int32 delta_y;
    bullet_typ_ptr bullet_it;
    spike_typ_ptr spike_it;
    spike_typ_ptr corridor_spike;
    enemy_typ_ptr enemy_it;
    enemy_typ_ptr corridor_enemy;

    bullet_it = bullets;
    i = MAX_BULLETS;

    while (i--)
    {
        if (bullet_it->active)
        {
            // Check for spike on bullet's corridor
            if (player.active)
            {
                spike_it = spikes;
                j = MAX_SPIKES;
                corridor_spike = NULL;
                
                while (j--)
                {
                    if (spike_it->active)
                    {
                        if (spike_it->corridor_index == bullet_it->corridor_index)
                        {
                            if (bullet_it->obj->world_z >= spike_it->end_pos[2])
                                corridor_spike = spike_it;

                            break;                                   
                        }
                    }

                    ++spike_it;
                }
                
                // Check for enemy collision

                enemy_it = enemies;
                j = MAX_ENEMIES;
                corridor_enemy = NULL;

                while (j--)
                {                
                    if (enemy_it->state == ES_ACTIVE)
                    {
                        if ((enemy_it->enemy_type == FLIPPER || enemy_it->enemy_type == FUSEBALL) 
                            && enemy_it->obj->world_z == LEVEL_ZNEAR)
                        {
                            if (bullet_it->obj->world_z <= (LEVEL_ZNEAR + 32768)) 
                            {
                                // Bullet in Z range

                                delta_x = ABS_VALUE(enemy_it->obj->world_x - bullet_it->obj->world_x);
                                delta_y = ABS_VALUE(enemy_it->obj->world_y - bullet_it->obj->world_y);

                                if (delta_x < 12000 && delta_y < 12000)
                                {
                                    // Hit
                                    corridor_enemy = enemy_it;
                                    break;
                                }
                            }
                        }
                        else 
                        {
                            // Be clever rather than run many 3D bounding sphere collision checks per bullet)
                            
                            // Don't bother checking collision unless enemy shares the bullet's corridor
                            if (bullet_it->corridor_index == enemy_it->corridor_index)
                            {
                                // Check bullet/enemy Z extents
                                if (bullet_it->obj->world_z >= enemy_it->obj->world_z - 16384 && 
                                    bullet_it->obj->world_z <= enemy_it->obj->world_z + 16384)
                                {                   
                                    if (enemy_it->enemy_type == FUSEBALL)
                                    {
                                        // Only hit IF fuseball is not on corridor joint

                                        vec3f16 p1;
                                        int32 dist;
                                        poly_props_typ props;
                                        
                                        props = get_corridor_props(&LCONTEXT_LEVEL.obj->polygons[enemy_it->corridor_index]);

                                        p1[X] = enemy_it->obj->world_x;
                                        p1[Y] = enemy_it->obj->world_y;
                                        p1[Z] = props.near_edge[1][Z];

                                        dist = get_squared_dist(p1, props.near_edge[1]);
                                        
                                        if (dist < 1000)
                                        {
                                            corridor_enemy = enemy_it;
                                            break;
                                        }
                                    }
                                    else 
                                    {
                                        // Simple case. Enemy hit.
                                        corridor_enemy = enemy_it;
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    ++enemy_it;
                }

                if (corridor_spike || corridor_enemy)
                {
                    // Bullet hit either a spike or enemy

                    if (corridor_spike && corridor_enemy)
                    {
                        // Find out which one is closer
                        if (corridor_enemy->obj->world_z <= corridor_spike->end_pos[2])
                        {
                            // Enemy in front
                            corridor_spike = NULL;
                        }
                        else 
                        {
                            // Spike in front 
                            corridor_enemy = NULL;
                        } 
                    }

                    if (corridor_spike)
                    {
                        // Spike hit
                        damage_spike(corridor_spike);                    
                    }
                    else
                    {
                        // Enemy hit
                        
                        --corridor_enemy->health;

                        if (corridor_enemy->health <= 0)
                        {
                            Point explosion_center;
                            vec3f16 enemy_center;
                            
                            enemy_score(corridor_enemy);
                            
                            // Set enemy explosion
                            enemy_center[X] = corridor_enemy->obj->world_x;
                            enemy_center[Y] = corridor_enemy->obj->world_y;
                            enemy_center[Z] = corridor_enemy->obj->world_z;

                            point_to_camera(enemy_center, enemy_center);  
                            point_to_screen(&explosion_center, enemy_center, TRUE);
                            
                            explode->ccb_HDX = 1258291;
                            explode->ccb_VDY = 78643;
                            explode->ccb_XPos = (explosion_center.pt_X - 8) << FRACBITS_16;
                            explode->ccb_YPos = (explosion_center.pt_Y - 8) << FRACBITS_16;
                            explode->ccb_Flags &= ~CCB_SKIP;

                            // Remove enemy
                            destroy_enemy(corridor_enemy, TRUE);                                                    
                        }
                    }

                    bullet_it->active = FALSE;           
                }
            }

            if (bullet_it->active)
            {
                // Bullet hasn't hit anything this frame

                bullet_it->obj->world_z += MulSF16(BULLET_SPEED, delta_time);

                if (bullet_it->obj->world_z >= (LEVEL_ZFAR + 16384))
                    bullet_it->active = FALSE;                
            }
        }

        ++bullet_it;
    }
}

void load_scores(void)
{
    uint32 i;
    char buffer[32];
    rez_envelope_typ rez_envelope;
    Rect sub_rect;

    // Load atlas
    load_resource("Assets/Graphics/UI/Digits.cel", REZ_CEL, &rez_envelope);
    score_atlas = (CCB*) rez_envelope.data;

    // Load point-display cels
    for (i = 0; i < MAX_ENEMY_TYPES; i++)
    {
        sprintf(buffer, "Assets/Graphics/P%d.cel", score_table[i]);
        load_resource(buffer, REZ_CEL, &rez_envelope);
        score_cels[i] = (CCB*) rez_envelope.data;
        score_cels[i]->ccb_Flags |= (CCB_LDPLUT | CCB_SKIP | CCB_MARIA | CCB_LDPPMP);
        score_cels[i]->ccb_Flags &= ~CCB_BGND;        
        score_cels[i]->ccb_PIXC = 0x01000100;    // 1/2
        //score_cels[i]->ccb_PIXC = 0x02800280; // 1/4
        // score_cels[i]->ccb_PIXC = 0x03800380; // 1/8

        if (i > 0)
            LINK_CEL(score_cels[i-1], score_cels[i]);
    }

    // Prep score digit cels
    for (i = 0; i < MAX_SCORE_DIGITS; i++)
    {
        sub_rect.rect_XLeft = 0;
        sub_rect.rect_YTop = 0;
        sub_rect.rect_XRight = DIGIT_WIDTH - 1; // Inclusive
        sub_rect.rect_YBottom = DIGIT_HEIGHT - 1;

        score_num_cels[i] = create_coded_cel8(score_atlas->ccb_Width, score_atlas->ccb_Height, FALSE);
        score_num_cels[i]->ccb_SourcePtr = score_atlas->ccb_SourcePtr;
        score_num_cels[i]->ccb_PLUTPtr = score_atlas->ccb_PLUTPtr;
        score_num_cels[i]->ccb_XPos = (8 + (14 * i)) << FRACBITS_16;
        score_num_cels[i]->ccb_YPos = 6 << FRACBITS_16;       

        set_cel_subregion_bpp8(score_num_cels[i], &sub_rect, DIGIT_ATLAS_WIDTH);        

        if (i > 0)
            LINK_CEL(score_num_cels[i-1], score_num_cels[i]);
    }
}

void enemy_score(enemy_typ_ptr enemy)
{
    CCB *ccb = score_cels[enemy->enemy_type];

    score += score_table[enemy->enemy_type];

    // Reset point cel position
    ccb->ccb_XPos = 8519680;    
    ccb->ccb_YPos = 6815744;
    // ccb->ccb_XPos = enemy->obj->polygons[0].screen[0].pt_X;
    // ccb->ccb_YPos = enemy->obj->polygons[0].screen[0].pt_Y;

    // Reset point cel scale
    ccb->ccb_HDX = 1310720;
    ccb->ccb_VDY = 81920;
    
    ccb->ccb_Flags &= ~CCB_SKIP;

    play_sample(*sfx[SFX_BOOM], DEFAULT_AUDIO_PRIORITY, 0x40D8);

    update_score_hud();

    #if DEBUG_MODE
        printf("score %d\n", score);
    #endif
}

void update_score(void)
{
    uint32 i;
    CCB *ccb;

    for (i = 0; i < MAX_ENEMY_TYPES; i++)
    {
        ccb = score_cels[i];

        if (!(ccb->ccb_Flags & CCB_SKIP))
        {
            ccb->ccb_HDX += 157286;
            ccb->ccb_VDY += 9830;
            ccb->ccb_XPos -= 327680;
            ccb->ccb_YPos -= 163840;

            if (ccb->ccb_XPos < 1835008)
                ccb->ccb_Flags |= CCB_SKIP;
        }
    }
}

void clear_score_cels(void)
{
    uint32 i;

    for (i = 0; i < MAX_ENEMY_TYPES; i++)
        score_cels[i]->ccb_Flags |= CCB_SKIP;
}

void draw_guides(void)
{
    polygon_typ_ptr poly;
    static uint32 guide_color = MakeRGB15(31, 31, 0);

    poly = &LCONTEXT_LEVEL.obj->polygons[player.corridor_index];

    draw_poly_wireframe(poly, guide_color);
}

void load_fonts(void)
{
    rez_envelope_typ rez_envelope;
    load_resource("Assets/Fonts/zx.3do", REZ_FONT, &rez_envelope);
    font_desc = (FontDescriptor*) rez_envelope.data;    
}

void clear_bullets(void)
{
    uint32 i;

    for (i = 0; i < MAX_BULLETS; i++)
        bullets[i].active = FALSE;
}

/* *************************************************************************************** */
/* =========================== PUBLIC FUNCTION DEFINITIONS =============================== */
/* *************************************************************************************** */

void showcase_ship(void)
{
    static int32 vector_x = ONE_F16;
    static int32 vector_y = ONE_F16;
    vec3f16 angles;

    player.obj->world_x += MulSF16(200, vector_x);
    player.obj->world_y += MulSF16(240, vector_y);

    if (player.obj->world_x < -ONE_F16 || player.obj->world_x > ONE_F16)
        vector_x = -vector_x;

    if (player.obj->world_y < -ONE_F16 || player.obj->world_y > ONE_F16)
        vector_y = -vector_y;

    angles[X] = 0;
    angles[Y] = ONE_F16;
    angles[Z] = ONE_F16;

    rotate_obj(player.obj, angles);
}

void set_play_handler(uint32 index)
{
    #if DEBUG_MODE 
        if (index >= PLAY_HANDLER_MAX)
            printf("Error - Invalid play handler index %d.\n", index);
    #endif 

    if (player.lives == 0)
        index = PLAY_HANDLER_OVER;

    reset_screen_colors(sc);

    explode->ccb_Flags |= CCB_SKIP;
    watch_out->ccb_Flags |= CCB_SKIP;

    if (index == PLAY_HANDLER_GAME)
    {
        volley_index = 0;
        reset_player();        
        enemy_spawn_time.previous_time = tick_start_time;
        next_level_time.previous_time = tick_start_time;
        zapper_effect.active = FALSE;
        zapper_effect.zap_state = FALSE;    
        reset_simple_timer(&bullet_rate_timer, time_io);
    }
    else if (index == PLAY_HANDLER_SWITCH)
    {
        obj_velocity = 131072;
        clear_enemies();
    }
    else if (index == PLAY_HANDLER_GRABBED)
    {
        obj_velocity = 131072;
    }
    else if (index == PLAY_HANDLER_INTRO)
    {
        clear_entities();
        reset_corridors();        
        reset_level_transrot();
        reset_play_camera();

        obj_velocity = 655360;      

        #if DEBUG_MODE 
            printf("Starting level %d.\n", current_level);
        #endif  
    }
    else if (index == PLAY_HANDLER_HIT)
    {
        play_sample(*sfx[SFX_BOOM], DEFAULT_AUDIO_PRIORITY, DEFAULT_AUDIO_AMPLITUDE);
        --player.lives;
        update_life_hud();
    }
    else if (index == PLAY_HANDLER_OVER)
    {
        reset_skewable_cel(&gover_skewable);
        gover->ccb_Flags &= ~CCB_SKIP;
    }
    else if (index == PLAY_HANDLER_END)
    {
        LINK_CEL(end_msg, score_num_cels[0]);

        player.obj->world_x = player.obj->world_y = 0;
        player.obj->world_z = 3 << FRACBITS_16;
        zero_camera();
        // Reset player rotation
        copy_vertex_def(&player.obj->vertex_def, &player.obj->vertex_def_copy);
        // Make it larger
        scale_obj(player.obj, 196608);
    }

    clear_score_cels();

    play_handler_index = index;
}

void play_start(void)
{
    rez_envelope_typ rez_envelope;

    init_player();
    load_bullets();
    init_enemies();
    load_audio();
    load_fonts();
    init_stars();
    init_3d();

    // Call reset_level_manager() before level cycling
    init_level_manager();	

    init_music_manager();

    // Set up logic handlers
    play_handlers[PLAY_HANDLER_INTRO] = intro_handler;
    play_handlers[PLAY_HANDLER_GAME] = game_handler;
    play_handlers[PLAY_HANDLER_HIT] = hit_handler;
    play_handlers[PLAY_HANDLER_SWITCH] = switch_handler;
    play_handlers[PLAY_HANDLER_END] = end_handler;
    play_handlers[PLAY_HANDLER_OVER] = over_handler;
    play_handlers[PLAY_HANDLER_GRABBED] = grab_handler;

    load_scores();    

    // Set up foreground   

    load_resource("Assets/Graphics/UI/Watch.cel", REZ_CEL, &rez_envelope);
    watch_out = (CCB*) rez_envelope.data;
    watch_out->ccb_Flags |= (/*CCB_LDPLUT | */CCB_SKIP);
    watch_out->ccb_Flags &= ~CCB_BGND;
    watch_out->ccb_XPos = 67 << FRACBITS_16;
    watch_out->ccb_YPos = 30 << FRACBITS_16;
    watch_out_dir = 1;
    watch_skewable = get_skewable_cel(watch_out);

    load_resource("Assets/Graphics/UI/Over.cel", REZ_CEL, &rez_envelope);
    gover = (CCB*) rez_envelope.data;
    gover->ccb_Flags |= (CCB_LDPLUT | CCB_SKIP);
    gover->ccb_Flags &= ~CCB_BGND;
    gover->ccb_XPos = 51 << FRACBITS_16;
    gover->ccb_YPos = 101 << FRACBITS_16;
    gover_skewable = get_skewable_cel(gover);

    load_resource("Assets/Graphics/UI/End.cel", REZ_CEL, &rez_envelope);
    end_msg = (CCB*) rez_envelope.data;
    end_msg->ccb_Flags |= CCB_LDPLUT;
    end_msg->ccb_Flags &= ~CCB_BGND;
    end_msg->ccb_XPos = 0;
    end_msg->ccb_YPos = 0;       

    load_resource("Assets/Graphics/Effects/Exp.cel", REZ_CEL, &rez_envelope);
    explode = (CCB*) rez_envelope.data;
    explode->ccb_Flags |= (CCB_LDPLUT | CCB_MARIA);
    explode->ccb_Flags &= ~CCB_BGND;
    // explode->ccb_PIXC = 0x01000100; // 1/2

    load_resource("Assets/Graphics/UI/Zapper.cel", REZ_CEL, &rez_envelope);
    zapper = (CCB*) rez_envelope.data;
    zapper->ccb_Flags |= (/*CCB_LDPLUT | */CCB_SKIP);
    zapper->ccb_Flags &= ~CCB_BGND;
    zapper->ccb_XPos = 38 << FRACBITS_16;
    zapper->ccb_YPos = 30 << FRACBITS_16;
    zapper_skewable = get_skewable_cel(zapper);
    LAST_CEL(zapper);

    // Set default settings. Hard mode is off by default.
    game_settings = GAME_SETTINGS_CLEAR;
    SET_GAME_SETTING(GAME_SET_MUSIC_MASK);    

    bullet_rate_timer = create_simple_timer(115);
    
    new_game();
}

int32 play_update(uint32 delta_time)
{   
    tick_start_time = GetMSecTime(time_io);

    play_handlers[play_handler_index](delta_time);

    return(1); // Never quit
}

void play_stop(void)
{
    // Never reached since play state is at the root
}
