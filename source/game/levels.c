#include "game_globals.h"

#define MAX_LEVEL_FILES 20

level_context_typ lc;
uint32 level_counter;
uint32 current_level;

static int32 ready_sig;
static Item load_next_sig;
static Item level_manager_item;
static Item parent_task_item;
static uint32 level_slot;
static Boolean first_load;

static uint16 even_odd_colors[4];

static void apply_even_odd_pal(level_typ_ptr level)
{
    uint32 i, starting_index, color_index;
    
    starting_index = rand() % 2; // 0 - 1

    if (starting_index == 1)
        starting_index = 2; // Purples
    // Else, blues

    for (i = 0; i < level->obj->poly_count; i++)
    {        
        color_index = (i % 2) + starting_index;

        #if DEBUG_MODE 
            if (color_index >= 4)
                printf("Error - Invalid odd/even color index.\n");
        #endif 

        set_cel_pal_to_color(level->obj->polygons[i].ccb, even_odd_colors[color_index]);
        memcpy((void*)level->palettes[i], (void*)level->obj->polygons[i].ccb->ccb_PLUTPtr, PALETTE_SIZE_BYTES);
    }

    level->wireframe_color = (uint32) even_odd_colors[starting_index + 1];
}

static void init_level(uint32 level_index)
{
    uint32 i;
    level_typ_ptr level = &lc.levels[level_index];

    for (i = 0; i < level->obj->poly_count; i++)
    {
        // Create poly cel
        level->obj->polygons[i].ccb = create_coded_colored_cel8(16, 16, 0);
        calc_poly_normal(&level->obj->polygons[i]);
        FastMapCelInit(level->obj->polygons[i].ccb);
    }   

    // Color palette logic here
    apply_even_odd_pal(level);
}

static void load_level(uint32 slot)
{
    char file_path[28];
    level_typ_ptr level = &lc.levels[slot];
    level->number = level_counter;
   
    sprintf(file_path, "Assets/Levels/Level%d", level_counter);

    level->obj = load_obj(file_path);
    level->wrap = FALSE;
    level->obj->world_x = 0;
    level->obj->world_y = 0;
    level->obj->world_z = 0;

    #if DEBUG_MODE 
        if (level->obj->poly_count > MAX_LEVEL_POLYS)
            printf("Error - Level poly count exceeded limit.\n");
    #endif

    if (level_counter == 5   || level_counter == 7    || level_counter == 10   ||
        level_counter == 13  || level_counter == 14   || level_counter == 18   ||
        level_counter == 20)
    {
        level->wrap = TRUE;   
    }

    // Keep prestine version of vertices for reset
    clone_vertex_def(&level->obj->vertex_def_copy, &level->obj->vertex_def);   
}

void unload_level(level_typ_ptr level)
{
    unload_obj(level->obj);
}

// Separate thread
void level_manager(void)
{
    int32 sigs;

    load_next_sig = AllocSignal(0);

    OpenMathFolio();

    SendSignal(parent_task_item, ready_sig);

    // Run forever
    while (1)
    {
        sigs = WaitSignal(load_next_sig);

        #if DEBUG_MODE
            if (sigs < 0)
                printf("Error - WaitSignal < 0\n");
        #endif
    
        if (sigs & load_next_sig)
        {
            // Cleanup check
            if (lc.levels[level_slot].obj)
            {
                #if 0
                    #if DEBUG_MODE 
                        dump_mem_info("vram before level uloading.", MEMTYPE_VRAM);
                        dump_mem_info("dram before level uloading.", MEMTYPE_DRAM);
                    #endif
                #endif

                unload_level(&lc.levels[level_slot]);

                #if 0
                #if DEBUG_MODE 
                    dump_mem_info("vram after level uloading.", MEMTYPE_VRAM);
                    dump_mem_info("dram after level uloading.", MEMTYPE_DRAM);
                #endif
                #endif
            }            

            load_level(level_slot);
            init_level(level_slot);

            // Sync first two loads
            if (first_load)
                SendSignal(parent_task_item, ready_sig);
            
            ++level_counter;

            if (level_counter > MAX_LEVEL_FILES)
            {
                #if DEBUG_MODE 
                    printf("Resetting level counter.\n");
                #endif

                level_counter = STARTING_LEVEL;
            }       
        }
    }
}

void cycle_levels(void)
{    
    if (first_load)
    {
        // Load first two levels synchronously
        level_slot = 0;
        SendSignal(level_manager_item, load_next_sig);
        WaitSignal(ready_sig);

        level_slot = 1;
        SendSignal(level_manager_item, load_next_sig);
        WaitSignal(ready_sig);

        lc.level_index = 0;
        first_load = FALSE;        
    }
    else
    {
        lc.level_index = 1 - lc.level_index;

        #if DEBUG_MODE 
            if (level_counter > MAX_LEVEL_FILES)
            {
                printf("Error - Invalid level file number %d.\n", level_counter);
                return;
            }
        #endif
      
        // Load next level asynchronously
        level_slot = 1 - level_slot;
        SendSignal(level_manager_item, load_next_sig);
    }
}

void init_level_manager(void)
{
    // reset_level_manager();
    
    ready_sig = AllocSignal(0);
    parent_task_item = CURRENTTASK->t.n_Item;

    // Set up various color palettes
    
    even_odd_colors[0] = MakeRGB15(12, 5, 12);
    even_odd_colors[1] = MakeRGB15(13, 6, 17);

    even_odd_colors[2] = MakeRGB15(0, 0, 8);
    even_odd_colors[3] = MakeRGB15(0, 0, 14);
    
    level_manager_item = CreateThread("level_manager", CURRENTTASK->t.n_Priority, level_manager, 2048);
    
    WaitSignal(ready_sig);
}

void reset_level_manager(void)
{
    memset((void*) &lc, 0, sizeof(level_context_typ));        
    level_counter = STARTING_LEVEL;
    current_level = STARTING_LEVEL;
    first_load = TRUE;
}

void reset_level_transrot(void)
{
    vec3f16 angles;

    LCONTEXT_LEVEL.obj->world_x = LCONTEXT_LEVEL.obj->world_y = 0;
    LCONTEXT_LEVEL.obj->world_z = 130 << FRACBITS_16;

    angles[0] = 0;
    angles[1] = -4194304;
    angles[2] = 0;
    
    rotate_obj(LCONTEXT_LEVEL.obj, angles);
}