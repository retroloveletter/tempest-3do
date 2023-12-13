#ifndef MATHS_H
#define MATHS_H

#include "operamath.h"

#define ABS_VALUE(x) ((x) >= 0 ? (x) : -(x))

/**
 * Copy source matrix into destination matrix.
 * sizeof(frac16) = 4.
 * A 3x3 matrix has 9 elements.
 * Total bytes = 4 * 9 (36).
 * I'm using the literal value 36 below to avoid calculations.
*/
#define COPY_MAT33F16(dest, source) (memcpy((void*)dest, (void*)source, 36))

// Vector = a - b
void vector3_from_points(vec3f16 dest, vec3f16 a, vec3f16 b);

void normalize_vector3(vec3f16 v);

void identity_matrix(mat33f16 matrix);

uint32 get_squared_dist(vec3f16 p1, vec3f16 p2);

#endif // MATHS_H