#ifndef AUDI_H_
#define AUDI_H_

// 3DO includes
#include "types.h"

#define DEFAULT_AUDIO_PRIORITY 100
#define DEFAULT_AUDIO_AMPLITUDE 0x3FFF
#define MAX_AMPLITUDE 0x7FFF

int32 init_audio_core(void);

void close_audio_core(void);

/**
 * @brief Play audio sample file.
 * 
 * Valid amplitude range 0..0x7fff.
 * 
 * @param sample 
 * @param priority 
 * @param amplitude 
 */
void play_sample(Item sample, uint32 priority, uint32 amplitude);

void service_sample_player(void);

/**
 * Returns status level for instrument.
 *  - AF_ABANDONED
 *  - AF_STOPPED
 *  - AF_RELEASED
 *  - AF_STARTED
*/
int32 get_channel_status(ubyte index);

void init_music_manager(void);

void start_music(void);

void stop_music(void);

#endif // AUDI_H_
