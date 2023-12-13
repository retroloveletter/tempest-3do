#include "game_globals.h"

#define ENEMY_BILLBOARD_W 32
#define ENEMY_BILLBOARD_H 32
#define ENEMY_BILLBOARD_S 42598
#define MAX_ANIM_CYCLE 10
#define PULSAR_COLOR 924

/* *************************************************************************************** */
/* ================================== PRIVATE TYPESS ===================================== */
/* *************************************************************************************** */

struct 
{
    uint32 enemy_counts[MAX_ENEMY_TYPES];
    uint32 active_spikes;
    uint32 active_enemies;
} hostile_stats;

typedef struct cel_anim_typ 
{
    uint32 frame_count;                     // Total frames in set
    int32 frame_cycle_lut[MAX_ANIM_CYCLE];  // A value of -1 denotes the end
    cel8_data_typ_ptr *frames;              // Pointer to frame array
} cel_anim_typ, *cel_anim_typ_ptr;

/* *************************************************************************************** */
/* ==================================== GLOBAL VARS ====================================== */
/* *************************************************************************************** */

enemy_typ enemies[MAX_ENEMIES];
simple_timer_typ enemy_spawn_timer;
enemy_typ_ptr killshot_enemy;
spike_typ spikes[MAX_SPIKES];

/* *************************************************************************************** */
/* =================================== PRIVATE VARS ====================================== */
/* *************************************************************************************** */

static void (*init_enemy_handlers[MAX_ENEMY_TYPES])(enemy_typ_ptr);
static void (*enemy_update_handlers[MAX_ENEMY_TYPES])(enemy_typ_ptr, uint32 delta_time);
static cel_anim_typ enemy_anims[MAX_ENEMY_TYPES];
static cel_anim_typ zapped_anim;
static simple_timer_typ anim_timer;

static char *enemy_names[MAX_ENEMY_TYPES] = {"Missile", "Flipper", 
    "Tanker", "Spiker", "Fuseball", "Pulsar", "Ftanker", "Ptanker"};

// Max simultaneous types
static uint32 enemy_max_counts[MAX_ENEMY_TYPES] = {
    99, // Missile
    2,  // Flipper
    1,  // Tanker
    1,  // Spiker
    1,  // Fuseball
    1,  // Pulsar
    1,  // Ftanker
    1   // Ptanker
};

// Spawn eligibility
static uint32 enemy_level[MAX_ENEMY_TYPES] = {
    1,  // Missile
    1,  // Flipper
    3,  // Tanker
    4,  // Spiker
    11, // Fuseball
    17, // Pulsar
    33, // Ftanker
    41  // Ptanker
};

// Animations
static int32 frame_cycle_luts[MAX_ENEMY_TYPES][MAX_ANIM_CYCLE] = {
    {0, 1, 2, 1, -1, -1, -1, -1, -1, -1},   // MISSILE
    {0,-1,-1,-1, -1, -1, -1, -1, -1, -1},   // FLIPPER
    {0, 1, 2, 1, -1, -1, -1, -1, -1, -1},   // TANKER
    {0,-1,-1,-1, -1, -1, -1, -1, -1, -1},   // SPIKER
    {0, 1, 2,-1, -1, -1, -1, -1, -1, -1},   // FUSEBALL
    {0,-1,-1,-1, -1, -1, -1, -1, -1, -1},   // PULSAR
    {0, 1, 2, 1, -1, -1, -1, -1, -1, -1},   // FTANKER
    {0, 1, 2, 1, -1, -1, -1, -1, -1, -1}    // PTANKER
};

/* *************************************************************************************** */
/* =========================== PRIVATE FUNCTION PROTOTYPES =============================== */
/* *************************************************************************************** */

static void spawn_enemy(uint32 enemy_type, uint32 corridor_index, int32 world_z);
static uint32 get_enemy_corridor(uint32 enemy_type);
static void load_enemy_anims(void);
static void activate_spike(vec3f16 end_pos, uint32 corridor_index);
// Init calls
static void init_flipper(enemy_typ_ptr enemy);
static void init_spiker(enemy_typ_ptr enemy);
static void init_missile(enemy_typ_ptr enemy);
static void init_tanker(enemy_typ_ptr enemy); // Reused by all tankers
static void init_fuseball(enemy_typ_ptr enemy);
static void init_pulsar(enemy_typ_ptr enemy);
// Update calls
static void update_flipper(enemy_typ_ptr enemy, uint32 delta_time);
static void update_spiker(enemy_typ_ptr enemy, uint32 delta_time);
static void update_missile(enemy_typ_ptr enemy, uint32 delta_time);
static void update_tanker(enemy_typ_ptr enemy, uint32 delta_time);
static void update_fuseball(enemy_typ_ptr enemy, uint32 delta_time);
static void update_pulsar(enemy_typ_ptr enemy, uint32 delta_time);
// Movement behavior calls
Boolean mb_bounce(enemy_typ_ptr enemy, uint32 delta_time);
Boolean mb_cam_walk(enemy_typ_ptr enemy, uint32 delta_time);
Boolean mb_payload(enemy_typ_ptr enemy, uint32 delta_time);
Boolean mb_near_walk(enemy_typ_ptr enemy, uint32 delta_time);
Boolean mb_far_walk(enemy_typ_ptr enemy, uint32 delta_time);
Boolean mb_roll(enemy_typ_ptr enemy, uint32 delta_time);
Boolean mb_side_step(enemy_typ_ptr enemy, uint32 delta_time);

/* *************************************************************************************** */
/* ============================ PUBLIC FUNCTION DEFINITIONS ============================== */
/* *************************************************************************************** */

void clear_enemies(void)
{
    uint32 i;

    for (i = 0; i < MAX_ENEMIES; i++)
        enemies[i].state = ES_INACTIVE;

    memset((void*)hostile_stats.enemy_counts, 0, sizeof(uint32) * MAX_ENEMY_TYPES);

    killshot_enemy = 0;
}

void init_enemies(void)
{
    uint32 i;    
    enemy_typ_ptr enemy;    

    load_enemy_anims();

    anim_timer = create_simple_timer(180);
    reset_simple_timer(&anim_timer, time_io);

    i = MAX_ENEMIES;
    enemy = enemies;

    while (i-- > 0)
    {        
        enemy->state = ES_INACTIVE;
        enemy->obj = load_obj("Assets/Entities/Billboard");
        scale_obj(enemy->obj, ENEMY_BILLBOARD_S);
        clone_vertex_def(&enemy->obj->vertex_def_copy, &enemy->obj->vertex_def);
        enemy->obj->polygons[0].ccb = create_coded_cel8(ENEMY_BILLBOARD_W, ENEMY_BILLBOARD_H, FALSE);
        FastMapCelInit(enemy->obj->polygons[0].ccb);
        enemy->obj->bsphere_radius = calc_bsphere_radius(enemy->obj);
        ++enemy;
    }

    // Enemy init function pointers
    init_enemy_handlers[FLIPPER] = init_flipper;
    init_enemy_handlers[MISSILE] = init_missile;
    init_enemy_handlers[TANKER] = init_tanker;
    init_enemy_handlers[SPIKER] = init_spiker;
    init_enemy_handlers[FUSEBALL] = init_fuseball;
    init_enemy_handlers[PULSAR] = init_pulsar;
    init_enemy_handlers[FTANKER] = init_tanker;
    init_enemy_handlers[PTANKER] = init_tanker;

    // Enemy update function pointers
    enemy_update_handlers[FLIPPER] = update_flipper;
    enemy_update_handlers[MISSILE] = update_missile;
    enemy_update_handlers[TANKER] = update_tanker;
    enemy_update_handlers[SPIKER] = update_spiker;
    enemy_update_handlers[FUSEBALL] = update_fuseball;
    enemy_update_handlers[PULSAR] = update_pulsar;
    enemy_update_handlers[FTANKER] = update_tanker;
    enemy_update_handlers[PTANKER] = update_tanker;
}

void update_enemies(uint32 delta_time)
{
    uint32 i;
    enemy_typ_ptr enemy;
    Boolean animate;
    int32 lut_index;
    cel_anim_typ_ptr anims;

    if (is_simple_timer_ready(&anim_timer, time_io))
    {
        animate = TRUE;
        reset_simple_timer(&anim_timer, time_io);
    }
    else 
    {
        animate = FALSE;
    }

    enemy = enemies;
    i = MAX_ENEMIES;

    while (i-- > 0)
    {
        if (enemy->state != ES_INACTIVE)
        {
            if (enemy->state == ES_ACTIVE)
                anims = &enemy_anims[enemy->enemy_type];
            else
                anims = &zapped_anim;
            
            if (anims->frame_cycle_lut[enemy->frame_index] < 0)
                enemy->frame_index = 0;

            lut_index = anims->frame_cycle_lut[enemy->frame_index];

            enemy->obj->polygons[0].ccb->ccb_SourcePtr = (CelData*) anims->frames[lut_index]->source;
            enemy->obj->polygons[0].ccb->ccb_PLUTPtr = (void*) anims->frames[lut_index]->plut;

            if (animate)
            {
                ++enemy->frame_index;                

                if (anims->frame_cycle_lut[enemy->frame_index] < 0)
                {
                    enemy->frame_index = 0;      
                                  
                    if (enemy->state == ES_DESTROY)
                        destroy_enemy(enemy, FALSE);
                }
            }

            if (enemy->state == ES_ACTIVE)
                enemy_update_handlers[enemy->enemy_type](enemy, delta_time);  
        }

        ++enemy;
    }
}

void add_enemies(void)
{
    uint32 i = MAX_ENEMIES;
    enemy_typ_ptr enemy = enemies;    

    while (i-- > 0)
    {
        if (enemy->state != ES_INACTIVE)
            add_obj(enemy->obj, TRUE);

        enemy++;
    }
}

void spawn_next_enemy(void)
{
    uint32 corridor_index;
    uint32 spawn_type;
    static uint32 next_enemy = 0;

    if (hostile_stats.active_enemies >= MAX_ENEMIES) // No more room
        return;     
    
    // Check spawn constraints for next enemy
    if ((hostile_stats.enemy_counts[next_enemy] < enemy_max_counts[next_enemy]) &&
        (current_level >= enemy_level[next_enemy]))
    {    
        spawn_type = next_enemy++;
    }
    else 
    {
        // Spawn missile and advance search for next frame
        ++next_enemy;
        spawn_type = MISSILE;
    }        

    if (next_enemy >= MAX_ENEMY_TYPES)
        next_enemy = 0;

    #if DEBUG_MODE 
        if (next_enemy >= MAX_ENEMY_TYPES || spawn_type >= MAX_ENEMY_TYPES)
            printf("Error - Invalid enemy spawn type.\n");
    #endif   

    // Get starting corridor
    corridor_index = get_enemy_corridor(spawn_type);

    spawn_enemy(spawn_type, corridor_index, LEVEL_ZFAR);
}

void activate_spike(vec3f16 end_pos, uint32 corridor_index)
{
    uint32 i = MAX_SPIKES;
    spike_typ_ptr spike = spikes;

    #if DEBUG_MODE
        Boolean slot_found = FALSE;
    #endif

    while (i--)
    {
        if (!spike->active)
        {
            spike->active = TRUE;
            spike->corridor_index = corridor_index;
            
            spike->begin_pos[0] = end_pos[0];
            spike->begin_pos[1] = end_pos[1];
            spike->begin_pos[2] = LEVEL_ZFAR;

            spike->end_pos[0] = end_pos[0];
            spike->end_pos[1] = end_pos[1];
            spike->end_pos[2] = end_pos[2];
     
            ++hostile_stats.active_spikes;

            #if DEBUG_MODE
                slot_found = TRUE;
            #endif

            break;
        }

        ++spike;
    }

    #if DEBUG_MODE 
        if (!slot_found)
            printf("Error - Unable to spawn new spike. No more room.\n");
    #endif
}

void damage_spike(spike_typ_ptr spike)
{
    if (!spike->active)
        return;

    spike->end_pos[2] += ONE_F16;

    if (spike->end_pos[2] >= spike->begin_pos[2])
    {
        // Destroy
        spike->active = FALSE;
        --hostile_stats.active_spikes;
    }
}

void draw_spikes(void)
{
    static GrafCon gcon;
    static Boolean first_run = TRUE;
    vec3f16 point_cam;    
    Point sc_begin;
    Point sc_end;    
    uint32 i = MAX_SPIKES;
    spike_typ_ptr spike = spikes;

    if (first_run)
    {
        first_run = FALSE;
        SetFGPen(&gcon, MakeRGB15(0,0,31));
    }

    while (i-- > 0)
    {
        if (spike->active)
        {
            // Point A

            point_to_camera(point_cam, spike->begin_pos);

            if (point_cam[2] < CAM_NEAR)
                point_cam[2] = CAM_NEAR;

            point_to_screen(&sc_begin, point_cam, TRUE);
            
            // Point B
            
            point_to_camera(point_cam, spike->end_pos);

            if (point_cam[2] < CAM_NEAR)
                point_cam[2] = CAM_NEAR;

            point_to_screen(&sc_end, point_cam, TRUE);

            // Draw line

            gcon.gc_PenX = sc_begin.pt_X;
            gcon.gc_PenY = sc_begin.pt_Y;

            DrawTo(SCONTEXT_BITEM, &gcon, sc_end.pt_X, sc_end.pt_Y);  
        }

        ++spike;
    }
}

void destroy_enemy(enemy_typ_ptr enemy, Boolean payload)
{
    #if DEBUG_MODE 
        if (hostile_stats.active_enemies == 0)
            printf("Error - no active enemies to destroy.\n");

        if (hostile_stats.enemy_counts[enemy->enemy_type] ==  0)
            printf("Error - no enemy of type to destroy.\n");

        if (enemy->state == ES_INACTIVE)
            printf("Error - destroy inactive enemy.\n");
    #endif
    
    if (enemy->enemy_type == PULSAR)
        reset_corridor(enemy->corridor_index);

    --hostile_stats.enemy_counts[enemy->enemy_type];
    --hostile_stats.active_enemies;

    enemy->state = ES_INACTIVE;

    if (payload)
    {
        if (enemy->enemy_type == TANKER)
            spawn_enemy(FLIPPER, enemy->corridor_index, enemy->obj->world_z);
        else if (enemy->enemy_type == FTANKER)
            spawn_enemy(FUSEBALL, enemy->corridor_index, enemy->obj->world_z);
        else if (enemy->enemy_type == PTANKER)
            spawn_enemy(PULSAR, enemy->corridor_index, enemy->obj->world_z);
    }
}

/* *************************************************************************************** */
/* =========================== PRIVATE FUNCTION DEFINITIONS ============================== */
/* *************************************************************************************** */

void init_flipper(enemy_typ_ptr enemy)
{
    enemy->health = 1;
    enemy->speed = 2;
    enemy->traversal_order = (rand() % 2) ? CCW : CW;
    enemy->logical_flag = TRUE;   
}

void init_pulsar(enemy_typ_ptr enemy)
{
    enemy->health = 1;
    enemy->speed = 1;
    enemy->ticks = rand() % 100 + 200;
    enemy->logical_flag = TRUE;
}

void init_spiker(enemy_typ_ptr enemy)
{
    enemy->health = 1;
    enemy->speed = 1;
    enemy->ticks = rand() % 50 + 80;
    enemy->logical_flag = FALSE;
}

void init_missile(enemy_typ_ptr enemy)
{
    enemy->health = 1;
    enemy->speed = 2;
}

void init_tanker(enemy_typ_ptr enemy)
{
    enemy->health = 1;
    enemy->speed = 1;
}

void init_fuseball(enemy_typ_ptr enemy)
{
    poly_props_typ props;

    enemy->health = 1;
    enemy->speed = 1;
    enemy->logical_flag = TRUE;
    enemy->ticks = 100;

    enemy->traversal_order = (rand() % 2) ? CCW : CW;

    props = get_corridor_props(&LCONTEXT_LEVEL.obj->polygons[enemy->corridor_index]);

    if (rand() % 2)
    {
        enemy->obj->world_x = props.near_edge[0][0];
        enemy->obj->world_y = props.near_edge[0][1];        
    }
    else
    {
        enemy->obj->world_x = props.near_edge[2][0];
        enemy->obj->world_y = props.near_edge[2][1];
    }
}

/*  Flipper attack timing used to be too precise and this made killing flippers
    difficult once they were on the rim. Flippers will wait 5 frames, or 0.08 seconds
    before checking player collision. This feels more balanced without having to 
    rewrite bullet / enemy logic. */
void update_flipper(enemy_typ_ptr enemy, uint32 delta_time)
{
    if (mb_near_walk(enemy, delta_time)) // Get to near rim
    {
        if (enemy->logical_flag)
        {
            // Flipper is about to flip, check for attack
            if (check_player_collision(enemy))
                player_hit(enemy);    
        }        
                
        mb_roll(enemy, delta_time);        
    }
}

void update_spiker(enemy_typ_ptr enemy, uint32 delta_time)
{
    rotate_obj4_z(enemy->obj, 196608);

    if (!enemy->logical_flag) // Payload target not reached
    {
        if (mb_payload(enemy, delta_time))
        {
            vec3f16 end_pos;

            enemy->logical_flag = TRUE; // Payload target reached

             // Drop payload
            end_pos[0] = enemy->obj->world_x;
            end_pos[1] = enemy->obj->world_y;
            end_pos[2] = enemy->obj->world_z;

            activate_spike(end_pos, enemy->corridor_index);

            // Fire missile if there is room
            if (hostile_stats.active_enemies + 1 < MAX_ENEMIES)
                spawn_enemy(MISSILE, enemy->corridor_index, enemy->obj->world_z);
        }
    }
    else                        
    {
        // Run away
        if (mb_far_walk(enemy, delta_time))
            destroy_enemy(enemy, FALSE);
    }   
}

void update_missile(enemy_typ_ptr enemy, uint32 delta_time)
{
    if (enemy->obj->world_z <= LEVEL_ZNEAR)
    {
        if (check_player_collision(enemy))
            player_hit(enemy);
    }

    // rotate_obj4_z(enemy->obj, 229376);

    if (mb_cam_walk(enemy, delta_time)) // Finished
        destroy_enemy(enemy, FALSE);
}   

void update_pulsar(enemy_typ_ptr enemy, uint32 delta_time)
{
    if (enemy->ticks > 0)
        mb_bounce(enemy, delta_time);

    if (enemy->ticks-- < -170) // Reset
    {
        enemy->ticks = rand() % 100 + 200;
        reset_corridor_palette(enemy->corridor_index);
    }
    else 
    {
        if (enemy->ticks < -100) // Shocking corridor
        {
            if (enemy->ticks % 2)
                set_corridor_color(enemy->corridor_index, PULSAR_COLOR);
            else 
                reset_corridor_palette(enemy->corridor_index);                

            // Player collision
            if (player.corridor_index == enemy->corridor_index)
            {
                if (player.obj->world_z == LEVEL_ZNEAR)
                {
                    player_hit(enemy);                        
                    set_corridor_color(enemy->corridor_index, PULSAR_COLOR);
                }
            }
        }
    }
}   

void update_tanker(enemy_typ_ptr enemy, uint32 delta_time)
{
    if (mb_near_walk(enemy, delta_time))
        destroy_enemy(enemy, TRUE);
}  

void update_fuseball(enemy_typ_ptr enemy, uint32 delta_time)
{
    if (enemy->obj->world_z <= LEVEL_ZNEAR)
    {
        mb_side_step(enemy, delta_time); 

        if (check_player_collision(enemy))
            player_hit(enemy);
    }
    else 
    {
        if (enemy->ticks > 0)
        {
            mb_near_walk(enemy, delta_time);
            --enemy->ticks;
        }
        else 
        {
            mb_side_step(enemy, delta_time); 
            mb_near_walk(enemy, delta_time);                  
        }
    }

    rotate_obj4_z(enemy->obj, 229376);
}   

void spawn_enemy(uint32 enemy_type, uint32 corridor_index, int32 world_z)
{    
    uint32 i;
    int32 lut_index;
    enemy_typ_ptr enemy = NULL;
        
    for (i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].state == ES_INACTIVE)
        {
            enemy = &enemies[i];
            break;
        }
    }

    // Ensure we have an enemy slot available.
    if (!enemy)
        return;
    
    // If spiker, ensure we have enough space.
    if (enemy_type == SPIKER)
    {
        if (hostile_stats.active_spikes >= MAX_SPIKES)
            return;
    }

    // OK 

    enemy->enemy_type = enemy_type;
    enemy->frame_index = 0;
    lut_index = enemy_anims[enemy->enemy_type].frame_cycle_lut[enemy->frame_index];

    enemy->obj->polygons[0].ccb->ccb_SourcePtr = (CelData*) enemy_anims[enemy->enemy_type].frames[lut_index]->source;    
    enemy->obj->polygons[0].ccb->ccb_PLUTPtr = (void*) enemy_anims[enemy->enemy_type].frames[lut_index]->plut;

    enemy->corridor_index = corridor_index;
    snap_obj_to_corridor(enemy->obj, &LCONTEXT_LEVEL.obj->polygons[enemy->corridor_index], world_z);

    init_enemy_handlers[enemy->enemy_type](enemy);   

    ++hostile_stats.enemy_counts[enemy->enemy_type];
    ++hostile_stats.active_enemies;

    enemy->state = ES_ACTIVE;
}

uint32 get_enemy_corridor(uint32 enemy_type)
{
    uint32 corridor_index = 0;

    if (enemy_type == SPIKER)
    {
        uint32 i;
        Boolean corridor_found;

        // Find a corridor without a spike or spiker.
        do 
        {
            corridor_index = rand() % LCONTEXT_LEVEL.obj->poly_count;
            corridor_found = TRUE; // Test below

            for (i = 0; i < MAX_SPIKES; i++)
            {
                if (spikes[i].active)
                {
                    if (spikes[i].corridor_index == corridor_index)
                    {
                        // Try again
                        corridor_found = FALSE;
                        break; 
                    }
                }
            }

            if (!corridor_found)
                continue;

            for (i = 0; i < MAX_ENEMIES; i++)
            {
                if (enemies[i].state == ES_ACTIVE)
                {
                    if (enemies[i].corridor_index == corridor_index)
                    {
                        // Try again
                        corridor_found = FALSE;
                        break;
                    }
                }
            }
        } while(!corridor_found);
    }    
    else 
    {
        corridor_index = rand() % LCONTEXT_LEVEL.obj->poly_count;
    }

    return(corridor_index);
}

void load_enemy_anims(void)
{
    uint32 i, j;
    CCB *temp_ccb;
    rez_envelope_typ rez_envelope;    
    char buffer[50];
    
    enemy_anims[FLIPPER].frame_count = 1;
    enemy_anims[TANKER].frame_count = 3;
    enemy_anims[SPIKER].frame_count = 1;
    enemy_anims[MISSILE].frame_count = 3;
    enemy_anims[FUSEBALL].frame_count = 3;
    enemy_anims[PULSAR].frame_count = 1;
    enemy_anims[FTANKER].frame_count = 3;
    enemy_anims[PTANKER].frame_count = 3;

    for (i = 0; i < MAX_ENEMY_TYPES; i++)
    {
        enemy_anims[i].frames = (cel8_data_typ_ptr*) AllocMem(sizeof(cel8_data_typ) * enemy_anims[i].frame_count, MEMTYPE_DRAM);

        for (j = 0; j < enemy_anims[i].frame_count; j++)
        {
            sprintf(buffer, "Assets/Graphics/%s/%s%d.cel", enemy_names[i], enemy_names[i], j+1);

            load_resource(buffer, REZ_CEL, &rez_envelope);            
            temp_ccb = (CCB*) rez_envelope.data;

            enemy_anims[i].frames[j] = cel_3do_to_cel8_data(temp_ccb);

            unload_resource(&rez_envelope, REZ_CEL);      
        }

        memcpy((void*)enemy_anims[i].frame_cycle_lut, (void*)frame_cycle_luts[i], sizeof(int32) * MAX_ANIM_CYCLE);       
    }

    // Anim used when super zapper is used

    zapped_anim.frame_count = 3;    
    zapped_anim.frames = (cel8_data_typ_ptr*) AllocMem(sizeof(cel8_data_typ) * zapped_anim.frame_count, MEMTYPE_DRAM);

    for (i = 0; i < zapped_anim.frame_count; i++)
    {
        sprintf(buffer, "Assets/Graphics/Effects/Zapped%d.cel", i+1);
        load_resource(buffer, REZ_CEL, &rez_envelope);
        temp_ccb = (CCB*) rez_envelope.data;
        zapped_anim.frames[i] = cel_3do_to_cel8_data(temp_ccb);
        unload_resource(&rez_envelope, REZ_CEL);
    }

    memset((void*)zapped_anim.frame_cycle_lut, -1, sizeof(int32) * MAX_ANIM_CYCLE);       

    zapped_anim.frame_cycle_lut[0] = 2;
    zapped_anim.frame_cycle_lut[1] = 1;
    zapped_anim.frame_cycle_lut[2] = 0;
    zapped_anim.frame_cycle_lut[3] = 1;
    zapped_anim.frame_cycle_lut[4] = 2; 
    zapped_anim.frame_cycle_lut[5] = -1; 
}

void clear_hostiles(void)
{
    clear_enemies();
    memset((void*)spikes, 0, sizeof(spike_typ) * MAX_SPIKES);
    memset((void*)&hostile_stats, 0, sizeof(hostile_stats));
}

/* *************************************************************************************** */
/* ------------------------------- MOVEMENT BEHAVIOR CODE -------------------------------- */
/* *************************************************************************************** */

// Walks to near rim, then walks to far rim, and back
Boolean mb_bounce(enemy_typ_ptr enemy, uint32 delta_time)
{
    if (enemy->logical_flag) // Move towards near
    {
        if (mb_near_walk(enemy, delta_time)) // Flip
            enemy->logical_flag = 1 - enemy->logical_flag;
    }
    else // Move towards far
    {
        if (mb_far_walk(enemy, delta_time)) // Flip
            enemy->logical_flag = 1 - enemy->logical_flag;
    }

    return(FALSE); // Do forever
}

// Walks to camera
Boolean mb_cam_walk(enemy_typ_ptr enemy, uint32 delta_time)
{
    enemy->obj->world_z -= (delta_time << enemy->speed);    

    if (enemy->obj->world_z <= (camera.world_z + 131072))
        return(TRUE);
    
    return(FALSE);
}

// Walks to rim near
Boolean mb_near_walk(enemy_typ_ptr enemy, uint32 delta_time)
{
    enemy->obj->world_z -= (delta_time << enemy->speed);    

    if (enemy->obj->world_z <= LEVEL_ZNEAR)
    {
        enemy->obj->world_z = LEVEL_ZNEAR;
        return(TRUE);
    }
    
    return(FALSE);
}

// Walks to rim far
Boolean mb_far_walk(enemy_typ_ptr enemy, uint32 delta_time)
{
    enemy->obj->world_z += (delta_time << enemy->speed);    

    if (enemy->obj->world_z >= LEVEL_ZFAR)
    {
        enemy->obj->world_z = LEVEL_ZFAR;
        return(TRUE);
    }
    
    return(FALSE);
}

// Walks to random point, then returns to far rim
Boolean mb_payload(enemy_typ_ptr enemy, uint32 delta_time)
{
    enemy->obj->world_z -= (delta_time << enemy->speed);
    
    if (--enemy->ticks < 0) // Target reached
    {
        return(TRUE);
    }
    else if (enemy->obj->world_z < LEVEL_ZNEAR)
    {
        enemy->obj->world_z = LEVEL_ZNEAR; // Snap
        return(TRUE);
    }
    
    return(FALSE);
}

Boolean mb_roll(enemy_typ_ptr enemy, uint32 delta_time)
{
    int32 dp;
    int32 corridor_index;
    vec3f16 up;
    vertex_typ_ptr vt;
    polygon_typ_ptr poly;
    poly_props_typ props;

    if (enemy->logical_flag) // Get next step
    {
        if (enemy->traversal_order == CW)
        {
            corridor_index = enemy->corridor_index - 1;

            if (corridor_index < 0)
            {
                corridor_index = enemy->corridor_index + 1;
                enemy->traversal_order = CCW;
            }                       
        }
        else // CCW
        {
            corridor_index = enemy->corridor_index + 1;

            if (corridor_index >= LCONTEXT_LEVEL.obj->poly_count)
            {
                corridor_index = enemy->corridor_index - 1;
                enemy->traversal_order = CW;                 
            }         
        }

        // Target corridor poly
        poly = &LCONTEXT_LEVEL.obj->polygons[corridor_index];

        enemy->vector[0] = poly->normal[0];
        enemy->vector[1] = poly->normal[1];
        enemy->vector[2] = 0;
        enemy->next_corridor = corridor_index;
        enemy->logical_flag = FALSE;
    }

    props = get_corridor_props(&LCONTEXT_LEVEL.obj->polygons[enemy->corridor_index]);

    if (enemy->traversal_order == CW)
    {            
        rotate_obj_pivot_z(enemy->obj, props.near_edge[0], -196608);
    }
    else // CCW
    {
        rotate_obj_pivot_z(enemy->obj, props.near_edge[2], 196608);
    }

    // Check if enemy rotated onto target corridor
    
    // Calc up vector using vertices 4 and 1
    vt = enemy->obj->vertex_def.vertices;
    poly = &enemy->obj->polygons[0];

    up[0] = vt[poly->vertex_lut[3]].vertex[VERTEX_X] - vt[poly->vertex_lut[0]].vertex[VERTEX_X];
    up[1] = vt[poly->vertex_lut[3]].vertex[VERTEX_Y] - vt[poly->vertex_lut[0]].vertex[VERTEX_Y];
    up[2] = 0;

    normalize_vector(up);

    dp = Dot3_F16(up, enemy->vector);

    // .98 - 1
    if ( dp <= -64880 && dp >= -ONE_F16)
    {
        // enemy->logical_flag = TRUE;
        enemy->corridor_index = enemy->next_corridor;
        snap_obj_to_corridor(enemy->obj, &LCONTEXT_LEVEL.obj->polygons[enemy->corridor_index], LEVEL_ZNEAR);   
        enemy->logical_flag = TRUE;
        return(TRUE);        
    }

    return(FALSE);
}

Boolean mb_side_step(enemy_typ_ptr enemy, uint32 delta_time)
{
    poly_props_typ props;
    int32 speed;
    int32 dist;
    int32 next_corridor;
    vec3f16 p1;
    int32 *vertex;
    Boolean ret = FALSE;

    if (enemy->logical_flag)
    {
        // Get travel vector based on current corridor
        props = get_corridor_props(&LCONTEXT_LEVEL.obj->polygons[enemy->corridor_index]);
        
        if (enemy->traversal_order == CW)
        {
            enemy->vector[X] = props.near_edge[0][X] - props.near_edge[2][X];
            enemy->vector[Y] = props.near_edge[0][Y] - props.near_edge[2][Y];
        }
        else // CCW
        {
            enemy->vector[X] = props.near_edge[2][X] - props.near_edge[0][X];
            enemy->vector[Y] = props.near_edge[2][Y] - props.near_edge[0][Y];
        }

        enemy->vector[Z] = 0;

        normalize_vector3(enemy->vector);

        enemy->logical_flag = FALSE;
    }

    // Has enemy crossed over onto next corridor?

    props = get_corridor_props(&LCONTEXT_LEVEL.obj->polygons[enemy->corridor_index]);

    p1[0] = enemy->obj->world_x;
    p1[1] = enemy->obj->world_y;
    p1[2] = 0;

    if (enemy->traversal_order == CW)
        vertex = props.near_edge[0];
    else // CCW
        vertex = props.near_edge[2];

    dist = get_squared_dist(p1, vertex);

    if (dist <= 1048612)
    {        
        enemy->obj->world_x = vertex[0];
        enemy->obj->world_y = vertex[1];
        enemy->logical_flag = TRUE;

        // Update corridor index
        if (enemy->traversal_order == CW)
        {
            next_corridor = enemy->corridor_index - 1;

            if (next_corridor < 0)
                enemy->traversal_order = CCW;
            else 
                enemy->corridor_index = next_corridor;
        }
        else // CCW
        {
            next_corridor = enemy->corridor_index + 1;

            if (next_corridor >= LCONTEXT_LEVEL.obj->poly_count)
                enemy->traversal_order = CW;
            else 
                enemy->corridor_index = next_corridor;
        }

        ret = TRUE;
    }
    else 
    {
        speed = (delta_time << enemy->speed) >> 3;
        enemy->obj->world_x += MulSF16(enemy->vector[X], speed);
        enemy->obj->world_y += MulSF16(enemy->vector[Y], speed);
    }      

    return(ret);
}

uint32 get_spike_count(void)
{
    return (hostile_stats.active_spikes);
}

void zap_enemies(void)
{
    enemy_typ_ptr enemy_it;
    uint32 i;

    enemy_it = enemies;
    i = MAX_ENEMIES;

    while (i--)
    {
        if (enemy_it->state == ES_ACTIVE)
        {
            enemy_it->state = ES_DESTROY;
            enemy_it->frame_index = 0;
            reset_simple_timer(&anim_timer, time_io);
        }
         
        ++enemy_it;
    }
}