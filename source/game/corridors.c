#include "game_globals.h"

poly_props_typ get_corridor_props(polygon_typ_ptr corridor)
{
    poly_props_typ props;
    vertex_typ_ptr verts;
    uint32 i, j;
    uint32 edge_lut[2];

    verts = corridor->parent->vertex_def.vertices;

    // Find near edge 
    for (i = 0, j = 0; i < 4; i++)
    {
        if (verts[corridor->vertex_lut[i]].vertex[VERTEX_Z] < 0)
            edge_lut[j++] = corridor->vertex_lut[i]; // Found one
    }

    #if DEBUG_MODE
        if (j != 2)
            printf("Error - could not locate corridor edge.\n");
    #endif 

    // Point 1
    props.near_edge[0][VERTEX_X] = verts[edge_lut[0]].vertex[VERTEX_X];
    props.near_edge[0][VERTEX_Y] = verts[edge_lut[0]].vertex[VERTEX_Y];
    props.near_edge[0][VERTEX_Z] = verts[edge_lut[0]].vertex[VERTEX_Z];

    // Point 2
    props.near_edge[2][VERTEX_X] = verts[edge_lut[1]].vertex[VERTEX_X];
    props.near_edge[2][VERTEX_Y] = verts[edge_lut[1]].vertex[VERTEX_Y];
    props.near_edge[2][VERTEX_Z] = verts[edge_lut[1]].vertex[VERTEX_Z];

    // Median
    props.near_edge[1][VERTEX_X] = (props.near_edge[0][VERTEX_X] + props.near_edge[2][VERTEX_X]) >> 1;
    props.near_edge[1][VERTEX_Y] = (props.near_edge[0][VERTEX_Y] + props.near_edge[2][VERTEX_Y]) >> 1;
    props.near_edge[1][VERTEX_Z] = (props.near_edge[0][VERTEX_Z] + props.near_edge[2][VERTEX_Z]) >> 1;

    return(props);
}

void snap_obj_to_corridor(object_typ_ptr obj, polygon_typ_ptr corridor, int32 z)
{
    vec3f16 rot_angles;
    poly_props_typ props = get_corridor_props(corridor);

    // Set position
    obj->world_x = props.near_edge[1][VERTEX_X]; // Median
    obj->world_y = props.near_edge[1][VERTEX_Y];
    obj->world_z = z;

    // Set rotation
    
    copy_vertex_def(&obj->vertex_def, &obj->vertex_def_copy); // Reset verts first

    rot_angles[ANGLE_Z] = get_corridor_angle(corridor);

    if (rot_angles[ANGLE_Z] != 0)
    {
        rot_angles[ANGLE_X] = 0;
        rot_angles[ANGLE_Y] = 0;
        rot_angles[ANGLE_Z] = -rot_angles[ANGLE_Z];
        rotate_obj(obj, rot_angles);
    }
}

int32 get_corridor_angle(polygon_typ_ptr corridor)
{
    int32 delta_x, delta_y;
    int32 angle;
    poly_props_typ props;

    props = get_corridor_props(corridor);

    delta_x = props.near_edge[1][VERTEX_X] - props.near_edge[0][VERTEX_X];
    delta_y = props.near_edge[1][VERTEX_Y] - props.near_edge[0][VERTEX_Y];

    // Avoid divide by zero. Is plane parallel to X axis?
    if (delta_y == 0)
        angle = (corridor->normal[1] > 0) ? 0 : 8388608;        
    else 
        angle = Atan2F16(delta_x, delta_y);

    return(angle);
}

void reset_corridor(uint32 corridor_index)
{
    reset_corridor_palette(corridor_index);
}

void reset_corridors(void)
{
    uint32 i;
    object_typ_ptr obj = LCONTEXT_LEVEL.obj;

    for (i = 0; i < obj->poly_count; i++)
        reset_corridor(i);
}

void reset_corridor_palette(uint32 corridor_index)
{
    memcpy((void*)LCONTEXT_LEVEL.obj->polygons[corridor_index].ccb->ccb_PLUTPtr, 
        (void*)LCONTEXT_LEVEL.palettes[corridor_index], PALETTE_SIZE_BYTES);
}

void set_corridor_color(uint32 corridor_index, uint16 color)
{
    set_cel_pal_to_color(LCONTEXT_LEVEL.obj->polygons[corridor_index].ccb, color);
}