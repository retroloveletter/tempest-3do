#include "cel_helper.h"

// 3DO includes
#include "stdio.h"
#include "celutils.h"
#include "string.h"

#define SKEW_AMOUNT 1200
#define SKEW_MAX_X 31000
#define SKEW_MIN_X -31000

static ubyte bpp_table[] = {0, 1, 2, 4, 6, 8, 16, 0};

/*  Passing (void*)1 into CreateCel() will prevent a data buffer from being allocated.
    Passing CREATECEL_UNCODED to CreateCel() will prevent a PLUT from being allocated.
    The CODED bit is then set later when preambles are built. If you will point the 
    returned cel's source and PLUT to an existing cel's data, pass FALSE for alloc. If
    you want source and PLUT space allocated for the returned cel, pass TRUE for alloc. */    

CCB *create_coded_cel8(uint32 width, uint32 height, Boolean alloc)
{
    CCB *ccb = NULL;

    if (alloc)
    {
        // Source and PLUT are allocated.
        ccb = CreateCel(width, height, 8, CREATECEL_CODED, NULL);
    }
    else 
    {
        // Source and PLUT are not allocated.
        ccb = CreateCel(width, height, 8, CREATECEL_UNCODED, (void*)1);
        ccb->ccb_SourcePtr = ccb->ccb_PLUTPtr = 0;
    }

    ccb->ccb_Flags = CCB_NPABS |    // NextPtr is an absolute ptr, not a relative one.
        CCB_SPABS	|	// SourcePtr is absolute ptr, not a relative one.
        CCB_PPABS	|	// PLUTPtr is absolute ptr, not a relative one.
        CCB_LDSIZE	|	// Load hdx-y and vdx-y from CCB.
        CCB_LDPRS	|	// Load ddx-y from CCB.
        CCB_CCBPRE	|	// Load the preamble words from CCB, opposed to from source data.
        CCB_YOXY	|	// Load X/Y pos from the CCB.
        CCB_ACW		|	// Pixels facing forward will be seen.
        CCB_ACCW	|	// Pixels facing backward will be seen.       
        CCB_ACE		|	// Enable both corner engines.
        CCB_LDPLUT  |   // Load PLUT from this cel.
        CCB_ALSC    |   // Line superclipping.
        CCB_LDPPMP;		// Load the PIXC word from CCB.

    ccb->ccb_Flags &= ~CCB_BGND; // Support 000 as transparent pixel.

    ccb->ccb_Width = width;
    ccb->ccb_Height = height;
    
    UNLAST_CEL(ccb);

    CLEAR_CEL_PREAMBLES(ccb); // This also marks the cel coded since it zeros the uncoded bit.

    SET_CEL_BPP8(ccb);
    SET_CEL_WOFFSET10(ccb, CALC_WOFFSET_BPP8(width));
    SET_CEL_TLHPCNT(ccb, width);
    SET_CEL_VCNT(ccb, height);

    return(ccb);
}

void print_cel(CCB *ccb)
{
    uint32 bpp = GET_CEL_BPP_VALUE(ccb);
   
    printf("** Basic fields **\n");

    printf("ccb_Width %d\n", ccb->ccb_Width);
    printf("ccb_Height %d\n", ccb->ccb_Height);

    printf("** Preamble 0 **\n");

    switch(bpp)
    {
    case 3:
        bpp = 4;
        break;            
    case 4:
        bpp = 6;
        break;
    case 5:
        bpp = 8;
        break;
    case 6:
        bpp = 16;
        break;
    default:
        break; // bpp = value already
    }

    printf("BPP %d\n", bpp);
    printf("Uncoded %d\n", GET_CEL_UNCODED(ccb));
    printf("VCNT %d\n", GET_CEL_VCNT(ccb));
    printf("Skipx %d\n", GET_CEL_SKIPX(ccb));
    printf("Rep8 %d\n", GET_CEL_REP8(ccb));

    printf("** Preamble 1 **\n");

    if (bpp == 8 || bpp == 16)
        printf("WOffset10 %d\n", GET_CEL_WOFFSET10(ccb));
    else 
        printf("WOffset8 %d\n", GET_CEL_WOFFSET8(ccb));

    printf("LRFORM %d\n", GET_CEL_LRFORM(ccb));

    printf("TLHPCNT %d\n\n", GET_CEL_TLHPCNT(ccb));
}

int32 get_cel_bpp(CCB *cel)
{
	return(bpp_table[GET_CEL_BPP_VALUE(cel)]);
}

int32 get_cel_row_bytes(CCB *cel)
{
	int32	woffset;
	uint32	pre1;

	pre1 = CEL_PRE1WORD(cel);

	if (get_cel_bpp(cel) < 8) 
		woffset = (pre1 & PRE1_WOFFSET8_MASK)  >> PRE1_WOFFSET8_SHIFT;
	else
        woffset = (pre1 & PRE1_WOFFSET10_MASK) >> PRE1_WOFFSET10_SHIFT;

	return ((woffset + PRE1_WOFFSET_PREFETCH) * sizeof(int32));

}

/*  The 3DO library contains GetCelDataBufferSize(), which this function was derived from. GetCelDataBufferSize() 
    does not compile correctly on the newer toolchain due to precedence differences. This version uses parenthesis
    to force the correct calculation order. */
int32 get_cel_src_bytes(CCB *cel)
{
	return((cel->ccb_Height * get_cel_row_bytes(cel)) + ((cel->ccb_Flags & CCB_CCBPRE) ? 0 : 8));
}

CCB *create_coded_colored_cel8(uint32 width, uint32 height, uint16 color)
{
    uint32 i;
    uint32 src_bytes;
    uint16 *pptr;
    CCB *cel = create_coded_cel8(width, height, TRUE);

    // cel->ccb_PLUTPtr = (void*) AllocMem(sizeof(uint16) * 32, MEMTYPE_CEL);
    pptr = (uint16*) cel->ccb_PLUTPtr;

    for (i = 0; i < 32; i++)
    {
        *pptr = color;
        pptr++;
    }
        
    src_bytes = get_cel_src_bytes(cel);
    // cel->ccb_SourcePtr = (void*) AllocMem(src_bytes, MEMTYPE_CEL);
    memset(cel->ccb_SourcePtr, 0, src_bytes);

    return(cel);
}

void set_cel_pal_to_color(CCB *cel, uint16 color)
{
    uint32 i;
    uint16 *pal = (uint16*) cel->ccb_PLUTPtr;

    for (i = 0; i < 32; i++)
        *pal++ = color;
}

cel8_data_typ_ptr cel_3do_to_cel8_data(CCB *ccb_3do)
{
    cel8_data_typ_ptr cel8_data = (cel8_data_typ_ptr) AllocMem(sizeof(cel8_data_typ), MEMTYPE_DRAM);

    cel8_data->source_bytes = get_cel_src_bytes(ccb_3do);

    // Copy source
    cel8_data->source = (ubyte*) AllocMem(cel8_data->source_bytes, MEMTYPE_DRAM);
    memcpy((void*)cel8_data->source, (void*)ccb_3do->ccb_SourcePtr, cel8_data->source_bytes);

    // Copy PLUT
    memcpy((void*)cel8_data->plut, (void*)ccb_3do->ccb_PLUTPtr, sizeof(uint16) * 32);
    
    return(cel8_data);    
}

void free_cel8_data(cel8_data_typ_ptr cel8_data)
{
    FreeMem(cel8_data->source, cel8_data->source_bytes);
    FreeMem(cel8_data, sizeof(cel8_data_typ));
}

void set_cel_subregion_bpp8(CCB *ccb, Rect *atlas, uint32 tex_width)
{
    ubyte *pixels;
    uint32 pixel_offset, word_offset, word_skipx;

    // Pixel_offset is in 1:1 space, as if each pixel was a matrix element.
    pixel_offset = XY_TO_LINEAR_OFFSET(atlas->rect_XLeft, atlas->rect_YTop, tex_width);

    // This figures out how many words away the pixel offset is.
    word_offset = XY_OFFSET_TO_WORD_BPP8(pixel_offset);

    // This figures out how many bit skips within the word to arrive at the pixel, if any.
    word_skipx = XY_OFFSET_TO_SKIPX_BPP8(pixel_offset);   

    // Use the above to adjust the source pointer.
    pixels = (ubyte*) ccb->ccb_SourcePtr;
    pixels = pixels + (word_offset << 2); // Same as multiplying by 4.
    ccb->ccb_SourcePtr = (CelData*) pixels;
    
    // Final adjustments to preamble.

    CLEAR_CEL_SKIPX(ccb);
    SET_CEL_SKIPX(ccb, word_skipx);

    CLEAR_CEL_TLHPCNT(ccb);
    SET_CEL_TLHPCNT(ccb, ((atlas->rect_XRight - atlas->rect_XLeft + 1) + word_skipx));
    
    CLEAR_CEL_VCNT(ccb);
    SET_CEL_VCNT(ccb, atlas->rect_YBottom - atlas->rect_YTop + 1);
    
    // print_cel(ccb);
}

void skew_cel(skewable_cel_typ_ptr skewable_cel)
{
    if (!skewable_cel->ccb)
        return;

    if (skewable_cel->skew_dir)
    {
        if ( (skewable_cel->ccb->ccb_VDX += SKEW_AMOUNT) > SKEW_MAX_X )
        {
            skewable_cel->ccb->ccb_VDX = SKEW_MAX_X;
            skewable_cel->skew_dir = 0;
        }
    }
    else 
    {
        if ( (skewable_cel->ccb->ccb_VDX -= SKEW_AMOUNT) < SKEW_MIN_X )
        {
            skewable_cel->ccb->ccb_VDX = SKEW_MIN_X;
            skewable_cel->skew_dir = 1;
        }
    }
}

skewable_cel_typ get_skewable_cel(CCB *ccb)
{
    skewable_cel_typ skewable_cel;

    if (ccb)
        skewable_cel.ccb = ccb;
    else 
        skewable_cel.ccb = 0;

    skewable_cel.skew_dir = 1;

    return(skewable_cel);
}

void set_cel_position(CCB *ccb, int32 x, int32 y)
{
    ccb->ccb_XPos = x << 16;
    ccb->ccb_YPos = y << 16;
}

void reset_skewable_cel(skewable_cel_typ_ptr skewable_cel)
{
    skewable_cel->ccb->ccb_VDX = 0;
    skewable_cel->skew_dir = 1;
}

CCB *seek_cel_list(CCB *head, uint32 index)
{
    uint32 i;
    CCB *ccb = head;

    for (i = 0; i < index; i++)
        ccb = ccb->ccb_NextPtr;

    return(ccb);
}