#include "app_globals.h"
#include "audi.h"
#include "resources.h"

// 3DO includes
#include "audio.h"
#include "stdio.h"
#include "kernel.h"
#include "soundfile.h"

/**
 * DSP ticks are DSP time units used during each frame of the DSP output.
 * The DSP can execute 565 ticks per 44,100 Hz frame.
 * 
 * fixedstereosample.dsp (16-bit 44100 Hz stereo)
 *      - 18 ticks and 16 words of memory, or 64 bytes.
 *      - 4 channels = 72 ticks and 64 words of memory, or 256 bytes.
 * 
 * mixer4x2.dsp (Four-channel stereo mixer).
 *      - 32 ticks
 *      - 32 words of memory
 * 
 * Total ticks: 104
 * Total memory: 384 bytes
*/

#define MAX_MIXER_CHANNELS 4
#define NUMBLOCKS 6
#define NUMBUFFS 4
#define BLOCKSIZE 3072
#define BUFSIZE (NUMBLOCKS * BLOCKSIZE)
#define DEFAULT_AMPLITUDE 0x7530

typedef struct mixer_typ 
{
    Item *instrument;
    Item lgain_knobs[4];
	Item rgain_knobs[4];
} mixer_typ;

typedef struct mixer_channel_typ
{
    Item *instrument;
    Item attachment;
    Item amp_knob;
    uint32 priority;
} mixer_channel_typ, *mixer_channel_typ_ptr;

// Mixer
static mixer_typ mixer;
static mixer_channel_typ mixer_channels[MAX_MIXER_CHANNELS];

static TagArg tags_start[2] = 
{
    {AF_TAG_SET_FLAGS, (void*)AF_ATTF_FATLADYSINGS},
    {TAG_END, 0}
};

// Possible return values for status: AF_STARTED, AF_RELEASED, AF_STOPPED, or AF_ABANDONED.
static TagArg tags_query[2] = 
{
    {AF_TAG_STATUS , (void*)0},
    {TAG_END, 0}
};

static Item music_ready_sig;
static Item music_task_item;
static Item parent_task_item;
static Item start_song_sig;
static Item stop_song_sig;
static SoundFilePlayer *sfp;
static int32 music_id;
static Boolean cycle_music;

static void music_player(void);

int32 init_audio_core(void)
{
    int32 i;
    Err err;
    mixer_channel_typ_ptr cptr;
    rez_envelope_typ rez_envelope;
    char buffer[16];

    try 
    {
        OpenAudioFolio();

        // Four-channel stereo mixer.
        err = load_resource("mixer4x2.dsp", REZ_INSTRUMENT, &rez_envelope);

        if (err < 0)
            throw("LoadInstrument (mixer4x2.dsp)", err);

        mixer.instrument = (Item*) rez_envelope.data;

        cptr = (mixer_channel_typ_ptr) mixer_channels;
     
        for (i = 0; i < MAX_MIXER_CHANNELS; i++)
        {
            cptr->attachment = -1;

            err = load_resource("fixedstereosample.dsp", REZ_INSTRUMENT, &rez_envelope);
            
            if (err < 0)
                throw("LoadInstrument (fixedstereosample.dsp)", err);

            cptr->instrument = (Item*) rez_envelope.data;

            // If you don't set the gains you won't hear anything from the mixer.

            sprintf(buffer, "LeftGain%d", i);
            mixer.lgain_knobs[i] = GrabKnob(*mixer.instrument, buffer);
            if (mixer.lgain_knobs[i] < 0) throw("GrabKnob (LeftGain)", mixer.lgain_knobs[i]);
            TweakKnob(mixer.lgain_knobs[i], 0x6000);

            sprintf(buffer, "RightGain%d", i);
            mixer.rgain_knobs[i] = GrabKnob(*mixer.instrument, buffer);
            if (mixer.rgain_knobs[i] < 0) throw("GrabKnob (RightGain)", mixer.rgain_knobs[i]);
            TweakKnob(mixer.rgain_knobs[i], 0x6000);

            cptr->amp_knob = GrabKnob(*cptr->instrument, "Amplitude");
            if (cptr->amp_knob < 0) throw("GrabKnob (Amplitude)", cptr->amp_knob);

            sprintf(buffer, "Input%d", i);

            err = ConnectInstruments(*cptr->instrument, "LeftOutput", *mixer.instrument, buffer);
            if (err < 0) throw("ConnectInstruments (LeftOutput)", err);

            err = ConnectInstruments(*cptr->instrument, "RightOutput", *mixer.instrument, buffer);
            if (err < 0) throw("ConnectInstruments (RightOutput)", err);

            cptr++; // Next
        }

        err = StartInstrument(*mixer.instrument, NULL);
        if (err < 0) throw("StartInstrument", err);
    }
    catch
    {
        // DUMP_3DO_ERR(__trycatch_msg, __trycatch_code);
        return(__trycatch_code);
    }

    return(0);
}

void service_sample_player(void)
{
    int32 i;
    mixer_channel_typ_ptr cptr = 0;

    cptr = (mixer_channel_typ_ptr) mixer_channels; // Start

    for (i = 0; i < MAX_MIXER_CHANNELS; i++)
    {
        // Look for busy instruments that are stopped.

        if (cptr->attachment > -1)
        {
            // This instrument has a sample attached to it. 
            GetAudioItemInfo(*cptr->instrument, tags_query);

            if ((int32)tags_query[0].ta_Arg == AF_STOPPED)
            {
                // Found a stopped channel. Clean it up.
                DetachSample(cptr->attachment);
                cptr->attachment = -1;
            }
        }

        cptr++;
    }
}

void play_sample(Item sample, uint32 priority, uint32 amplitude)
{
    int32 i;
   	
    mixer_channel_typ_ptr cptr = 0;     // Used to iterate through channels.
    mixer_channel_typ_ptr useptr = 0;   // Hold the channel to use.    

    cptr = (mixer_channel_typ_ptr) mixer_channels; // Start
    
    for (i = 0; i < MAX_MIXER_CHANNELS; i++)
    {
        if (cptr->attachment > -1)
        {
            // This instrument has a sample attached to it. 
            if (useptr == 0)
            {
                // We don't yet have a hijack backup.
                if (priority > cptr->priority)
                {
                    // If all other channels are busy then we can hijack this channel.
                    useptr = cptr;
                }
            }           
        }
        else 
        {
            // This channel is free. Use it. 
            useptr = cptr;
            break;           
        }

        cptr++; // Next channel.
    }

    if (useptr)
    {
        // We have a channel to use.
        if (useptr->attachment > -1)
        {
            // This instrument has a sample attached to it. 
            GetAudioItemInfo(*useptr->instrument, tags_query);

            switch((int32) tags_query[0].ta_Arg)
            {
            case AF_STARTED:
            case AF_RELEASED:
            case AF_ABANDONED:
                StopInstrument(*useptr->instrument, NULL);
                break;
            case AF_STOPPED: // No need to stop anything.
                break;
            default:
                break;
            }
            
            DetachSample(useptr->attachment);
            useptr->attachment = -1;
        }

        // Attach new sample and play it.
        useptr->attachment = AttachSample(*useptr->instrument, sample, NULL);
        useptr->priority = priority;
        SetAudioItemInfo(useptr->attachment, tags_start);

        if (amplitude > MAX_AMPLITUDE) // Clamp
            amplitude = MAX_AMPLITUDE;

        TweakKnob(useptr->amp_knob, amplitude);

        StartInstrument(*useptr->instrument, NULL);
    }    
}

void close_audio_core(void)
{
    int32 i;
    mixer_channel_typ_ptr cptr;
    rez_envelope_typ rez_envelope;

    StopInstrument(*mixer.instrument, NULL);
    rez_envelope.data = (void*) mixer.instrument;
    unload_resource(&rez_envelope, REZ_INSTRUMENT);

    cptr = (mixer_channel_typ_ptr) mixer_channels;
     
    for (i = 0; i < MAX_MIXER_CHANNELS; i++)
    {
        if (get_channel_status(i) != AF_STOPPED)
            StopInstrument(*cptr->instrument, NULL);

        rez_envelope.data = (void*) cptr->instrument;
        unload_resource(&rez_envelope, REZ_INSTRUMENT);
        cptr++;
    }

    CloseAudioFolio();
}

int32 get_channel_status(ubyte index)
{
    GetAudioItemInfo(*mixer_channels[index].instrument, tags_query);
    return((int32)tags_query[0].ta_Arg);
}

/* MUSIC */

void init_music_manager(void)
{
    parent_task_item = CURRENTTASK->t.n_Item;
    music_ready_sig = AllocSignal(0);
    music_task_item = CreateThread("music_player", KernelBase->kb_CurrentTask->t.n_Priority+1, music_player, 2048);
    WaitSignal(music_ready_sig);
}

/* MUSIC PLAYING THREAD */

void music_player(void)
{    
    int32 sigs_in = 0;
    int32 sigs_needed;
    int32 sig_result;
    Item *sfp_ins_ptr;
    char buffer[30];
    rez_envelope_typ rez_envelope;
    
	OpenAudioFolio();

    start_song_sig = AllocSignal(0);
    stop_song_sig = AllocSignal(0);

    sfp = CreateSoundFilePlayer(NUMBUFFS, BUFSIZE, 0);
    sfp->sfp_Flags |= SFP_NO_SAMPLER;

    load_resource("system/audio/dsp/fixedmonosample.dsp", REZ_INSTRUMENT, &rez_envelope);
    sfp_ins_ptr = (Item*) rez_envelope.data;
    sfp->sfp_SamplerIns = *sfp_ins_ptr;

    SendSignal(parent_task_item, music_ready_sig);
    
    // Run forever
    while (1) 
    {
        if (!cycle_music)
        {
            #if DEBUG_MODE
                printf("Entering music wait state.\n");
            #endif 

            WaitSignal(start_song_sig);
        }

        sprintf(buffer, "Assets/Audio/Music%d.aiff", music_id);

        #if DEBUG_MODE
            printf("Loading song %s\n", buffer);
        #endif

        lock_disc_drive();
        LoadSoundFile(sfp, buffer);         
        unlock_disc_drive();
        StartSoundFile(sfp, DEFAULT_AMPLITUDE);
        
        do
        {
            lock_disc_drive();
            ServiceSoundFile(sfp, sigs_in, &sigs_needed);
            unlock_disc_drive();

            if (sigs_needed)
            {
                sigs_in = WaitSignal(sigs_needed | stop_song_sig);

                // Check for music stop action
                if (sigs_in & stop_song_sig)
                {                    
                    cycle_music = FALSE;
                    break;
                }
            }
            else 
            {
                #if DEBUG_MODE 
                    printf("End of song\n");
                #endif

                // End of song
                ++music_id;

                if (music_id > 3)
                    music_id = 1;

                cycle_music = TRUE;

                break;
            }
        } while(sigs_needed);

        #if DEBUG_MODE 
            printf("Finished music playing loop.\n");
        #endif

        // Make sure nothing is left in the queue

        sig_result = GetCurrentSignals() & sigs_needed;
        if(sig_result) WaitSignal(sig_result);

        sig_result = GetCurrentSignals() & stop_song_sig;
        if(sig_result) WaitSignal(sig_result);  

        StopSoundFile(sfp);
        UnloadSoundFile(sfp);
        ScavengeMem();
    }    
}

void start_music(void)
{
    music_id = 1;
    cycle_music = FALSE;
    SendSignal(music_task_item, start_song_sig);
}

void stop_music(void)
{
    cycle_music = FALSE;
    SendSignal(music_task_item, stop_song_sig);
}