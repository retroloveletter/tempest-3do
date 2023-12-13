// My includes.
#include "app_globals.h"
#include "maths.h"

// 3DO includes.
#include "string.h"


void normalize_vector3(vec3f16 v)
{
    frac16 length = SqrtF16(SquareSF16(v[0]) + SquareSF16(v[1]) + SquareSF16(v[2]));

    // This can happen with very small polygons due to 16.16 precision
    if (length == 0)
        length = ONE_F16;

    v[0] = DivSF16(v[0], length); // x
    v[1] = DivSF16(v[1], length); // y
    v[2] = DivSF16(v[2], length); // z
}

void vector3_from_points(vec3f16 dest, vec3f16 a, vec3f16 b)
{
    dest[0] = a[0] - b[0]; // x
    dest[1] = a[1] - b[1]; // y
    dest[2] = a[2] - b[2]; // z
}

void identity_matrix(mat33f16 matrix)
{
	// 36 bytes is 3DO specific.. sizeof(frac16) * 9 for the 3x3 array
	memset((void*)matrix, 0, 36);

	matrix[0][0] = ONE_F16;
	matrix[1][1] = ONE_F16;
	matrix[2][2] = ONE_F16;
}

uint32 get_squared_dist(vec3f16 p1, vec3f16 p2)
{
    int32 a = p2[0] - p1[0];
    int32 b = p2[1] - p1[1];
    int32 c = p2[2] - p1[2];
    int32 squared_dist = SquareSF16(a) + SquareSF16(b) + SquareSF16(c);
    return(squared_dist);
}
