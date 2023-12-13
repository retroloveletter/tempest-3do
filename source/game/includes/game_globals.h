#ifndef GAME_GLOBALS_H
#define GAME_GLOBALS_H


// 3DO includes
#include "stdio.h"
#include "string.h"
#include "mem.h"
#include "celutils.h"
#include "event.h"
#include "fontlib.h"
#include "textlib.h"
#include "task.h"
#include "kernel.h"
#include "timerutils.h"
#include "operamath.h"

// My includes
#include "app_globals.h"
#include "threed.h"
#include "resources.h"
#include "cel_helper.h"
#include "debugger.h"
#include "audi.h"
#include "levels.h"
#include "stimers.h"
#include "maths.h"

// Settings bit masks
#define GAME_SETTINGS_CLEAR 0
#define GAME_SET_MUSIC_MASK 1
#define GAME_SET_HARD_MASK 2
#define SET_GAME_SETTING(setting_mask) (game_settings |= setting_mask)
#define UNSET_GAME_SETTING(setting_mask) (game_settings &= ~setting_mask)
#define TOGGLE_GAME_SETTING(setting_mask) (game_settings ^= setting_mask)
// General
#define GOD_MODE 0
#define SHOW_LEVEL_NORMALS 0    // Only use this for debugging / testing
#define LEVEL_ZNEAR -262144
#define LEVEL_ZFAR 262144
#define MAX_BULLETS 5           // Max on screen at a time
#define BULLET_SPEED 851968
#define MAX_ENEMIES 6           // Max at a time
#define CAM_BASE_Z_OFFSET (LEVEL_ZNEAR - 163840)
#define CAM_NEAR 32768
#define PALETTE_SIZE_BYTES 64
#define MAX_SPIKES 5
#define MAX_STARS 10
#define LEVEL_SKIP 0            // Only use this for debugging / testing
// Player 
#define MAX_LIVES 5
#define BASE_MOVE_SPEED 70000
#define MOVE_VEL_INC 5000
#define MAX_MOVE_VEL 180000
#define JUMP_FORCE -1703936     // Force applied to z velocity when jumping
#define GRAVITY_Z 98304         // Force pulling player back after jumping
// Angles 
#define ANG_256 16777216
#define ANG_128 8388608
// Flags
#define X_AXIS 1
#define Y_AXIS 2
#define Z_AXIS 4
// Save typing
#define SCONTEXT_BITMAP sc->sc_Bitmaps[sc->sc_CurrentScreen]->bm_Buffer
#define SCONTEXT_BITEM sc->sc_BitmapItems[sc->sc_CurrentScreen]

// Play logic types
enum 
{
    PLAY_HANDLER_GAME = 0,  // Gameplay
    PLAY_HANDLER_INTRO,     // New level or level reset transition
    PLAY_HANDLER_HIT,       // Player ship destroyed
    PLAY_HANDLER_SWITCH,    // Old level transition out
    PLAY_HANDLER_END,       // Won the game
    PLAY_HANDLER_OVER,      // Game over
    PLAY_HANDLER_GRABBED,   // Flipper enemy took player
    PLAY_HANDLER_MAX
};

enum 
{
    SFX_ZAP = 0,
    SFX_BOOM,
    SFX_PULSE,
    SFX_WHOA,
    SFX_CLEAR,
    SFX_VOICE,
    SFX_MAX
};

// Winding order types
enum 
{
    CW = 0,
    CCW
};

enum ENEMY_TYPE
{
    MISSILE = 0,
    FLIPPER,        // Gets to rim and chases player  
    TANKER,         // Turns into Flippers on rim
    SPIKER,         // Drops spikes on corridors
    FUSEBALL,
    PULSAR,
    FTANKER,
    PTANKER,
    MAX_ENEMY_TYPES
};

typedef struct 
{
    uint32 previous_time;
    uint32 delay;
} time_delta_typ;

typedef enum 
{
    ES_ACTIVE = 0,
    ES_DESTROY,
    ES_INACTIVE,
    ES_STATE_MAX
} ENEMY_STATE;

typedef struct spike_typ
{
    Boolean active;
    uint32 corridor_index;
    vec3f16 begin_pos;
    vec3f16 end_pos;
} spike_typ, *spike_typ_ptr;

typedef struct poly_props_typ 
{
    vec3f16 near_edge[3]; // A, Median, B
} poly_props_typ, *poly_props_typ_ptr;

typedef struct enemy_typ 
{
    // Boolean active;
    ENEMY_STATE state;
    object_typ_ptr obj;
    int32 health;
    uint32 corridor_index;
    uint32 speed;
    uint32 enemy_type;
    uint32 frame_index;
    
    // ---- Sharable props ----

    // Used by Fuseballs and Flippers
    uint32 traversal_order;   
    vec3f16 vector;
    // Used by Spikers, Fuseballs, and Flippers
    Boolean logical_flag;
    // Used by Flippers
    uint32 next_corridor;
    // Used by Pulsars, Spikers, and Fuseballs
    int32 ticks;
} enemy_typ, *enemy_typ_ptr;

typedef struct player_typ 
{
    object_typ_ptr obj;
    Boolean active;
    int32 velocity_z;
    uint32 lives;
    int32 corridor_index;
    int32 rotation_angle;
    vertex_def_typ left_weight_vdef;
    vertex_def_typ right_weight_vdef;
    vertex_def_typ_ptr active_vdef;
} player_typ, *player_typ_ptr;

typedef struct bullet_typ 
{
    Boolean active;
    uint32 corridor_index;
    object_typ_ptr obj;
} bullet_typ, *bullet_typ_ptr;

typedef struct star_typ 
{
    CCB *ccb;
    vec3f16 cam;
    vec3f16 world;
    vec3f16 reset_world;
} star_typ;

#if DEBUG_MODE
    extern MemInfo vram_info;
    extern MemInfo dram_info;
#endif

// gs_play.c
extern void set_play_handler(uint32 index);
extern Item *sfx[SFX_MAX];
extern star_typ stars[MAX_STARS];
extern void init_stars(void);
extern void update_stars(void);
extern void reset_star(star_typ *sptr);
extern void showcase_ship(void);
extern uint32 game_settings;

// corridors.c
extern poly_props_typ get_corridor_props(polygon_typ_ptr corridor);
extern int32 get_corridor_angle(polygon_typ_ptr corridor);
extern void snap_obj_to_corridor(object_typ_ptr obj, polygon_typ_ptr corridor, int32 z);
extern void reset_corridor(uint32 corridor_index);
extern void reset_corridors(void);
extern void reset_corridor_palette(uint32 corridor_index);
extern void set_corridor_color(uint32 corridor_index, uint16 color);

// enemies.c
extern simple_timer_typ enemy_spawn_timer;
extern enemy_typ enemies[MAX_ENEMIES];
extern enemy_typ_ptr killshot_enemy; // Holds enemy that killed player
extern spike_typ spikes[MAX_SPIKES];
extern void init_enemies(void);
extern void update_enemies(uint32 delta_time);
extern void add_enemies(void);
extern void spawn_next_enemy(void);
extern void draw_spikes(void);
extern void damage_spike(spike_typ_ptr spike);
extern void clear_enemies(void);
extern void destroy_enemy(enemy_typ_ptr enemy, Boolean payload);
extern void clear_hostiles(void);
extern uint32 get_spike_count(void);
extern void zap_enemies(void);

// player.c
extern player_typ player;
extern uint32 player_poly_order[4];
extern CCB *lives[MAX_LIVES];
extern int32 player_move_vel;
extern void init_player(void);
extern void reset_player(void);
extern void snap_player(uint32 axis_flags);
extern void trans_vertex_xy_by(vec3f16 vertex, int32 amount, int32 angle);
extern Boolean check_player_collision(enemy_typ_ptr enemy);
extern void player_hit(enemy_typ_ptr enemy);

// camera.c
extern void zero_camera(void);
extern void reset_play_camera(void);
extern void set_cam_target_x(int32 x);
extern void set_cam_target_y(int32 y);
extern void update_cam(uint32 delta_time);

#endif // GAME_GLOBALS_H