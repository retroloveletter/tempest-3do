#ifndef LEVELS_H
#define LEVELS_H

// 3DO includes
#include "types.h"
#include "threed.h"
#include "string.h"

// Save some typing
#define LCONTEXT_LEVEL lc.levels[lc.level_index]
#define MAX_LEVEL_POLYS 20
#define MAX_LEVELS 99
// Only works up to level 20
#define STARTING_LEVEL 1

typedef struct level_typ 
{
    uint32 number;
    object_typ_ptr obj;
    uint16 palettes[MAX_LEVEL_POLYS][32];
    uint32 wireframe_color;
    Boolean wrap;
} level_typ, *level_typ_ptr;

typedef struct level_context_typ 
{
    level_typ levels[2];
    uint32 level_index;
} level_context_typ, *level_context_typ_ptr;

extern level_context_typ lc;
extern CCB *level_template;
extern uint32 current_level;
extern uint32 level_counter;

void unload_level(level_typ_ptr level);
void init_level_manager(void);
void cycle_levels(void);
void reset_level_manager(void);
void reset_level_transrot(void);

#endif // LEVELS_H