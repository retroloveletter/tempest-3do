#ifndef CEL_HELPER_H
#define CEL_HELPER_H

// 3DO includes
#include "graphics.h"

typedef struct cel8_data_typ
{
    ubyte *source;
    uint16 plut[32];
    uint32 source_bytes;
} cel8_data_typ, *cel8_data_typ_ptr;

typedef struct skewable_cel_typ
{
    CCB *ccb;
    int32 skew_dir;
} skewable_cel_typ, *skewable_cel_typ_ptr;

#define XY_TO_LINEAR_OFFSET(x, y, w) (y * w + x)

// Link CCB A to CCB B.
#define LINK_CEL(a, b) (a->ccb_NextPtr = b)

/*  Call XY_TO_LINEAR_OFFSET() before XY_OFFSET_TO_WORD_BPP8() and pass in the offset.
    This macro will figure out the nth word in bitmap source data holds the pixel 
    specified by the offset. This is helpful when extracting bitmap regions and setting
    up preambles. This macro is only for 8bpp data. There are 4 pixels per word when 
    using 8bpp. The shift is the same as divide by 4. */
#define XY_OFFSET_TO_WORD_BPP8(offset) (offset >> 2)

/*  Call XY_TO_LINEAR_OFFSET() before XY_OFFSET_TO_SKIPX_BPP8() and pass in the offset.
    This macro will figure out how many pixels to skip in order to reach the pixel
    in the word specified by the offset. This is helpful when extracting bitmap regions
    and setting up preambles. This macro is only for 8bpp data.*/
#define XY_OFFSET_TO_SKIPX_BPP8(offset) (offset % 4)

// You still need to subtract PRE1_WOFFSET_PREFETCH from this value.
#define CALC_WOFFSET_BPP8(width) ( (width % 4 == 0) ? (width / 4) : (width / 4 + 1) )

// The 3DO library is missing these shift constants.
#define SHIFT_PRE0_LINEAR 4
#define SHIFT_PRE0_REP8 3
#define SHIFT_PRE1_LRFORM 11

// Preamble getters and setters. The 3DO library has inconsistencies with naming conventions.

#define GET_CEL_BPP_VALUE(ccb) ((ccb->ccb_PRE0 & PRE0_BPP_MASK) >> PRE0_BPP_SHIFT)

#define GET_CEL_UNCODED(ccb) ((ccb->ccb_PRE0 & PRE0_LINEAR) >> SHIFT_PRE0_LINEAR)

// The number of rows in this cel minus one. 
#define GET_CEL_VCNT(ccb) ((ccb->ccb_PRE0 & PRE0_VCNT_MASK) >> PRE0_VCNT_SHIFT)

#define GET_CEL_SKIPX(ccb) ((ccb->ccb_PRE0 & PRE0_SKIPX_MASK) >> PRE0_SKIPX_SHIFT)

//  This is the number of cel data words from one row of pixels to the next row minus 2 for 1,2,4 or G bpp cels.
#define GET_CEL_WOFFSET8(ccb) ((ccb->ccb_PRE1 & PRE1_WOFFSET8_MASK) >> PRE1_WOFFSET8_SHIFT)

//  This is the number of cel data words from one row of pixels to the next row minus 2 for 8 and 16 bpp cels.
#define GET_CEL_WOFFSET10(ccb) ((ccb->ccb_PRE1 & PRE1_WOFFSET10_MASK) >> PRE1_WOFFSET10_SHIFT)

#define GET_CEL_LRFORM(ccb) ((ccb->ccb_PRE1 & PRE1_LRFORM) >> SHIFT_PRE1_LRFORM)

// This value is the number of pixels in each row of the cel source data minus 1.
#define GET_CEL_TLHPCNT(ccb) ((ccb->ccb_PRE1 & PRE1_TLHPCNT_MASK) >> PRE1_TLHPCNT_SHIFT)

#define GET_CEL_REP8(ccb) ((ccb->ccb_PRE0 & PRE0_REP8) >> SHIFT_PRE0_REP8)

// Clear macros
#define CLEAR_CEL_SKIPX(ccb) (ccb->ccb_PRE0 &= 0xF0FFFFFF)

#define CLEAR_CEL_TLHPCNT(ccb) (ccb->ccb_PRE1 &= 0xFFFFF800)

#define CLEAR_CEL_VCNT(ccb) (ccb->ccb_PRE0 &= 0xFFFF003F)

// These setters do not clear prior values.

#define SET_CEL_REP8(ccb) (ccb->ccb_PRE0 |= PRE0_REP8)

#define SET_CEL_VCNT(ccb, height) (ccb->ccb_PRE0 |= ((height - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT))

#define SET_CEL_UNCODED(ccb) (ccb->ccb_PRE0 |= PRE0_LINEAR)

#define SET_CEL_BPP8(ccb) (ccb->ccb_PRE0 |= (PRE0_BPP_8 << PRE0_BPP_SHIFT))

#define SET_CEL_BPP16(ccb) (ccb->ccb_PRE0 |= (PRE0_BPP_16 << PRE0_BPP_SHIFT))

#define SET_CEL_SKIPX(ccb, skip) (ccb->ccb_PRE0 |= (skip << PRE0_SKIPX_SHIFT))

#define SET_CEL_WOFFSET8(ccb, width) (ccb->ccb_PRE1 |= ((width - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET8_SHIFT))

#define SET_CEL_WOFFSET10(ccb, width) (ccb->ccb_PRE1 |= ((width - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET10_SHIFT))

#define SET_CEL_LRFORM(ccb) (ccb->ccb_PRE1 |= PRE1_LRFORM)

#define SET_CEL_TLHPCNT(ccb, width) (ccb->ccb_PRE1 |= ((width - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT))

// CLears 

#define CLEAR_CEL_PREAMBLES(ccb) (ccb->ccb_PRE0 = ccb->ccb_PRE1 = 0)

// Debug function
void print_cel(CCB *ccb);

/**
 * @brief Create an 8bpp coded cel. 
 * 
 * If alloc is FALSE, you still need to assign a source and PLUT from an existing cel. 
 * If alloc is TRUE, this cel is intended to be unique and space for the source and PLUT is
 * allocated. You still need to set the source and PLUT data.
 * 
 * @param width Cel width
 * @param height Cel height
 * @return CCB* 
 */
CCB *create_coded_cel8(uint32 width, uint32 height, Boolean alloc);

CCB *create_coded_colored_cel8(uint32 width, uint32 height, uint16 color);

int32 get_cel_row_bytes(CCB *cel);

int32 get_cel_src_bytes(CCB *cel);

void set_cel_pal_to_color(CCB *cel, uint16 color);

/**
 * @brief Extract cel source and plut data from 3DO 8bpp coded cel file.
 * 
 * This is useful to save memory when you only want to store cel data, without having
 * to store the entire 3DO CCB. You should UnloadCel() or DeleteCel() after calling 
 * this function.
 * 
 * @param ccb_3do 
 * @return cel8_data_typ_ptr 
 */
cel8_data_typ_ptr cel_3do_to_cel8_data(CCB *ccb_3do);

void free_cel8_data(cel8_data_typ_ptr cel8_data);

void set_cel_subregion_bpp8(CCB *ccb, Rect *atlas, uint32 tex_width);

skewable_cel_typ get_skewable_cel(CCB *ccb);

void skew_cel(skewable_cel_typ_ptr skewable_cel);

void set_cel_position(CCB *ccb, int32 x, int32 y);

void reset_skewable_cel(skewable_cel_typ_ptr skewable_cel);

CCB *seek_cel_list(CCB *head, uint32 index);

#endif // CEL_HELPER_H