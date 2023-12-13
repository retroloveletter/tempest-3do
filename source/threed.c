#include "threed.h"
#include "maths.h"
#include "resources.h"
#include "cel_helper.h"

// 3DO includes
#include "stdio.h"
#include "string.h"
#include "mem.h"
#include "celutils.h"

/* *************************************************************************************** */
/* ================================ CONSTANTS / TYPES ==================================== */
/* *************************************************************************************** */

#define VIEW_DIST_SHIFT 8       // 2 ^ 8, or 256 view distance
#define VIEW_DIST_F16 16777216
#define MAX_POLY_RASTER 120
// #define CAM_Z_TO_LUT(z) (((z >> 4) << 11) >> FRACBITS_16) // z / 16 * 2048
#define CAM_Z_TO_LUT(z) ((z << 7) >> FRACBITS_16) // z / 16 * 2048
#define Z_LUT_SIZE 2048

/* *************************************************************************************** */
/* ===================================== GLOBALS ========================================= */
/* *************************************************************************************** */

camera_typ camera;

/* *************************************************************************************** */
/* ================================== PRIVATE VARS ======================================= */
/* *************************************************************************************** */

static polygon_typ_ptr poly_list[MAX_POLY_RASTER];
static uint32 poly_list_size = 0;

// This is very specific to level logic
static int32 inv_depth_table[Z_LUT_SIZE];

/* *************************************************************************************** */
/* ========================== PUBLIC FUNCTION DEFINITIONS ================================ */
/* *************************************************************************************** */

void poly_to_world_cam(polygon_typ_ptr poly)
{
    uint32 i;
    vertex_typ_ptr vtyp;

    for (i = 0; i < 4; i++)
    {
        // World
        vtyp = &poly->parent->vertex_def.vertices[poly->vertex_lut[i]];
        poly->world[i][0] = vtyp->vertex[VERTEX_X] + poly->parent->world_x;
        poly->world[i][1] = vtyp->vertex[VERTEX_Y] + poly->parent->world_y;
        poly->world[i][2] = vtyp->vertex[VERTEX_Z] + poly->parent->world_z;

        // Cam
        poly->camera[i][0] = poly->world[i][0] - camera.world_x;
        poly->camera[i][1] = poly->world[i][1] - camera.world_y;
        poly->camera[i][2] = poly->world[i][2] - camera.world_z;
    }    

    poly->avgz = (poly->camera[0][2] + poly->camera[1][2] + poly->camera[2][2] + poly->camera[3][2]) >> 2;
}

Boolean poly_to_world_cam_clip(polygon_typ_ptr poly, int32 near)
{
    uint32 i, j;
    vertex_typ_ptr vtyp;

    for (i = 0, j = 0; i < 4; i++)
    {
         // World
        vtyp = &poly->parent->vertex_def.vertices[poly->vertex_lut[i]];
        poly->world[i][VERTEX_X] = vtyp->vertex[VERTEX_X] + poly->parent->world_x;
        poly->world[i][VERTEX_Y] = vtyp->vertex[VERTEX_Y] + poly->parent->world_y;
        poly->world[i][VERTEX_Z] = vtyp->vertex[VERTEX_Z] + poly->parent->world_z;

        // Cam
        poly->camera[i][VERTEX_X] = poly->world[i][0] - camera.world_x;
        poly->camera[i][VERTEX_Y] = poly->world[i][1] - camera.world_y;
        poly->camera[i][VERTEX_Z] = poly->world[i][2] - camera.world_z;

        if (poly->camera[i][VERTEX_Z] < near)
        {
            poly->camera[i][VERTEX_Z] = near;
            j++;
        }
    }

    if (j == 4)
        return(FALSE);

    poly->avgz = (poly->camera[0][2] + poly->camera[1][2] + poly->camera[2][2] + poly->camera[3][2]) >> 2;
    
    return(TRUE);
}

void point_to_camera(vec3f16 cam, vec3f16 point)
{
    cam[0] = point[0] - camera.world_x;
    cam[1] = point[1] - camera.world_y;
    cam[2] = point[2] - camera.world_z;
}

void poly_to_screen(polygon_typ_ptr poly, Boolean use_inv_lut)
{
    uint32 i;
    int32 z;
    int32 lut_index;
    int32 lut_value;

    if (use_inv_lut)
    {
        for (i = 0; i < 4; i++)
        {
            z = poly->camera[i][VERTEX_Z];       

            if (z < 0) 
                z = -z; // Positive only during calc

            // Z should be within the range 0 - 16            
            lut_index = CAM_Z_TO_LUT(z);

            if (lut_index < 0) lut_index = 0;
            else if (lut_index >= 2048) lut_index = 2047;

            #if DEBUG_MODE 
                if (lut_index < 0 || lut_index >= 2048)
                    printf("Error - Invalid LUT index %d.\n", lut_index);
            #endif 

            lut_value = inv_depth_table[lut_index];

            if (poly->camera[i][VERTEX_Z] < 0)
                lut_value = -lut_value; // Go back to negative if needed

            poly->screen[i].pt_X = MulSF16(poly->camera[i][VERTEX_X] << VIEW_DIST_SHIFT, lut_value) + display_width2_f16;
            poly->screen[i].pt_Y = MulSF16(-poly->camera[i][VERTEX_Y] << VIEW_DIST_SHIFT, lut_value) + display_height2_f16;

            // Test clamping to reduce lag
            #if 0
            if (poly->screen[i].pt_X > 27525120)
                poly->screen[i].pt_X = 27525120;

            if (poly->screen[i].pt_X < -27525120)
                poly->screen[i].pt_X = -27525120;
            #endif
        }
    }
    else 
    {
        for (i = 0; i < 4; i++)
        {
            z = poly->camera[i][VERTEX_Z];
            
            if (z == 0) 
                z = 1;

            poly->screen[i].pt_X = DivSF16(poly->camera[i][VERTEX_X] << VIEW_DIST_SHIFT, z) + display_width2_f16;
            poly->screen[i].pt_Y = DivSF16(-poly->camera[i][VERTEX_Y] << VIEW_DIST_SHIFT, z) + display_height2_f16;
        }
    }

    FastMapCelf16(poly->ccb, poly->screen);
}

void point_to_screen(Point *dest, vec3f16 point, Boolean use_inv_lut)
{
    int32 z = point[2];
    int32 lut_index;
    int32 lut_value;

    if (use_inv_lut)
    {
        if (z < 0) 
            z = -z; // Positive only during calc

        // Z should be within the range 0 - 16            
        lut_index = CAM_Z_TO_LUT(z);

        if (lut_index < 0) lut_index = 0;
        else if (lut_index >= 2048) lut_index = 2047;

        #if DEBUG_MODE 
            if (lut_index < 0 || lut_index >= 2048)
                printf("Error - Invalid LUT index %d.\n", lut_index);
        #endif 

        lut_value = inv_depth_table[lut_index];

        if (point[2] < 0)
            lut_value = -lut_value; // Go back to negative if needed

        dest->pt_X = ( MulSF16(point[0] << VIEW_DIST_SHIFT, lut_value) >> FRACBITS_16 ) + display_width2;
        dest->pt_Y = ( MulSF16(-point[1] << VIEW_DIST_SHIFT, lut_value) >> FRACBITS_16 ) + display_height2;
    }
    else 
    {
        if (z == 0) z = 1;

        dest->pt_X = ( DivSF16(point[0] << VIEW_DIST_SHIFT, z) >> FRACBITS_16 ) + display_width2;
        dest->pt_Y = ( DivSF16(-point[1] << VIEW_DIST_SHIFT, z) >> FRACBITS_16 ) + display_height2;
    }
}

void calc_poly_normal(polygon_typ_ptr poly)
{
    vertex_typ_ptr vert1, vert2, vert3;
    vec3f16 p1, p2;
    vec3f16 vec1, vec2;

    vert1 = &poly->parent->vertex_def.vertices[ poly->vertex_lut[0] ];
    vert2 = &poly->parent->vertex_def.vertices[ poly->vertex_lut[1] ];
    vert3 = &poly->parent->vertex_def.vertices[ poly->vertex_lut[2] ];

    p1[0] = vert1->vertex[VERTEX_X];
    p1[1] = vert1->vertex[VERTEX_Y];
    p1[2] = vert1->vertex[VERTEX_Z];

    p2[0] = vert2->vertex[VERTEX_X];
    p2[1] = vert2->vertex[VERTEX_Y];
    p2[2] = vert2->vertex[VERTEX_Z];

    vector3_from_points(vec1, p2, p1);

    p1[0] = vert2->vertex[VERTEX_X];
    p1[1] = vert2->vertex[VERTEX_Y];
    p1[2] = vert2->vertex[VERTEX_Z];

    p2[0] = vert3->vertex[VERTEX_X];
    p2[1] = vert3->vertex[VERTEX_Y];
    p2[2] = vert3->vertex[VERTEX_Z];

    vector3_from_points(vec2, p2, p1);

    Cross3_F16(poly->normal, vec2, vec1);

    normalize_vector3(poly->normal);
}

void scale_obj(object_typ_ptr obj, int32 scale_factor)
{    
    vertex_typ_ptr vtyp;
    int32 i;

    vtyp = obj->vertex_def.vertices; // First

    i = obj->vertex_def.vertex_count;

    while (i--)
    {
        // MulScalarF16() 

        vtyp->vertex[VERTEX_X] = MulSF16(vtyp->vertex[VERTEX_X], scale_factor);
        vtyp->vertex[VERTEX_Y] = MulSF16(vtyp->vertex[VERTEX_Y], scale_factor);
        vtyp->vertex[VERTEX_Z] = MulSF16(vtyp->vertex[VERTEX_Z], scale_factor);

        vtyp++; // Next
    }
}

void scale_obj_x(object_typ_ptr obj, int32 scale_factor)
{    
    vertex_typ_ptr vtyp;
    int32 i;

    vtyp = obj->vertex_def.vertices; // First

    i = obj->vertex_def.vertex_count;

    while (i--)
    {
        // MulScalarF16() 

        vtyp->vertex[VERTEX_X] = MulSF16(vtyp->vertex[VERTEX_X], scale_factor);

        vtyp++; // Next
    }
}

void scale_obj_y(object_typ_ptr obj, int32 scale_factor)
{    
    vertex_typ_ptr vtyp;
    int32 i;

    vtyp = obj->vertex_def.vertices; // First

    i = obj->vertex_def.vertex_count;

    while (i--)
    {
        // MulScalarF16() 

        vtyp->vertex[VERTEX_Y] = MulSF16(vtyp->vertex[VERTEX_Y], scale_factor);

        vtyp++; // Next
    }
}

object_typ_ptr load_obj(char *file_path)
{
    rez_envelope_typ rez_envelope;
    object_typ_ptr obj;

    obj = (object_typ_ptr) AllocMem(sizeof(object_typ), MEMTYPE_DRAM);
    memset((void*)obj, 0, sizeof(object_typ));

    if (load_resource(file_path, REZ_FILE, &rez_envelope) >= 0)
    {
        uint32 i;

        seek_rez_data(&rez_envelope, (int32*) &obj->vertex_def.vertex_count);
        seek_rez_data(&rez_envelope, (int32*) &obj->poly_count);

        obj->vertex_def.vertices = (vertex_typ_ptr) AllocMem(sizeof(vertex_typ) * obj->vertex_def.vertex_count, MEMTYPE_DRAM);

        for (i = 0; i < obj->vertex_def.vertex_count; i++)
        {
            // 12 bytes per vertex (3 values * 4 bytes)
            seek_rez_data(&rez_envelope, &obj->vertex_def.vertices[i].vertex[VERTEX_X]);
            seek_rez_data(&rez_envelope, &obj->vertex_def.vertices[i].vertex[VERTEX_Y]);
            seek_rez_data(&rez_envelope, &obj->vertex_def.vertices[i].vertex[VERTEX_Z]);
        }

        obj->polygons = (polygon_typ_ptr) AllocMem(sizeof(polygon_typ) * obj->poly_count, MEMTYPE_DRAM);

        for (i = 0; i < obj->poly_count; i++)
        {
            // 16 bytes per polygon (4 values * 4 bytes)
            seek_rez_data(&rez_envelope, (int32*) &obj->polygons[i].vertex_lut[0]);
            seek_rez_data(&rez_envelope, (int32*) &obj->polygons[i].vertex_lut[1]);
            seek_rez_data(&rez_envelope, (int32*) &obj->polygons[i].vertex_lut[2]);
            seek_rez_data(&rez_envelope, (int32*) &obj->polygons[i].vertex_lut[3]);

            obj->polygons[i].ccb = 0;

            obj->polygons[i].parent = obj;
        }

        unload_resource(&rez_envelope, REZ_FILE);

        #if DEBUG_MODE 
            printf("Loaded obj %s.\n", file_path);
        #endif
    }
    else 
    {
        FreeMem((void*)obj, sizeof(object_typ));
        obj = 0;    

        #if DEBUG_MODE 
            printf("Error - failed to load obj %s.\n", file_path);
        #endif
    }

    return(obj);
}

void unload_obj(object_typ_ptr obj)
{
    polygon_typ_ptr poly;
    uint32 i;

    if (!obj)
        return;

    if (obj->vertex_def.vertex_count > 0)
    {
        if (obj->vertex_def.vertices)
        {
            FreeMem(obj->vertex_def.vertices, sizeof(vertex_typ) * obj->vertex_def.vertex_count);
        }
    }

    if (obj->vertex_def_copy.vertex_count > 0)
    {
        if (obj->vertex_def_copy.vertices)
        {
            FreeMem(obj->vertex_def_copy.vertices, sizeof(vertex_typ) * obj->vertex_def_copy.vertex_count);
        }
    }

    if (obj->poly_count > 0)
    {
        if (obj->polygons)
        {
            poly = obj->polygons;

            i = obj->poly_count;

            while (i-- > 0)
            {
                if (poly->ccb)
                {
                    DeleteCel(poly->ccb);
                }
                poly++;
            }

            FreeMem(obj->polygons, sizeof(polygon_typ) * obj->poly_count);
        }
    }    

    FreeMem(obj, sizeof(object_typ));
}

void copy_vertex_def(vertex_def_typ_ptr dest, vertex_def_typ_ptr source)
{
    uint32 nbytes = sizeof(vertex_typ) * source->vertex_count;
    memcpy((void*)dest->vertices, (void*)source->vertices, nbytes);    
}

void clone_vertex_def(vertex_def_typ_ptr dest, vertex_def_typ_ptr source)
{
    uint32 nbytes = sizeof(vertex_typ) * source->vertex_count;
    dest->vertices = (vertex_typ_ptr) AllocMem(nbytes, MEMTYPE_DRAM);
    dest->vertex_count = source->vertex_count;
    memcpy((void*)dest->vertices, (void*)source->vertices, nbytes);
}

int32 add_obj_zclip(object_typ_ptr obj, int32 near)
{
    uint32 i;
    polygon_typ_ptr poly;

    // Check if poly fits
    if ((poly_list_size + obj->poly_count > MAX_POLY_RASTER) || poly_list_size >= MAX_POLY_RASTER)
        return(-1);

    poly = obj->polygons;
    i = obj->poly_count;

    while (i--)
    {
        if (poly_to_world_cam_clip(poly, near))
        {
            // One or more polygon vertices are in front of the camera
            poly_to_screen(poly, FALSE);
            poly_list[poly_list_size++] = poly;    
        }
       
        ++poly;
    }

    return(0);
}

int32 add_obj(object_typ_ptr obj, Boolean use_inv_lut)
{
    uint32 i = obj->poly_count;
    polygon_typ_ptr poly = obj->polygons;

    // Check if poly fits
    if ((poly_list_size + obj->poly_count > MAX_POLY_RASTER) || poly_list_size >= MAX_POLY_RASTER)
        return(-1);

    while (i--)
    {
        poly_to_world_cam(poly);
        poly_to_screen(poly, use_inv_lut);
        poly_list[poly_list_size++] = poly;     
        ++poly;
    }

    return(0);
}

void begin_3d(void)
{
    poly_list_size = 0;
}

void end_3d(void)
{
    int32 i;
    
    if (poly_list_size > 0)
    {
        // Link cels 

        for (i = 0; i < poly_list_size-1; i++)
        {
            LINK_CEL(poly_list[i]->ccb, poly_list[i+1]->ccb);
            UNLAST_CEL(poly_list[i]->ccb);
        }

        LAST_CEL(poly_list[poly_list_size-1]->ccb);
    }
}

void sort_polys(void)
{
    uint32 i, j;
    polygon_typ_ptr temp;

    for (i = 0; i < poly_list_size; i++)
    {
        for (j = 0; j < poly_list_size-1; j++)
        {
            if (poly_list[j]->avgz < poly_list[j+1]->avgz)
            {
                temp = poly_list[j];
                poly_list[j] = poly_list[j+1];
                poly_list[j+1] = temp;
            }
        }
    }
}

void raster_scene(CCB *bg_start, CCB *bg_end, CCB *fg_start)
{
    CCB *first = NULL;

    if (poly_list_size > 0)
    {
        if (bg_start)
        {
            first = bg_start;
            UNLAST_CEL(bg_end);
            LINK_CEL(bg_end, poly_list[0]->ccb);

            if (fg_start)
            {
                UNLAST_CEL(poly_list[poly_list_size-1]->ccb);
                LINK_CEL(poly_list[poly_list_size-1]->ccb, fg_start);
            }           
        }
        else 
        {
            first = poly_list[0]->ccb;

            if (fg_start)
            {
                UNLAST_CEL(poly_list[poly_list_size-1]->ccb);
                LINK_CEL(poly_list[poly_list_size-1]->ccb, fg_start);
            }
        }
    }
    else 
    {
        if (fg_start)
            first = fg_start;
    }

    if (first)
        DrawCels(sc->sc_BitmapItems[sc->sc_CurrentScreen], first);
}

void raster_scene_wireframe(uint32 color, CCB *fg)
{
    polygon_typ_ptr *poly_pp;
    uint32 i;

    poly_pp = poly_list;

    for (i = 0; i < poly_list_size; i++)
    {
        draw_poly_wireframe(*poly_pp, color);
        ++poly_pp;
    }

    if (fg)
        DrawCels(sc->sc_BitmapItems[sc->sc_CurrentScreen], fg);
}

void draw_poly_wireframe(polygon_typ_ptr poly, uint32 color)
{
    uint32 i;

    static GrafCon gcon;
    
    SetFGPen(&gcon, color);

    gcon.gc_PenX = poly->screen[0].pt_X >> FRACBITS_16;
    gcon.gc_PenY = poly->screen[0].pt_Y >> FRACBITS_16;

    for (i = 1; i < 4; i++)
    {
        DrawTo(sc->sc_BitmapItems[sc->sc_CurrentScreen], &gcon, 
            poly->screen[i].pt_X >> FRACBITS_16, poly->screen[i].pt_Y >> FRACBITS_16);
    }

    DrawTo(sc->sc_BitmapItems[sc->sc_CurrentScreen], &gcon, 
        poly->screen[0].pt_X >> FRACBITS_16, poly->screen[0].pt_Y >> FRACBITS_16);
}

void draw_poly_normal(Item bitmap_item, polygon_typ_ptr poly)
{
    static GrafCon gcon;
    static Boolean first_run = TRUE;
    vec3f16 pos;
    vec3f16 normal;
    Coord start_x, start_y;
    Coord end_x, end_y;

    if (first_run)
    {
        SetFGPen(&gcon, MakeRGB15(0, 31, 0));
        first_run = FALSE;
    }

    // Scale normal for display purposes
    normal[0] = MulSF16(poly->normal[0], 16384); // x
    normal[1] = MulSF16(poly->normal[1], 16384); // y
    normal[2] = MulSF16(poly->normal[2], 16384); // z

    // Draw normal from center of poly

    pos[0] = (poly->world[0][0] + poly->world[2][0]) / 2 - camera.world_x;
    pos[1] = (poly->world[0][1] + poly->world[2][1]) / 2 - camera.world_y;
    pos[2] = (poly->world[0][2] + poly->world[2][2]) / 2 - camera.world_z;

    if (pos[2] == 0)
        pos[2] = 1;

    start_x = ( DivSF16(pos[0] << VIEW_DIST_SHIFT, pos[2]) >> FRACBITS_16 ) + display_width2;
    start_y = ( DivSF16(-pos[1] << VIEW_DIST_SHIFT, pos[2]) >> FRACBITS_16 ) + display_height2;

    // Project normal endpoint in 3D

    pos[0] = pos[0] + normal[0];
    pos[1] = pos[1] + normal[1];
    pos[2] = pos[2] + normal[2];

    if (pos[2] == 0)
        pos[2] = 1;

    // Convert endpoint to screen space

    end_x = ( DivSF16(pos[0] << VIEW_DIST_SHIFT, pos[2]) >> FRACBITS_16 ) + display_width2;
    end_y = ( DivSF16(-pos[1] << VIEW_DIST_SHIFT, pos[2]) >> FRACBITS_16 ) + display_height2;

    gcon.gc_PenX = start_x;
    gcon.gc_PenY = start_y;

    DrawTo(bitmap_item, &gcon, end_x, end_y);
}

void draw_obj_normals(Item bitmap_item, object_typ_ptr obj)
{
    uint32 i;

    for (i = 0; i < obj->poly_count; i++)
        draw_poly_normal(bitmap_item, &obj->polygons[i]);
}

void rotate_obj(object_typ_ptr obj, vec3f16 angles)
{
    uint32 flags;
    int32 i;
    frac16 cs, sn;
    mat33f16 rotx;
    mat33f16 roty;
    mat33f16 rotz;
    mat33f16 temp;
    mat33f16 rotation;
    vec3f16 transform;
    vertex_typ_ptr vtyp;
    
    flags = 0;

    if (angles[0])
    {
        // x rotation
        flags |= 1;

        identity_matrix(rotx);
        cs = CosF16(angles[0]);
        sn = SinF16(angles[0]);
        rotx[1][1] = cs;
        rotx[1][2] = -sn;
        rotx[2][1] = sn;
        rotx[2][2] = cs;
    }

    if (angles[1])
    {
        // y rotation
        flags |= 2;

        identity_matrix(roty);
        cs = CosF16(angles[1]);
        sn = SinF16(angles[1]);       
        roty[0][0] = cs;
        roty[0][2] = sn;
        roty[2][0] = -sn;
        roty[2][2] = cs;
    }

    if (angles[2])
    {
        // z rotation
        flags |= 4;

        identity_matrix(rotz);
        cs = CosF16(angles[2]);
        sn = SinF16(angles[2]);
        rotz[0][0] = cs;
        rotz[0][1] = -sn;
        rotz[1][0] = sn;
        rotz[1][1] = cs;
    }

    if (flags)
    {
        switch (flags)
        {
            case 1: // x rot only
                COPY_MAT33F16(rotation, rotx);
                break;
            case 2: // y rot only
                COPY_MAT33F16(rotation, roty);
                break;
            case 4: // z rot only
                COPY_MAT33F16(rotation, rotz);
                break;
            case 3: // x and y rot 
                MulMat33Mat33_F16(rotation, rotx, roty);
                break;
            case 6: // y and z rot 
                MulMat33Mat33_F16(rotation, roty, rotz);
                break;
            case 7: // x y z rot 
                MulMat33Mat33_F16(temp, rotx, roty);
                MulMat33Mat33_F16(rotation, temp, rotz);
                break;
            default:
                break;
        }

        /*  Update vertices. Note, this will impact all meshes using the same vertex def.
            If you want to isolate this, give the object its own deep copy. */

        vtyp = obj->vertex_def.vertices; // First

        i = obj->vertex_def.vertex_count;
        
        while (i--)
        {
            transform[0] = vtyp->vertex[VERTEX_X];
            transform[1] = vtyp->vertex[VERTEX_Y];
            transform[2] = vtyp->vertex[VERTEX_Z];
            
            MulVec3Mat33_F16(vtyp->vertex, transform, rotation);

            vtyp++; // Next vertex
        }
    }
}

void rotate_obj4_z(object_typ_ptr obj, int32 angle)
{
    mat33f16 rotz;
    frac16 cs, sn;
    vec3f16 source_verts[4];
    vec3f16 dest_verts[4];

    cs = CosF16(angle);
    sn = SinF16(angle);

    identity_matrix(rotz);
    rotz[0][0] = cs;
    rotz[0][1] = -sn;
    rotz[1][0] = sn;
    rotz[1][1] = cs;

    // No loops to squeeze performance

    source_verts[0][VERTEX_X] = obj->vertex_def.vertices[0].vertex[VERTEX_X];
    source_verts[0][VERTEX_Y] = obj->vertex_def.vertices[0].vertex[VERTEX_Y];
    source_verts[0][VERTEX_Z] = obj->vertex_def.vertices[0].vertex[VERTEX_Z];

    source_verts[1][VERTEX_X] = obj->vertex_def.vertices[1].vertex[VERTEX_X];
    source_verts[1][VERTEX_Y] = obj->vertex_def.vertices[1].vertex[VERTEX_Y];
    source_verts[1][VERTEX_Z] = obj->vertex_def.vertices[1].vertex[VERTEX_Z];

    source_verts[2][VERTEX_X] = obj->vertex_def.vertices[2].vertex[VERTEX_X];
    source_verts[2][VERTEX_Y] = obj->vertex_def.vertices[2].vertex[VERTEX_Y];
    source_verts[2][VERTEX_Z] = obj->vertex_def.vertices[2].vertex[VERTEX_Z];

    source_verts[3][VERTEX_X] = obj->vertex_def.vertices[3].vertex[VERTEX_X];
    source_verts[3][VERTEX_Y] = obj->vertex_def.vertices[3].vertex[VERTEX_Y];
    source_verts[3][VERTEX_Z] = obj->vertex_def.vertices[3].vertex[VERTEX_Z];

    MulManyVec3Mat33_F16(dest_verts, source_verts, rotz, 4);

    obj->vertex_def.vertices[0].vertex[VERTEX_X] = dest_verts[0][VERTEX_X];
    obj->vertex_def.vertices[0].vertex[VERTEX_Y] = dest_verts[0][VERTEX_Y];
    obj->vertex_def.vertices[0].vertex[VERTEX_Z] = dest_verts[0][VERTEX_Z];

    obj->vertex_def.vertices[1].vertex[VERTEX_X] = dest_verts[1][VERTEX_X];
    obj->vertex_def.vertices[1].vertex[VERTEX_Y] = dest_verts[1][VERTEX_Y];
    obj->vertex_def.vertices[1].vertex[VERTEX_Z] = dest_verts[1][VERTEX_Z];

    obj->vertex_def.vertices[2].vertex[VERTEX_X] = dest_verts[2][VERTEX_X];
    obj->vertex_def.vertices[2].vertex[VERTEX_Y] = dest_verts[2][VERTEX_Y];
    obj->vertex_def.vertices[2].vertex[VERTEX_Z] = dest_verts[2][VERTEX_Z];

    obj->vertex_def.vertices[3].vertex[VERTEX_X] = dest_verts[3][VERTEX_X];
    obj->vertex_def.vertices[3].vertex[VERTEX_Y] = dest_verts[3][VERTEX_Y];
    obj->vertex_def.vertices[3].vertex[VERTEX_Z] = dest_verts[3][VERTEX_Z];
}

void rotate_obj_pivot_z(object_typ_ptr obj, vec3f16 pivot, int32 angle)
{
    uint32 i;
    int32 cs, sn;
    vertex_typ_ptr vtyp;
    vec3f16 transform;
    mat33f16 rotz;

    vtyp = obj->vertex_def.vertices; // First

    i = obj->vertex_def.vertex_count;
    
    identity_matrix(rotz);
    cs = CosF16(angle);
    sn = SinF16(angle);
    rotz[0][0] = cs;
    rotz[0][1] = -sn;
    rotz[1][0] = sn;
    rotz[1][1] = cs;
    
    // Rotate verts 

    while (i--)
    {
        // Convert vertex to world space and subtract pivot point.
        transform[VERTEX_X] = vtyp->vertex[VERTEX_X] + obj->world_x - pivot[VERTEX_X];
        transform[VERTEX_Y] = vtyp->vertex[VERTEX_Y] + obj->world_y - pivot[VERTEX_Y];
        transform[VERTEX_Z] = vtyp->vertex[VERTEX_Z] + obj->world_z - pivot[VERTEX_Z];
        
        // Rotate 
        MulVec3Mat33_F16(vtyp->vertex, transform, rotz);

        // Add pivot back
        vtyp->vertex[VERTEX_X] += pivot[VERTEX_X];// - obj->world_x;
        vtyp->vertex[VERTEX_Y] += pivot[VERTEX_Y];// - obj->world_y;
        vtyp->vertex[VERTEX_Z] += pivot[VERTEX_Z];// - obj->world_z;

        vtyp++; // Next vertex
    }

    // Rotate center to keep object intact
    
    transform[VERTEX_X] = obj->world_x - pivot[VERTEX_X];
    transform[VERTEX_Y] = obj->world_y - pivot[VERTEX_Y];
    transform[VERTEX_Z] = obj->world_z - pivot[VERTEX_Z];

    MulVec3Mat33_F16(transform, transform, rotz);

    obj->world_x = transform[VERTEX_X] + pivot[VERTEX_X];
    obj->world_y = transform[VERTEX_Y] + pivot[VERTEX_Y];
    obj->world_z = transform[VERTEX_Z] + pivot[VERTEX_Z];

    // Convert verts back to local space

    vtyp = obj->vertex_def.vertices; // First

    i = obj->vertex_def.vertex_count;

    while (i--)
    {
        vtyp->vertex[VERTEX_X] -= obj->world_x;
        vtyp->vertex[VERTEX_Y] -= obj->world_y;
        vtyp->vertex[VERTEX_Z] -= obj->world_z;

        vtyp++; // Next vertex
    }
}

object_typ_ptr copy_obj(object_typ_ptr source)
{
    object_typ_ptr dest;
    uint32 i;

    dest = (object_typ_ptr) AllocMem(sizeof(object_typ), MEMTYPE_DRAM);
    memset((void*)dest, 0, sizeof(object_typ));

    dest->vertex_def.vertex_count = source->vertex_def.vertex_count;
    dest->vertex_def.vertices = (vertex_typ_ptr) AllocMem(sizeof(vertex_typ) * source->vertex_def.vertex_count, MEMTYPE_DRAM);
    memcpy((void*)dest->vertex_def.vertices, (void*)source->vertex_def.vertices, sizeof(vertex_typ) * source->vertex_def.vertex_count);

    dest->poly_count = source->poly_count;
    dest->polygons = (polygon_typ_ptr) AllocMem(sizeof(polygon_typ) * source->poly_count, MEMTYPE_DRAM);

    for (i = 0; i < source->poly_count; i++)
    {
        dest->polygons[i].parent = dest;
        memcpy((void*)dest->polygons[i].vertex_lut, (void*)source->polygons[i].vertex_lut, (sizeof(uint32) * 4));
        memcpy((void*)dest->polygons[i].normal, (void*)source->polygons[i].normal, sizeof(vec3f16));

        // Set up CCB 
        dest->polygons[i].ccb = create_coded_cel8(source->polygons[i].ccb->ccb_Width, source->polygons[i].ccb->ccb_Height, FALSE);
        dest->polygons[i].ccb->ccb_SourcePtr = source->polygons[i].ccb->ccb_SourcePtr;
        dest->polygons[i].ccb->ccb_PLUTPtr = source->polygons[i].ccb->ccb_PLUTPtr;
    }

    dest->bsphere_radius = source->bsphere_radius;

    return(dest);
}

void print_obj(object_typ_ptr obj)
{
    uint32 i, j;

    printf("** Print Object **\n");
    printf("World xyz %d %d %d\n", obj->world_x, obj->world_y, obj->world_z);
    printf("Poly count %d\n", obj->poly_count);
    printf("Vertex count %d\n", obj->vertex_def.vertex_count);
    printf("Vertices:\n");
    
    for (i = 0; i < obj->vertex_def.vertex_count; i++)
    {
        printf("\t%d %d %d\n", obj->vertex_def.vertices[i].vertex[0], 
            obj->vertex_def.vertices[i].vertex[1], 
            obj->vertex_def.vertices[i].vertex[2]);
    }

    for (i = 0; i < obj->poly_count; i++)
    {
        printf("Poly %d\n", i+1);

        printf("\tVertex LUT: ");

        for (j = 0; j < 4; j++)
            printf("%d ", obj->polygons[i].vertex_lut[j]);
        printf("\n");
        
        printf("\tCCB width / height: %d %d\n", obj->polygons[i].ccb->ccb_Width, obj->polygons[i].ccb->ccb_Height);
    }

    printf("\n");
}

void normalize_vector(vec3f16 vec)
{
    int32 mag = SqrtF16(SquareSF16(vec[0]) + SquareSF16(vec[1]) + SquareSF16(vec[2]));

    if (mag == 0) mag = 1;
    
    vec[0] = DivSF16(vec[0], mag);
    vec[1] = DivSF16(vec[1], mag);
    vec[2] = DivSF16(vec[2], mag);
}

int32 get_vector_magnitude(vec3f16 vec)
{
    int32 mag = SqrtF16(SquareSF16(vec[0]) + SquareSF16(vec[1]) + SquareSF16(vec[2]));
    return(mag);    
}

void print_matrix_3x3(mat33f16 matrix)
{
    uint32 x, y;

    for (y = 0; y < 3; y++)
    {
        for (x = 0; x < 3; x++)
        {
            printf("%d ", matrix[x][y]);
        }
        printf("\n");
    }
    printf("\n");
}

uint32 calc_bsphere_radius(object_typ_ptr obj)
{
    int32 x, y, z;
    int32 radius, next_radius;
    uint32 i;

    radius = next_radius = 0;

    for (i = 0; i < obj->vertex_def.vertex_count; i++)
    {
        x = obj->vertex_def.vertices[i].vertex[VERTEX_X];
        y = obj->vertex_def.vertices[i].vertex[VERTEX_Y];
        z = obj->vertex_def.vertices[i].vertex[VERTEX_Z];

        next_radius = SqrtF16(SquareSF16(x) + SquareSF16(y) + SquareSF16(z));
       
        if (next_radius > radius)
            radius = next_radius;
    }   

    return( (uint32) radius);
}

Boolean is_colliding(object_typ_ptr obj1, object_typ_ptr obj2, Boolean half)
{
    int32 squared_dist;
    int32 squared_radius;
    int32 r1, r2;
    int32 a, b, c;

    a = obj2->world_x - obj1->world_x;
    b = obj2->world_y - obj1->world_y;
    c = obj2->world_z - obj1->world_z;

    r1 = obj1->bsphere_radius;
    r2 = obj2->bsphere_radius;

    if (half)
    {
        r1 = r1 >> 1;
        r2 = r2 >> 1;
    }

    squared_dist = SquareSF16(a) + SquareSF16(b) + SquareSF16(c);
    squared_radius = SquareSF16(r1 + r2);

    if (squared_radius >= squared_dist)
        return(TRUE);

    return(FALSE);
}

void init_3d(void)
{
    uint32 i;
    uint32 value = 512;

    for (i = 0; i < Z_LUT_SIZE; i++)
    {
        inv_depth_table[i] = DivSF16(ONE_F16, value);
        value += 512;
    }
}

void translate_obj(object_typ_ptr obj, vec3f16 transform)
{
    obj->world_x += transform[0];
    obj->world_y += transform[1];
    obj->world_z += transform[2];
}

void set_obj_xyz(object_typ_ptr obj, int32 x, int32 y, int32 z)
{
    obj->world_x = x;
    obj->world_y = y;
    obj->world_z = z;
}

void calc_obj_normals(object_typ_ptr obj)
{
    uint32 i;

    for (i = 0; i < obj->poly_count; i++)
        calc_poly_normal(&obj->polygons[i]);
}