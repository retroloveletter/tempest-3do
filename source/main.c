#include "game_globals.h"
#include "gs_play.h"
#include "audi.h"
#include "levels.h"

// 3DO includes
#include "graphics.h"
#include "operamath.h"
#include "audio.h"
#include "mem.h"
#include "stdio.h"
#include "timerutils.h"
#include "event.h"

void setup_core_display(void)
{
	sc = (ScreenContext*) AllocMem(sizeof(ScreenContext), MEMTYPE_ANY);

	QueryGraphics(QUERYGRAF_TAG_DEFAULTDISPLAYTYPE, (void*) &display_type);

	#if 0
		if ((display_type == DI_TYPE_PAL1) || (display_type == DI_TYPE_PAL2)) 
		{
			display_type = DI_TYPE_PAL2; 
			display_width = 384;
			display_height = 288;
		}
		else 
		{
			display_type = DI_TYPE_NTSC;
			display_width = 320;
			display_height = 240;
		}
	#else 
		if (display_type != DI_TYPE_NTSC)
		{
			printf("Error - System not NTSC.\n");
			exit(0);
		}

		display_width = 320;
		display_height = 240;
	#endif

	display_width2 = display_width / 2;
	display_height2 = display_height / 2;
	display_width2_f16 = display_width2 << FRACBITS_16;
	display_height2_f16 = display_height2 << FRACBITS_16;
	
	#if DEBUG_MODE
		if (display_type == DI_TYPE_NTSC)
			printf("Creating NTSC screen context\n");
		else 
			printf("Creating PAL screen context\n");
	#endif

	CreateBasicDisplay(sc, display_type, 2);
	sc->sc_CurrentScreen = 0;
}

void setup_core_ioreqs(void)
{
	vbl_io = GetVBLIOReq();
	sport_io = GetVRAMIOReq();
	time_io = GetTimerIOReq();
}

/* PUBLIC */

void init_core(void)
{
	uint32 i;

	// Open folios
	OpenGraphicsFolio();
	OpenMathFolio();
	
	// Init audio
	init_audio_core();

	setup_core_display();
	setup_core_ioreqs();	

	// Init controls
	InitEventUtility(1, 1, LC_ISFOCUSED);

	// Enable super clipping

	for (i = 0 ; i < sc->sc_NumScreens ; i++)
	{
		// Only touch bit 26 (ASCALL on or off)
		if (SetCEControl(sc->sc_BitmapItems[i], 0xFFFFFFFF, (1 << 26)) != 0)
		{
			#if DEBUG_MODE 
				printf("Error - Failed enabling super clipping.\n");
			#endif
		}
	}

	// Seed random number generator with hardware random number
	srand(ReadHardwareRandomNumber());
}

int main(int argc, char *argv[])
{
	init_core();
	init_app();

	do
	{
		run_gstate_loop(play_start, play_update, play_stop);
	} while(1); // This outer loop never ends

	return(0);
}
