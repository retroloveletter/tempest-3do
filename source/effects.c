#include "effects.h"
#include "operamath.h"

// Full intensity R,G,B in standard system CLUT entry
#define STD_FULL_INTENSITY 0x00F8

void reset_screen_colors(ScreenContext *sc)
{
	int32 i;

	for (i = 0; i < sc->sc_NumScreens; i++)
		ResetScreenColors(sc->sc_ScreenItems[i]);
}

void apply_screen_luminance(ScreenContext *sc, uint32 lpercent)
{
	uint32 i;
	uint32 clut_entries[32];
	uint32 r, g, b;

	lpercent = DivSF16(lpercent << 16, 6553600);

	reset_screen_colors(sc);

	get_screen_clut_colors(sc->sc_ScreenItems[0], clut_entries);

	for (i = 0; i < 32; i++)
	{       
		r = COLOR24_TO_R8(clut_entries[i]) << 16;
		g = COLOR24_TO_G8(clut_entries[i]) << 16;
		b = COLOR24_TO_B8(clut_entries[i]) << 16;

		r = (r + MulSF16(r, lpercent)) >> 16;
		g = (g + MulSF16(g, lpercent)) >> 16;
		b = (b + MulSF16(b, lpercent)) >> 16;		

		if (r > 255) r = 255;
		if (g > 255) g = 255;
		if (b > 255) b = 255;

		clut_entries[i] = MakeCLUTColorEntry(i, (uint8) r, (uint8) g, (uint8) b);
	}

	for (i = 0; i < sc->sc_NumScreens; i++)
		SetScreenColors(sc->sc_ScreenItems[i], clut_entries, 32);
}

uint32 get_screen_clut_color(Item screen_item, uint32 index)
{
	uint32 vdl_word;

	Screen *screen = (Screen *) CheckItem(screen_item, NODE_GRAPHICS, TYPE_SCREEN);

	// Skip 4 header words and first control word
	vdl_word = (uint32) *(screen->scr_VDLPtr->vdl_DataPtr + 5 + index);

	// We only want the lower 24 bits (8 bits for R, G, and B)
	return(vdl_word & 0xFFFFFF);
}

void get_screen_clut_colors(Item screen_item, uint32 *clut_entries)
{
	uint32 i;

	for (i = 0; i < 32; i++)
		clut_entries[i] = get_screen_clut_color(screen_item, i);
}