#ifndef EFFECTS_H
#define EFFECTS_H

#include "displayutils.h"

#define COLOR24_TO_R8(c24) ((c24 >> 16) & 0xFF)
#define COLOR24_TO_G8(c24) ((c24 >> 8) & 0xFF)
#define COLOR24_TO_B8(c24) (c24 & 0xFF)

void apply_screen_luminance(ScreenContext *sc, uint32 lpercent);

void reset_screen_colors(ScreenContext *sc);

/**
 * Only use this function if your screens are using the simple VDL type (VDLTYPE_SIMPLE), not a custom one.
*/
uint32 get_screen_clut_color(Item screen_item, uint32 index);

void get_screen_clut_colors(Item screen_item, uint32 *clut_entries);

#endif // EFFECTS_H