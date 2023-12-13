// My includes
#include "game_globals.h"

player_typ player;
uint32 player_poly_order[4] = {1, 0, 2, 3};
CCB *lives[MAX_LIVES];
int32 player_move_vel;

void init_player(void)
{
    uint32 i;
    polygon_typ_ptr poly;
    vertex_typ_ptr verts;
    rez_envelope_typ rez_envelope;

    player.obj = load_obj("Assets/Entities/Player");

    // Player will have 4 polygons

    for (i = 0; i < player.obj->poly_count; i++)
    {
        player.obj->polygons[i].ccb = create_coded_colored_cel8(16, 16, MakeRGB15(31,31,0));
        FastMapCelInit(player.obj->polygons[i].ccb);
        calc_poly_normal(&player.obj->polygons[i]);
    }

    scale_obj(player.obj, 26215);

    clone_vertex_def(&player.obj->vertex_def_copy, &player.obj->vertex_def); // Prestine
    clone_vertex_def(&player.left_weight_vdef, &player.obj->vertex_def);
    clone_vertex_def(&player.right_weight_vdef, &player.obj->vertex_def);

    poly = &player.obj->polygons[ player_poly_order[1] ];

    verts = player.left_weight_vdef.vertices;
    trans_vertex_xy_by(verts[poly->vertex_lut[0]].vertex, 10000, -8388608);
    trans_vertex_xy_by(verts[poly->vertex_lut[1]].vertex, 10000, -8388608);

    verts = player.right_weight_vdef.vertices;
    trans_vertex_xy_by(verts[poly->vertex_lut[0]].vertex, 10000, 0);
    trans_vertex_xy_by(verts[poly->vertex_lut[1]].vertex, 10000, 0);

    player.active_vdef = &player.obj->vertex_def_copy; // Prestine
    
    player.obj->bsphere_radius = calc_bsphere_radius(player.obj);

    // HUD
    load_resource("Assets/Graphics/UI/Life.cel", REZ_CEL, &rez_envelope);

    for (i = 0; i < MAX_LIVES; i++)
    {
        if (i == 0)
        {
            lives[i] = (CCB*) rez_envelope.data;
            lives[i]->ccb_Flags |= CCB_LDPLUT;
            lives[i]->ccb_Flags &= ~CCB_BGND;          
        }
        else 
        {
            lives[i] = create_coded_cel8(18, 18, FALSE);
            lives[i]->ccb_SourcePtr = lives[0]->ccb_SourcePtr;
            lives[i]->ccb_PLUTPtr = lives[0]->ccb_PLUTPtr;          
        }

        lives[i]->ccb_XPos = (292 - (20 * i)) << FRACBITS_16;
        lives[i]->ccb_YPos = 6 << FRACBITS_16;

        if (i > 0)
            LINK_CEL(lives[i-1], lives[i]);       
    }
}

void reset_player(void)
{
    player.corridor_index = rand() % LCONTEXT_LEVEL.obj->poly_count;
    player.velocity_z = 0;
    player_move_vel = 0;
    player.active_vdef = &player.obj->vertex_def_copy;
    player.active = TRUE;
    snap_player(X_AXIS|Y_AXIS|Z_AXIS);
}

void snap_player(uint32 axis_flags)
{
    poly_props_typ props;
    polygon_typ_ptr poly;
    vec3f16 angles;

    poly = &LCONTEXT_LEVEL.obj->polygons[player.corridor_index];
    props = get_corridor_props(poly);

    // Snap

    if (axis_flags & X_AXIS)
    {
        player.obj->world_x = props.near_edge[1][VERTEX_X];
        set_cam_target_x(player.obj->world_x);       
    }

    // Update camera target to median (smoother look)

    if (axis_flags & Y_AXIS)
    {
        player.obj->world_y = props.near_edge[1][VERTEX_Y];
        set_cam_target_y(player.obj->world_y);
    }

    if (axis_flags & Z_AXIS)
        player.obj->world_z = LEVEL_ZNEAR;
     
    // Reset player rotation
    copy_vertex_def(&player.obj->vertex_def, player.active_vdef);

    // Set rotation
    angles[2] = get_corridor_angle(poly);

    if (angles[2] != 0)
    {
        angles[0] = 0;
        angles[1] = 0;
        angles[2] = -angles[2];
        rotate_obj(player.obj, angles);
    }

    player.rotation_angle = angles[2];
}

void trans_vertex_xy_by(vec3f16 vertex, int32 amount, int32 angle)
{
    int32 step_x = MulSF16(amount, CosF16(angle));
    int32 step_y = MulSF16(amount, SinF16(angle)); 

    vertex[0] += step_x;
    vertex[1] += step_y;   
}

Boolean check_player_collision(enemy_typ_ptr enemy)
{
    if (is_colliding(enemy->obj, player.obj, TRUE))
        return(TRUE);            

    return(FALSE);
}

void player_hit(enemy_typ_ptr enemy)
{
    #if !GOD_MODE
        if (enemy)
        {
            // Hit by animate object
            if (enemy->enemy_type == FLIPPER)
            {
                killshot_enemy = enemy;                
                play_sample(*sfx[SFX_WHOA], 500, DEFAULT_AUDIO_AMPLITUDE);
                set_play_handler(PLAY_HANDLER_GRABBED);
            }
            else 
            {
                set_play_handler(PLAY_HANDLER_HIT);
            }            
        }
        else 
        {
            // Hit by inanimate object
            set_play_handler(PLAY_HANDLER_HIT);
        }

        player.active = FALSE;
    #endif
}
