/**
 * @file threed.h
 * @brief Small 3D library
 * 
 * Object vertices need to be in CCW winding order.
 * 
 */

#ifndef THREED_H
#define THREED_H

// My includes
#include "app_globals.h"

// 3DO includes
#include "types.h"
#include "graphics.h"
#include "operamath.h"

// Useful constants to index vec3f16 types

#define X 0
#define Y 1
#define Z 2

// Useful constants to index vec3f16 types
#define VERTEX_X 0
#define VERTEX_Y 1  
#define VERTEX_Z 2  

// Useful constants to index vec3f16 types
#define ANGLE_X 0   
#define ANGLE_Y 1
#define ANGLE_Z 2

typedef struct vertex_typ
{
    vec3f16 vertex;    
} vertex_typ, *vertex_typ_ptr;

typedef struct vertex_def_typ 
{
    uint32 vertex_count;
    vertex_typ_ptr vertices;
} vertex_def_typ, *vertex_def_typ_ptr;

struct object_typ;

typedef struct polygon_typ
{
    struct object_typ *parent;
    CCB *ccb;
    uint32 vertex_lut[4];
    vec3f16 world[4];
    vec3f16 camera[4];
    Point screen[4];
    vec3f16 normal;
    int32 avgz;
    uint16 pal_backup[32];
} polygon_typ, *polygon_typ_ptr;

typedef struct object_typ
{
    int32 world_x;
    int32 world_y;
    int32 world_z;
    vertex_def_typ vertex_def;
    vertex_def_typ vertex_def_copy;
    uint32 poly_count;
    polygon_typ_ptr polygons;
    uint32 bsphere_radius;
} object_typ, *object_typ_ptr;

typedef struct camera_typ 
{
    int32 world_x;
    int32 world_y;
    int32 world_z;
} camera_typ, *camera_typ_ptr;

extern camera_typ camera;

void init_3d(void);

object_typ_ptr copy_obj(object_typ_ptr source);

void poly_to_world_cam(polygon_typ_ptr poly);

Boolean poly_to_world_cam_clip(polygon_typ_ptr poly, int32 near);

void point_to_camera(vec3f16 cam, vec3f16 point);

void poly_to_screen(polygon_typ_ptr poly, Boolean use_inv_lut);

void point_to_screen(Point *dest, vec3f16 point, Boolean use_inv_lut);

void calc_poly_normal(polygon_typ_ptr poly);

void scale_obj(object_typ_ptr obj, int32 scale_factor);

void scale_obj_x(object_typ_ptr obj, int32 scale_factor);

void scale_obj_y(object_typ_ptr obj, int32 scale_factor);

/**
 * @brief Rotate object on specified axis via angles.
 * 
 * @param obj 
 * @param angles 
 */
void rotate_obj(object_typ_ptr obj, vec3f16 angles);

void rotate_obj4_z(object_typ_ptr obj, int32 angle);

/**
 * @brief Rotate object around pivot point along z axis.
 * 
 * @param obj 
 * @param pivot 
 * @param angle 
 */
void rotate_obj_pivot_z(object_typ_ptr obj, vec3f16 pivot, int32 angle);

void sort_polys(void);

uint32 calc_bsphere_radius(object_typ_ptr obj);

/**
 * @brief Copy source into dest. Ensure both sizes are equal.
 * 
 * @param dest 
 * @param source 
 */
void copy_vertex_def(vertex_def_typ_ptr dest, vertex_def_typ_ptr source);

/**
 * @brief Allocate space in dest and copy source into dest.
 * 
 * @param dest 
 * @param source 
 */
void clone_vertex_def(vertex_def_typ_ptr dest, vertex_def_typ_ptr source);

/**
 * @brief This is very slow and should only be used for debugging / testing.
 * @param obj 
 */
void draw_obj_normals(Item bitmap_item, object_typ_ptr obj);

/**
 * @brief This is very slow and should only be used for debugging / testing.
 * @param poly 
 */
void draw_poly_normal(Item bitmap_item, polygon_typ_ptr poly);

/**
 * @brief Load object model.
 * 
 * This uses load_resource under the hood to support semaphores.
 * @param path 
 * @return object_typ_ptr 
 */
object_typ_ptr load_obj(char *path);

void unload_obj(object_typ_ptr obj);

int32 add_obj(object_typ_ptr obj, Boolean use_inv_lut);

/**
 * @brief 
 * This will only work for axis-aligned quads.
 * @param poly 
 * @param near 
 * @return Boolean 
 */
int32 add_obj_zclip(object_typ_ptr obj, int32 near);

/**
 * @brief Call this before adding 3d objects
 */
void begin_3d(void);

/**
 * @brief Call after adding 3d objects
 */
void end_3d(void);

void raster_scene(CCB *bg_start, CCB *bg_end, CCB *fg_start);

void raster_scene_wireframe(uint32 color, CCB *fg);

void print_obj(object_typ_ptr obj);

void print_matrix_3x3(mat33f16 matrix);

void normalize_vector(vec3f16 vec);

int32 get_vector_magnitude(vec3f16 vec);

Boolean is_colliding(object_typ_ptr obj1, object_typ_ptr obj2, Boolean half);

void translate_obj(object_typ_ptr obj, vec3f16 transform);

void draw_poly_wireframe(polygon_typ_ptr poly, uint32 color);

void set_obj_xyz(object_typ_ptr obj, int32 x, int32 y, int32 z);

void calc_obj_normals(object_typ_ptr obj);

#endif // THREED_H