/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h> 
#include <string.h>
#include <ctype.h>

#include "platform/i_sound.h"
#include "platform/s_midi.h"
#include "fix/fix.h"
#include "object.h"
#include "platform/mono.h"
#include "platform/timer.h"
#include "platform/joy.h"
#include "digi.h"
#include "sounds.h"
#include "args.h"
#include "platform/key.h"
#include "newdemo.h"
#include "game.h"
#include "misc/error.h"
#include "wall.h"
#include "cfile/cfile.h"
#include "piggy.h"
#include "text.h"
#include "kconfig.h"
#include <vector>

#define DIGI_PAUSE_BROKEN 1		//if this is defined, don't pause MIDI songs

#define _DIGI_SAMPLE_FLAGS (_VOLUME | _PANNING )

#define _DIGI_MAX_VOLUME (16384)	//16384

// patch files
#define  _MELODIC_PATCH       "melodic.bnk"
#define  _DRUM_PATCH          "drum.bnk"
#define  _DIGDRUM_PATCH       "drum32.dig"

static int	Digi_initialized = 0;
static int	digi_atexit_called = 0;			// Set to 1 if atexit(digi_close) was called

int digi_driver_board = 0;
int digi_driver_port = 0;
int digi_driver_irq = 0;
int digi_driver_dma = 0;
int digi_midi_type = 0;			// Midi driver type
int digi_midi_port = 0;			// Midi driver port
static int digi_max_channels = 8;
static int digi_driver_rate = 11025;			// rate to use driver at
static int digi_dma_buffersize = 4096;			// size of the dma buffer to use (4k)
int digi_timer_rate = 9943;			// rate for the timer to go off to handle the driver system (120 Hz)
int digi_lomem = 0;
static int digi_volume = _DIGI_MAX_VOLUME;		// Max volume
static int midi_volume = 128 / 2;						// Max volume
static int midi_system_initialized = 0;
static int digi_system_initialized = 0;
static int timer_system_initialized = 0;
static int digi_sound_locks[MAX_SOUNDS];
char digi_last_midi_song[16] = "";
char digi_last_melodic_bank[16] = "";
char digi_last_drum_bank[16] = "";
char* digi_driver_path = NULL;//Was _NULL -KRB

//[ISB] not windows types
static uint32_t						hSOSDigiDriver = 0xffff;			// handle for the SOS driver being used 
static uint32_t     				hSOSMidiDriver = 0xffff;			// handle for the loaded MIDI driver
static uint32_t						hTimerEventHandle = 0xffff;		// handle for the timer function

static void* lpInstruments = NULL;		// pointer to the instrument file
static int InstrumentSize = 0;
static void* lpDrums = NULL;				// pointer to the drum file
static int DrumSize = 0;
// track mapping structure, this is used to map which track goes
// out which device. this can also be mapped by the name of the 
// midi track. to map by the name of the midi track use the define
// _MIDI_MAP_TRACK for each of the tracks 
/*
static _SOS_MIDI_TRACK_DEVICE   sSOSTrackMap = {
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK,
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK,
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK,
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK,
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK,
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK,
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK,
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK
};
*/
// handle for the initialized MIDI song
uint32_t     wSongHandle = 0xffff;
uint8_t* SongData = NULL;
int		SongSize;


#define SOF_USED				1 		// Set if this sample is used
#define SOF_PLAYING			2		// Set if this sample is playing on a channel
#define SOF_LINK_TO_OBJ		4		// Sound is linked to a moving object. If object dies, then finishes play and quits.
#define SOF_LINK_TO_POS		8		// Sound is linked to segment, pos
#define SOF_PLAY_FOREVER	16		// Play forever (or until level is stopped), otherwise plays once
#define SOF_LOW_PRIORITY	32		// Should continue playing if possible, but can be stopped if a new sound needs a slot. 

typedef struct sound_object 
{
	short			signature;		// A unique signature to this sound
	uint8_t			flags;			// Used to tell if this slot is used and/or currently playing, and how long.
	fix			max_volume;		// Max volume that this sound is playing at
	fix			max_distance;	// The max distance that this sound can be heard at...
	int			volume;			// Volume that this sound is playing at
	//int			pan;				// Pan value that this sound is playing at
	vms_vector		pos;
	vms_vector		vel;
	uint32_t			handle;			// What handle this sound is playing on.  Valid only if SOF_PLAYING is set.
	short			soundnum;		// The sound number that is playing
	union {
		struct {
			short			segnum;				// Used if SOF_LINK_TO_POS field is used
			short			sidenum;
			vms_vector	position;
		};
		struct {
			short			objnum;				// Used if SOF_LINK_TO_OBJ field is used
			short			objsignature;
		};
	};
} sound_object;

typedef struct
{
	int length;
	int volume;
	int loop;
	int global;
	int hasVelocity;
	fix maxDistance;
	vms_vector startpos;
	vms_vector velocity;

	uint8_t* data;
	
} sampledata_t;

//[ISB] Need more sound objects, otherwise buildup occurs especially when levels have lots of fans. 
#define MAX_SOUND_OBJECTS 24
sound_object SoundObjects[MAX_SOUND_OBJECTS];
short next_signature = 0;

int digi_sounds_initialized = 0;

void digi_reset_digi_sounds();

int digi_xlat_sound(int soundno)
{
		if ( soundno < 0 ) return -1;

		if ( digi_lomem )	
		{
			soundno = AltSounds[soundno];
			if ( soundno == 255 ) return -1;
		}
		return Sounds[soundno];
}


void digi_close_midi()
{
	if (digi_midi_type>0)	
	{
		if (wSongHandle < 0xffff)	
		{
			// stop the last MIDI song from playing
			//sosMIDIStopSong( wSongHandle );
			S_StopSong();
			// uninitialize the last MIDI song
			//sosMIDIUnInitSong( wSongHandle );
			wSongHandle = 0xffff;
		}
		if (SongData) 	
		{
			//if (!dpmi_unlock_region(SongData, SongSize))	
			//{
			//	mprintf( (1, "Error unlocking midi file" ));
			//}
			free(SongData);
			SongData = NULL;
		}
		// reset the midi driver and
		// uninitialize the midi driver and tell it to free the memory allocated
		// for the driver
		//if ( hSOSMidiDriver < 0xffff )	
		//{
		//	sosMIDIResetDriver( hSOSMidiDriver );
		//	sosMIDIUnInitDriver( hSOSMidiDriver, _TRUE  );
		//	hSOSMidiDriver = 0xffff;
		//}
	
		if ( midi_system_initialized )	
		{
			// uninitialize the MIDI system
			//sosMIDIUnInitSystem();
			S_ShutdownMusic();
			midi_system_initialized = 0;
		}
	}
}

void digi_close_digi()
{
	if (digi_driver_board>0)	
	{
		I_ShutdownAudio();
		if ( hTimerEventHandle < 0xffff )	
		{
			//sosTIMERRemoveEvent( hTimerEventHandle );
			hTimerEventHandle = 0xffff;
		}
		//if ( hSOSDigiDriver < 0xffff )
			//sosDIGIUnInitDriver( hSOSDigiDriver, _TRUE, _TRUE );
	}
}


void digi_close()
{
	if (!Digi_initialized) return;
	Digi_initialized = 0;

	if ( timer_system_initialized )	
	{
		// Remove timer...
		//timer_set_function( NULL );
	}

	digi_close_midi();
	digi_close_digi();

	if ( digi_system_initialized )	
	{
		// uninitialize the DIGI system
		//sosDIGIUnInitSystem();
		digi_system_initialized = 0;
	}

	if ( timer_system_initialized )	
	{
		// Remove timer...
		timer_system_initialized = 0;
		//sosTIMERUnInitSystem( 0 );
	}
}

//extern int loadpats(char* filename);

int digi_load_fm_banks(char* melodic_file, char* drum_file)
{
	return 0;

}

int digi_init_midi()
{
	if (digi_midi_type > 0)
	{
		int res = S_InitMusic(digi_midi_type);
		if (!res) midi_system_initialized = 1;
		return res;
	}

	return 0;
}

int digi_init_digi()
{
	return 0;
}

int digi_init()
{
	if (digi_driver_board == 0)
		digi_driver_board = 1; //[ISB] hackhack
	if (digi_midi_type == 0)
		digi_midi_type = _MIDI_GEN; //[ISB] hackhackhack
	int i;
#ifdef USE_CD
	{
		FILE * fp;
		fp = fopen( "hmimdrv.386", "rb" );
		if ( fp )
			fclose(fp);
		else
			digi_driver_path = destsat_cdpath;
	}
#endif

	if ( FindArg( "-nomusic" ))
		digi_midi_type = 0;

	if ( (digi_midi_type<1) && (digi_driver_board<1) )
		return 0;

	/*if ( !FindArg( "-noloadpats" ) )	
	{
		if ( digi_midi_type == _MIDI_GUS )	
		{
			char fname[128];
			strcpy( fname, "DESCENTG.INI" );
			loadpats(fname);
		}
	}*/
	Digi_initialized = 1;

	// initialize the DIGI system and lock down all SOS memory
	//sosDIGIInitSystem( digi_driver_path, _SOS_DEBUG_NORMAL );
	i = I_InitAudio();
	if (i)
	{
		Warning("Cannot initalize sound library\n");
		return 1;
	}
	digi_system_initialized = 1;

	// Initialize timer, where we will call it from out own interrupt
	// routine, and we will call DOS 18.2Hz ourselves.
	timer_system_initialized = 1;

	if (digi_init_digi()) return 1;
	if (digi_init_midi()) return 1;

	if (!digi_atexit_called)	
	{
		atexit(digi_close);
		digi_atexit_called = 1;
	}

	digi_init_sounds();
	digi_set_midi_volume(midi_volume);

	for (i=0; i<MAX_SOUNDS; i++ )
		digi_sound_locks[i] = 0;
	digi_reset_digi_sounds();

	return 0;
}

// Toggles sound system on/off
void digi_reset()
{
	if ( Digi_initialized )	
	{
		digi_reset_digi_sounds();
		digi_close();
		mprintf( (0, "Sound system DISABLED.\n" ));
	} 
	else 
	{
		digi_init();
		mprintf( (0, "Sound system ENABLED.\n" ));
	}
}

int digi_total_locks = 0;

//[ISB] In practice, in this modern era of linear address spaces and sound APIs,
//these aren't needed, but for now I'm keeping them in case they do important bookkeeping.
//I guess. 
uint8_t* digi_lock_sound_data(int soundnum)
{
	if ( !Digi_initialized ) return NULL;
	if ( digi_driver_board <= 0 )	return NULL;

	if ( digi_sound_locks[soundnum] == 0 )	
	{
		digi_total_locks++;
	}
	digi_sound_locks[soundnum]++;
	return GameSounds[soundnum].data;
}

void digi_unlock_sound_data(int soundnum)
{
	if ( !Digi_initialized ) return;
	if ( digi_driver_board <= 0 )	return;

	Assert( digi_sound_locks[soundnum] > 0 );

	if ( digi_sound_locks[soundnum] == 1 )	
	{
		digi_total_locks--;
		//mprintf(( 0, "Total sound locks = %d\n", digi_total_locks ));
		//i = dpmi_unlock_region( GameSounds[soundnum].data, GameSounds[soundnum].length );
		//if ( !i ) Error( "Error unlocking sound %d\n", soundnum );
	}
	digi_sound_locks[soundnum]--;
}

static int next_handle = 0;
static uint16_t SampleHandles[32] = { 0xffff, 0xffff, 0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff };
static int SoundNums[32] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
static int32_t SoundVolumes[32] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };

void digi_reset_digi_sounds()
{
	int i;

	if ( !Digi_initialized ) return;
	if ( digi_driver_board <= 0 )	return;

	for (i=0; i<32; i++ )	
	{
		if (SampleHandles[i] < _MAX_VOICES )	
		{
			//if ( !sosDIGISampleDone( hSOSDigiDriver, SampleHandles[i])  )		
			if (!I_CheckSoundPlaying(SampleHandles[i]))
			{
				//mprintf(( 0, "Stopping sound %d.\n", next_handle ));
				//sosDIGIStopSample(hSOSDigiDriver, SampleHandles[i]);
				I_StopSound(SampleHandles[i]);
			}
			SampleHandles[i] = 0xffff;
		}
		if ( SoundNums[i] != -1 )	
		{
			digi_unlock_sound_data(SoundNums[i]);
			SoundNums[i] = -1;
		}
	}
	for (i=0; i<MAX_SOUNDS; i++ )	
	{
		Assert(digi_sound_locks[i] == 0);
	}
}

void reset_sounds_on_channel(int channel)
{
	int i;

	if ( !Digi_initialized ) return;
	if ( digi_driver_board <= 0 )	return;

	for (i=0; i<digi_max_channels; i++ )	
	{
		if (SampleHandles[i] == channel )	
		{
			SampleHandles[i] = 0xffff;
			if ( SoundNums[i] != -1 )	
			{
				digi_unlock_sound_data(SoundNums[i]);
				SoundNums[i] = -1;
			}
		}
	}
}


void digi_set_max_channels(int n)
{
	digi_max_channels = n;

	if (digi_max_channels < 1)
		digi_max_channels = 1;
	if (digi_max_channels > 32)
		digi_max_channels = 32;

	if ( !Digi_initialized ) return;
	if ( digi_driver_board <= 0 )	return;

	digi_reset_digi_sounds();
}

int digi_get_max_channels()
{
	return digi_max_channels;
}

uint16_t digi_start_sound(sampledata_t* sampledata, short soundnum)
{
	int i, ntries;
	uint16_t sHandle = 0xFFFF;

	if (!Digi_initialized) return 0xFFFF;
	if (digi_driver_board <= 0)	return 0xFFFF;

	soundnum  = soundnum;
	ntries = 0;

TryNextChannel:
	if ((SampleHandles[next_handle] < _MAX_VOICES)  && !I_CheckSoundDone(SampleHandles[next_handle]))		
	{
		if ( (SoundVolumes[next_handle] > (uint32_t)digi_volume) && (ntries<digi_max_channels) )	
		{
			//mprintf(( 0, "Not stopping loud sound %d.\n", next_handle ));
			next_handle++;
			if ( next_handle >= digi_max_channels )
				next_handle = 0;
			ntries++;
			goto TryNextChannel;
		}
		//mprintf(( 0, "[SS:%d]", next_handle ));
		I_StopSound(SampleHandles[next_handle]);
		SampleHandles[next_handle] = 0xffff;
	}

	if ( SoundNums[next_handle] != -1 )	
	{
		digi_unlock_sound_data(SoundNums[next_handle]);
		SoundNums[next_handle] = -1;
	}

	digi_lock_sound_data(soundnum);
	sHandle = I_GetSoundHandle();
	if (sHandle == _ERR_NO_SLOTS)
	{
		mprintf(( 1, "NOT ENOUGH SOUND SLOTS!!!\n" ));
		digi_unlock_sound_data(soundnum);
		return sHandle;
	}
	I_SetSoundData(sHandle, sampledata->data, sampledata->length, 11025);
	if (sampledata->global)
	{
		I_SetVolume(sHandle, sampledata->volume);
		I_SetUISound(sHandle);
	}
	else
	{
		I_SetSoundInformation(sHandle, sampledata->volume, &sampledata->startpos);
		I_SetRolloff(sHandle, f2fl(sampledata->maxDistance));
		if (sampledata->hasVelocity)
			I_SetVelocity(sHandle, &sampledata->velocity);
	}
	I_PlaySound(sHandle, sampledata->loop);
	//mprintf(( 0, "Starting sound on channel %d\n", sHandle ));

	for (i=0; i<digi_max_channels; i++ )	
	{
		if (SampleHandles[i] == sHandle )	
		{
			SampleHandles[i] = 0xffff;
			if ( SoundNums[i] != -1 )	
			{
				digi_unlock_sound_data(SoundNums[i]);
				SoundNums[i] = -1;
			}
		}
	}

	SampleHandles[next_handle] = sHandle;
	SoundNums[next_handle] = soundnum;
	SoundVolumes[next_handle] = sampledata->volume;
//	mprintf(( 0, "Starting sample %d at volume %d\n", next_handle, sampledata->wVolume  ));
	if (SoundVolumes[next_handle] > (uint32_t)digi_volume)
		mprintf(( 0, "Starting loud sample %d\n", next_handle ));
	next_handle++;
	if ( next_handle >= digi_max_channels )
		next_handle = 0;
	//mprintf(( 0, "%d samples playing\n", sosDIGISamplesPlaying(hSOSDigiDriver) ));
	return sHandle;
}

//[ISB] this isn't called ever, which saves me having to do the bookkeeping for it. Yay.
int digi_is_sound_playing(int soundno)
{
	/*uint16_t SampleHandle = 0xFFFF;
	soundno = digi_xlat_sound(soundno);

	if (!Digi_initialized) return 0;
	if (digi_driver_board<1) return 0;

	if (soundno < 0 ) return 0;
	if (GameSounds[soundno].data==NULL)
	{
		Int3();
		return 0;
	}

	//SampleHandle = sosDIGIGetSampleHandle( hSOSDigiDriver, soundno );
	if ((SampleHandle < _MAX_VOICES) && (!sosDIGISampleDone( hSOSDigiDriver, SampleHandle))) //[ISB] todo
		return 1;*/
	return 0;
}

void digi_play_sample_once(int soundno, fix max_volume)
{
	uint16_t SampleHandle;
	digi_sound *snd;
	sampledata_t DigiSampleData;
	//_SOS_START_SAMPLE sSOSSampleData;

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_sound( soundno );
	soundno = digi_xlat_sound(soundno);

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;
	if (soundno < 0 ) return;
	snd = &GameSounds[soundno];
	if (snd==NULL) 
	{
		Int3();
		return;
	}

	//SampleHandle = sosDIGIGetSampleHandle( hSOSDigiDriver, soundno );
	SampleHandle = I_GetSoundHandle();
	//if ( (SampleHandle < _MAX_VOICES) && (!sosDIGISampleDone( hSOSDigiDriver, SampleHandle)) )	
	if ((SampleHandle < _MAX_VOICES) && (!I_CheckSoundDone(SampleHandle)))
	{
		//sosDIGIStopSample(hSOSDigiDriver, SampleHandle);
		I_StopSound(SampleHandle);
		//while ( !sosDIGISampleDone( hSOSDigiDriver, SampleHandle));
		//return;
	}
	
	memset(&DigiSampleData, 0, sizeof(sampledata_t));
	DigiSampleData.global = 1;
	DigiSampleData.volume = fixmuldiv(max_volume, digi_volume, F1_0);
	DigiSampleData.data = snd->data;
	DigiSampleData.length = snd->length;

//	if ( sosDIGISamplesPlaying(hSOSDigiDriver) >= digi_max_channels )
//		return;

	// start the sample playing
	digi_start_sound(&DigiSampleData, soundno);
}


void digi_play_sample(int soundno, fix max_volume)
{
	digi_sound *snd;
	sampledata_t DigiSampleData;
	//_SOS_START_SAMPLE sSOSSampleData;

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_sound( soundno );
	soundno = digi_xlat_sound(soundno);

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;
	if (soundno < 0 ) return;
	snd = &GameSounds[soundno];
	if (snd==NULL) 
	{
		Int3();
		return;
	}

	//mprintf( (0, "Playing sample of length %d\n", snd->length ));

	memset(&DigiSampleData, 0, sizeof(sampledata_t));
	DigiSampleData.global = 1;
	DigiSampleData.volume = fixmuldiv(max_volume, digi_volume, F1_0);
	DigiSampleData.data = snd->data;
	DigiSampleData.length = snd->length;

//	if ( sosDIGISamplesPlaying(hSOSDigiDriver) >= digi_max_channels )
//		return;

	// start the sample playing
	digi_start_sound(&DigiSampleData, soundno);
}


void digi_play_sample_3d(int soundno, int angle, int volume, int no_dups)
{
	//_SOS_START_SAMPLE sSOSSampleData;
	sampledata_t DigiSampleData;
	digi_sound *snd;

	no_dups = 1;

	if ( Newdemo_state == ND_STATE_RECORDING )		
	{
		if ( no_dups )
			newdemo_record_sound_3d_once( soundno, angle, volume );
		else
			newdemo_record_sound_3d( soundno, angle, volume );
	}
	soundno = digi_xlat_sound(soundno);

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;
	if (soundno < 0 ) return;
	snd = &GameSounds[soundno];
	if (snd==NULL)
	{
		Int3();
		return;
	}

	if (volume < 10) return;

	/*memset( &sSOSSampleData, 0, sizeof(_SOS_START_SAMPLE));
	sSOSSampleData.wLoopCount 		= 0x00;
	sSOSSampleData.wChannel 		= _CENTER_CHANNEL;
	sSOSSampleData.wSampleID		= soundno;
	sSOSSampleData.dwSampleSize 	= ( LONG )snd->length;
	sSOSSampleData.dwSampleByteLength 	= ( LONG )snd->length;
	sSOSSampleData.lpSamplePtr		= snd->data;
	sSOSSampleData.lpCallback		= _NULL;		//sosDIGISampleCallback;
	sSOSSampleData.wSamplePanLocation	= angle;			// 0 - 0xFFFF
	sSOSSampleData.wSamplePanSpeed 		= 0;
	sSOSSampleData.wVolume					= fixmuldiv(volume,digi_volume,F1_0);;					// 0 - 0x7fff
	sSOSSampleData.wSampleFlags			= _DIGI_SAMPLE_FLAGS;*/

	memset(&DigiSampleData, 0, sizeof(sampledata_t));
	//DigiSampleData.angle = 0xFFFF / 2;
	//DigiSampleData.angle = angle;
	DigiSampleData.volume = fixmuldiv(volume, digi_volume, F1_0);
	DigiSampleData.data = snd->data;
	DigiSampleData.length = snd->length;

	// start the sample playing
	digi_start_sound(&DigiSampleData, soundno);
}

void digi_play_sample_3d_pos(int soundno, vms_vector *pos, int volume, int no_dups)
{
	//_SOS_START_SAMPLE sSOSSampleData;
	sampledata_t DigiSampleData;
	digi_sound* snd;

	no_dups = 1;

	soundno = digi_xlat_sound(soundno);

	if (!Digi_initialized) return;
	if (digi_driver_board < 1) return;
	if (soundno < 0) return;
	snd = &GameSounds[soundno];
	if (snd == NULL)
	{
		Int3();
		return;
	}

	if (volume < 10) return;

	/*memset( &sSOSSampleData, 0, sizeof(_SOS_START_SAMPLE));
	sSOSSampleData.wLoopCount 		= 0x00;
	sSOSSampleData.wChannel 		= _CENTER_CHANNEL;
	sSOSSampleData.wSampleID		= soundno;
	sSOSSampleData.dwSampleSize 	= ( LONG )snd->length;
	sSOSSampleData.dwSampleByteLength 	= ( LONG )snd->length;
	sSOSSampleData.lpSamplePtr		= snd->data;
	sSOSSampleData.lpCallback		= _NULL;		//sosDIGISampleCallback;
	sSOSSampleData.wSamplePanLocation	= angle;			// 0 - 0xFFFF
	sSOSSampleData.wSamplePanSpeed 		= 0;
	sSOSSampleData.wVolume					= fixmuldiv(volume,digi_volume,F1_0);;					// 0 - 0x7fff
	sSOSSampleData.wSampleFlags			= _DIGI_SAMPLE_FLAGS;*/

	memset(&DigiSampleData, 0, sizeof(sampledata_t));
	DigiSampleData.startpos = *pos;
	DigiSampleData.volume = fixmuldiv(volume, digi_volume, F1_0);
	DigiSampleData.data = snd->data;
	DigiSampleData.length = snd->length;

	// start the sample playing
	digi_start_sound(&DigiSampleData, soundno);
}

void digi_set_midi_volume(int mvolume)
{
	int old_volume = midi_volume;

	if ( mvolume > 127 )
		midi_volume = 127;
	else if ( mvolume < 0 )
		midi_volume = 0;
	else
		midi_volume = mvolume;

	if (digi_midi_type > 0)	
	{
		if ((old_volume < 1) && (midi_volume > 1))	
		{
			if (wSongHandle == 0xffff )
				digi_play_midi_song(digi_last_midi_song, digi_last_melodic_bank, digi_last_drum_bank, 1);
		}
		//sosMIDISetMasterVolume(midi_volume);
		I_SetMusicVolume(midi_volume);
	}
	
}

void digi_set_digi_volume(int dvolume)
{
	dvolume = fixmuldiv( dvolume, _DIGI_MAX_VOLUME, 0x7fff);
	if ( dvolume > _DIGI_MAX_VOLUME )
		digi_volume = _DIGI_MAX_VOLUME;
	else if ( dvolume < 0 )
		digi_volume = 0;
	else
		digi_volume = dvolume;

	if ( !Digi_initialized ) return;
	if ( digi_driver_board <= 0 )	return;

	digi_sync_sounds();
}


// 0-0x7FFF 
void digi_set_volume(int dvolume, int mvolume)
{
	digi_set_midi_volume( mvolume );
	digi_set_digi_volume( dvolume );
//	mprintf(( 1, "Volume: 0x%x and 0x%x\n", digi_volume, midi_volume ));
}

void digi_stop_current_song()
{
	StopHQSong();

	//[ISB] TODO MIDI
	// Stop last song...
	if (wSongHandle < 0xffff )	
	{
		// stop the last MIDI song from playing
		//sosMIDIStopSong( wSongHandle );
		S_StopSong();
		// uninitialize the last MIDI song
		//sosMIDIUnInitSong( wSongHandle );
		wSongHandle = 0xffff;
	}
	if (SongData) 	
	{
		/*if (!dpmi_unlock_region(SongData, SongSize))	
		{
			mprintf( (1, "Error unlocking midi file" ));
		}*/
		free(SongData);
		SongData = NULL;
	}
}


void digi_play_midi_song(char* filename, char* melodic_bank, char* drum_bank, int loop)
{
	char fname[128];
	uint16_t wError;                 // error code returned from functions
	CFILE *fp;

	// structure to pass sosMIDIInitSong
	//_SOS_MIDI_INIT_SONG     sSOSInitSong;

	if (!Digi_initialized) return;
	if (digi_midi_type <= 0)	return;

	digi_stop_current_song();

	if (filename == NULL)	return;

	if (PlayHQSong(filename, loop)) //[ISB] moved here to prevent a null pointer problem
		return;

	strcpy(digi_last_midi_song, filename);
	strcpy(digi_last_melodic_bank, melodic_bank);
	strcpy(digi_last_drum_bank, drum_bank);

	fp = NULL;

	if ((digi_midi_type==_MIDI_FM)||(digi_midi_type==_MIDI_OPL3))
	{
		int sl;
		sl = strlen(filename);
		strcpy(fname, filename);
		fname[sl-1] = 'q';
		fp = cfopen(fname, "rb");
	}

	if (!fp)	
	{
		fp = cfopen( filename, "rb" );
		if (!fp) 
		{
			mprintf((1, "Error opening midi file, '%s'", filename));
			return;
		}
	}
	if ( midi_volume < 1 )		
	{
		cfclose(fp);
		return;				// Don't play song if volume == 0;
	}
	SongSize = cfilelength(fp);
	SongData = (uint8_t*)malloc(SongSize);

	if (SongData==NULL)
	{
		cfclose(fp);
		mprintf((1, "Error mallocing %d bytes for '%s'", SongSize, filename));
		return;
	}
	if (cfread (SongData, SongSize, 1, fp)!=1)	
	{
		mprintf((1, "Error reading midi file, '%s'", filename));
		cfclose(fp);
		free(SongData);
		SongData=NULL;
		return;
	}
	cfclose(fp);

	if ( (digi_midi_type==_MIDI_FM)||(digi_midi_type==_MIDI_OPL3) )	
	{
		if (!digi_load_fm_banks(melodic_bank, drum_bank))	
		{
			return;
		}
	}


	//initialize the song
	//if((wError = sosMIDIInitSong( &sSOSInitSong, &sSOSTrackMap, &wSongHandle )))
	if ((wError = S_StartSong(SongSize, SongData, (bool)loop, &wSongHandle)))
	{
		//mprintf((1, "\nHMI Error : %s", sosGetErrorString( wError )));
		free(SongData);
		SongData=NULL;
		return;
	}

	Assert( wSongHandle == 0 );	   
}

int digi_get_sound_audible(vms_matrix* listener, vms_vector* listener_pos, int listener_seg, vms_vector* sound_pos, int sound_seg, fix max_distance)
{
	vms_vector	vector_to_sound;
	fix angle_from_ear, cosang, sinang;
	fix distance;
	fix path_distance;
	int volume;

	//max_distance = (max_distance * 5) / 4;		// Make all sounds travel 1.25 times as far.

	//	Warning: Made the vm_vec_normalized_dir be vm_vec_normalized_dir_quick and got illegal values to acos in the fang computation.
	distance = vm_vec_normalized_dir_quick(&vector_to_sound, sound_pos, listener_pos);

	if (distance < max_distance)
	{
		int num_search_segs = f2i(max_distance / 20);
		if (num_search_segs < 1) num_search_segs = 1;

		path_distance = find_connected_distance(listener_pos, listener_seg, sound_pos, sound_seg, num_search_segs, WID_RENDPAST_FLAG);
		if (path_distance > -1)
		{
			volume = F1_0 - fixdiv(path_distance, max_distance);
			//mprintf( (0, "Sound path distance %.2f, volume is %d / %d\n", f2fl(distance), *volume, max_volume ));
			if (volume > 0)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
	}

	return 0;
}

void digi_get_sound_loc(vms_matrix* listener, vms_vector* listener_pos, int listener_seg, vms_vector* sound_pos, int sound_seg, fix max_volume, int* volume, int* pan, fix max_distance)
{
	vms_vector	vector_to_sound;
	fix angle_from_ear, cosang,sinang;
	fix distance;
	fix path_distance;

	*volume = 0;
	*pan = 0;

	max_distance = (max_distance*5)/4;		// Make all sounds travel 1.25 times as far.

	//	Warning: Made the vm_vec_normalized_dir be vm_vec_normalized_dir_quick and got illegal values to acos in the fang computation.
	distance = vm_vec_normalized_dir_quick( &vector_to_sound, sound_pos, listener_pos );

	if (distance < max_distance)	
	{
		int num_search_segs = f2i(max_distance/20);
		if (num_search_segs < 1) num_search_segs = 1;

		path_distance = find_connected_distance(listener_pos, listener_seg, sound_pos, sound_seg, num_search_segs, WID_RENDPAST_FLAG );
		if (path_distance > -1)	
		{
			*volume = max_volume - fixdiv(path_distance,max_distance);
			//mprintf( (0, "Sound path distance %.2f, volume is %d / %d\n", f2fl(distance), *volume, max_volume ));
			if (*volume > 0 )	
			{
				angle_from_ear = vm_vec_delta_ang_norm(&listener->rvec,&vector_to_sound,&listener->uvec);
				fix_sincos(angle_from_ear,&sinang,&cosang);
				//mprintf( (0, "volume is %.2f\n", f2fl(*volume) ));
				if (Config_channels_reversed) cosang *= -1;
				*pan = (cosang + F1_0)/2;
			} 
			else
			{
				*volume = 0;
			}
		}
	}	
}


void digi_init_sounds()
{
	int i;

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	digi_reset_digi_sounds();

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	
	{
		if (digi_sounds_initialized) 
		{
			if ( SoundObjects[i].flags & SOF_PLAYING )	
			{
				I_StopSound(SoundObjects[i].handle);
			}
		}
		SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
	}
	digi_sounds_initialized = 1;
}

void digi_start_sound_object(int i)
{
	// start sample structures
	//_SOS_START_SAMPLE sSOSSampleData;
	sampledata_t DigiSampleData;

	memset( &DigiSampleData, 0, sizeof(sampledata_t));

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	// Sound is not playing, so we must start it again
	SoundObjects[i].signature=next_signature++;

	DigiSampleData.data = GameSounds[SoundObjects[i].soundnum].data;
	DigiSampleData.length = GameSounds[SoundObjects[i].soundnum].length;
	DigiSampleData.volume = fixmuldiv(SoundObjects[i].volume , digi_volume, F1_0);
	DigiSampleData.maxDistance = SoundObjects[i].max_distance;
	DigiSampleData.global = 0;
	DigiSampleData.startpos = SoundObjects[i].pos;
	if (SoundObjects[i].vel.x != 0 || SoundObjects[i].vel.y != 0 || SoundObjects[i].vel.z != 0)
	{
		DigiSampleData.hasVelocity = true;
		DigiSampleData.velocity.x = SoundObjects[i].vel.x;
		DigiSampleData.velocity.y = SoundObjects[i].vel.y;
		DigiSampleData.velocity.z = SoundObjects[i].vel.z;
	}
	if (SoundObjects[i].flags & SOF_PLAY_FOREVER) 
	{
		DigiSampleData.loop = 1;
	}

	// start the sample playing
	SoundObjects[i].handle = digi_start_sound(&DigiSampleData, SoundObjects[i].soundnum);
	if (SoundObjects[i].handle != _ERR_NO_SLOTS )		
	{
		SoundObjects[i].flags |= SOF_PLAYING;
		//mprintf(( 0, "Starting sound object %d on channel %d\n", i, SoundObjects[i].handle ));
		reset_sounds_on_channel( SoundObjects[i].handle );
	}
//	else
//		mprintf( (1, "[Out of channels: %i] ", i ));

}

int digi_find_sound_object()
{
	int i;
	for (i = 0; i < MAX_SOUND_OBJECTS; i++)
		if (SoundObjects[i].flags == 0)
			return i;

	//Oops, no sound found, so try a lower priority one
	for (i = 0; i < MAX_SOUND_OBJECTS; i++)
		if (SoundObjects[i].flags & SOF_LOW_PRIORITY)
			return i;

	//Still no sound, so we'll have to terminate one
	for (i = 0; i < MAX_SOUND_OBJECTS; i++)
		if (!(SoundObjects[i].flags & SOF_PLAY_FOREVER))
		{
			SoundObjects[i].flags = 0;
			if (I_CheckSoundPlaying(SoundObjects[i].handle))
			{
				I_StopSound(SoundObjects[i].handle);
			}
			return i;
		}

	return MAX_SOUND_OBJECTS;
}

int digi_link_sound_to_object2(int org_soundnum, short objnum, int forever, fix max_volume, fix max_distance)
{
	int i,volume,pan;
	object * objp;
	int soundnum;

	soundnum = digi_xlat_sound(org_soundnum);

	if ( max_volume < 0 ) return -1;
//	if ( max_volume > F1_0 ) max_volume = F1_0;

	if (!Digi_initialized) return -1;
	if (soundnum < 0 ) return -1;
	if (GameSounds[soundnum].data==NULL) 
	{
		Int3();
		return -1;
	}
	if ((objnum<0)||(objnum>Highest_object_index))
		return -1;
	if (digi_driver_board<1) return -1;

	/*if ( !forever )	
	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, &Objects[objnum].pos, Objects[objnum].segnum, max_volume,&volume, &pan, max_distance );
		digi_play_sample_3d( org_soundnum, pan, volume, 0 );
		return -1;
	}*/

	i = digi_find_sound_object();
	SoundObjects[i].objnum = objnum;

	objp = &Objects[SoundObjects[i].objnum];
	if (i==MAX_SOUND_OBJECTS) 
	{
		mprintf((1, "Too many sound objects!\n" ));
		return -1;
	}

	SoundObjects[i].signature=next_signature++;
	SoundObjects[i].flags = SOF_USED | SOF_LINK_TO_OBJ;
	if ( forever )
		SoundObjects[i].flags |= SOF_PLAY_FOREVER;
	SoundObjects[i].objsignature = Objects[objnum].signature;
	SoundObjects[i].max_volume = max_volume;
	SoundObjects[i].max_distance = max_distance * 5 / 4;
	SoundObjects[i].volume = max_volume;
	SoundObjects[i].pos.x = objp->pos.x;
	SoundObjects[i].pos.y = objp->pos.y;
	SoundObjects[i].pos.z = objp->pos.z;
	if (objp->movement_type == MT_PHYSICS)
	{
		SoundObjects[i].vel.x = objp->mtype.phys_info.velocity.x;
		SoundObjects[i].vel.y = objp->mtype.phys_info.velocity.y;
		SoundObjects[i].vel.z = objp->mtype.phys_info.velocity.z;
	}
	else
		SoundObjects[i].vel.x = SoundObjects[i].vel.y = SoundObjects[i].vel.z = 0;
	SoundObjects[i].soundnum = soundnum;

	//digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum,
	//					&objp->pos, objp->segnum, SoundObjects[i].max_volume,
	//					&SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );

	if (forever || digi_get_sound_audible(&Viewer->orient, &Viewer->pos, Viewer->segnum, &objp->pos, objp->segnum, SoundObjects[i].max_distance))
		digi_start_sound_object(i);

	return SoundObjects[i].signature;
}

int digi_link_sound_to_object(int soundnum, short objnum, int forever, fix max_volume)
{																									// 10 segs away
	return digi_link_sound_to_object2(soundnum, objnum, forever, max_volume, 256*F1_0);
}

int digi_link_sound_to_pos2(int org_soundnum, short segnum, short sidenum, vms_vector* pos, int forever, fix max_volume, fix max_distance)
{
	int i, volume, pan;
	int soundnum;

	soundnum = digi_xlat_sound(org_soundnum);

	if ( max_volume < 0 ) return -1;
//	if ( max_volume > F1_0 ) max_volume = F1_0;

	if (!Digi_initialized) return -1;
	if (soundnum < 0 ) return -1;
	if (GameSounds[soundnum].data==NULL)
	{
		Int3();
		return -1;
	}
	if (digi_driver_board<1) return -1;

	if ((segnum<0)||(segnum>Highest_segment_index))
		return -1;

	/*if ( !forever )	
	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, pos, segnum, max_volume, &volume, &pan, max_distance );
		digi_play_sample_3d( org_soundnum, pan, volume, 0 );
		return -1;
	}*/

	i = digi_find_sound_object();

	if (i==MAX_SOUND_OBJECTS)
	{
		mprintf((1, "Too many sound objects!\n" ));
		return -1;
	}


	SoundObjects[i].signature=next_signature++;
	SoundObjects[i].flags = SOF_USED | SOF_LINK_TO_POS;
	if ( forever )
		SoundObjects[i].flags |= SOF_PLAY_FOREVER;
	SoundObjects[i].segnum = segnum;
	SoundObjects[i].sidenum = sidenum;
	SoundObjects[i].position = *pos;
	SoundObjects[i].soundnum = soundnum;
	SoundObjects[i].max_volume = max_volume;
	SoundObjects[i].max_distance = max_distance * 5 / 4;
	SoundObjects[i].volume = max_volume;
	SoundObjects[i].pos.x = pos->x;
	SoundObjects[i].pos.y = pos->y;
	SoundObjects[i].pos.z = pos->z;
	SoundObjects[i].vel.x = SoundObjects[i].vel.y = SoundObjects[i].vel.z = 0;

	if (forever || digi_get_sound_audible(&Viewer->orient, &Viewer->pos, Viewer->segnum, pos, segnum, SoundObjects[i].max_distance))
		digi_start_sound_object(i);

	return SoundObjects[i].signature;
}

int digi_link_sound_to_pos(int soundnum, short segnum, short sidenum, vms_vector* pos, int forever, fix max_volume)
{
	return digi_link_sound_to_pos2( soundnum, segnum, sidenum, pos, forever, max_volume, F1_0 * 256 );
}


void digi_kill_sound_linked_to_segment(int segnum, int sidenum, int soundnum)
{
	int i,killed;

	soundnum = digi_xlat_sound(soundnum);

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	killed = 0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	
	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_POS) )	
		{
			if ((SoundObjects[i].segnum == segnum) && (SoundObjects[i].soundnum==soundnum ) && (SoundObjects[i].sidenum==sidenum) )
			{
				if ( SoundObjects[i].flags & SOF_PLAYING )	
				{
					//sosDIGIStopSample( hSOSDigiDriver, SoundObjects[i].handle );
					I_StopSound(SoundObjects[i].handle);
				}
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}
	// If this assert happens, it means that there were 2 sounds
	// that got deleted. Weird, get John.
	if ( killed > 1 )	
	{
		mprintf( (1, "ERROR: More than 1 sounds were deleted from seg %d\n", segnum ));
	}
}

void digi_kill_sound_linked_to_object(int objnum)
{
	int i,killed;

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	killed = 0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	
	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_OBJ ) )	
		{
			if (SoundObjects[i].objnum == objnum)	
			{
				if ( SoundObjects[i].flags & SOF_PLAYING )	
				{
					I_StopSound(SoundObjects[i].handle);
				}
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}
	// If this assert happens, it means that there were 2 sounds
	// that got deleted. Weird, get John.
	if ( killed > 1 )	
	{
		mprintf( (1, "ERROR: More than 1 sounds were deleted from object %d\n", objnum ));
	}
}

void digi_sync_sounds()
{
	int i;
	int oldvolume, oldpan;

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	if (Viewer)
	{
		I_SetListenerPos(&Viewer->pos, &Viewer->mtype.phys_info.velocity, &Viewer->orient);
	}

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	
	{
		if ( SoundObjects[i].flags & SOF_USED )	
		{
			oldvolume = 0;

			if ( !(SoundObjects[i].flags & SOF_PLAY_FOREVER) )	
			{
				// Check if its done.
				if (SoundObjects[i].flags & SOF_PLAYING) 
				{
					if (!I_CheckSoundPlaying(SoundObjects[i].handle))
					{
						mprintf((0, "killing sound object %d due to done playing\n", i));
						SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
						continue;		// Go on to next sound...
					}
				}
			}

			if ( SoundObjects[i].flags & SOF_LINK_TO_POS )
			{
				oldvolume = digi_get_sound_audible( &Viewer->orient, &Viewer->pos, Viewer->segnum,
								&SoundObjects[i].position, SoundObjects[i].segnum, SoundObjects[i].max_distance );

			} 

			if ( SoundObjects[i].flags & SOF_LINK_TO_OBJ && !(SoundObjects[i].flags & SOF_LOW_PRIORITY) )	
			{
				object * objp;

				objp = &Objects[SoundObjects[i].objnum];

				oldvolume = digi_get_sound_audible(&Viewer->orient, &Viewer->pos, Viewer->segnum,
					&objp->pos, objp->segnum, SoundObjects[i].max_distance); //Always must be done

				if ((objp->type==OBJ_NONE) || (objp->signature!=SoundObjects[i].objsignature) || (objp->flags & OF_SHOULD_BE_DEAD))	
				{
					// The object that this is linked to is dead, so just end this sound if it is looping.
					if ( (SoundObjects[i].flags & SOF_PLAYING)  && (SoundObjects[i].flags & SOF_PLAY_FOREVER))	
					{
						I_StopSound(SoundObjects[i].handle);
						SoundObjects[i].flags = 0;
						continue; // Go on to next sound...
					}
					else
					{
						mprintf((0, "low-prioritizing sound object %d due to source dead\n", i));
						SoundObjects[i].flags |= SOF_LOW_PRIORITY | SOF_LINK_TO_POS;	// Mark as dead, so some other sound can use this sound	
						SoundObjects[i].flags &= ~SOF_LINK_TO_OBJ; //Switch to linked to a position so it doesn't refer to a bad entity. 
						SoundObjects[i].position = objp->pos;
						SoundObjects[i].segnum = objp->segnum;
						vm_vec_zero(&SoundObjects[i].vel);
					}
				}
				else 
				{
					I_SetPosition(SoundObjects[i].handle, &objp->pos);
					if (objp->movement_type == MT_PHYSICS)
					{
						I_SetVelocity(SoundObjects[i].handle, &objp->mtype.phys_info.velocity);
					}
				}
			}

			//SoundObjects[i].volume = fixmuldiv(SoundObjects[i].volume, digi_volume, F1_0);
			//if (oldvolume != SoundObjects[i].volume) 	
			{
				if (!oldvolume)	
				{
					// Sound is too far away, so stop it from playing.
					if ((SoundObjects[i].flags & SOF_PLAYING)&&(SoundObjects[i].flags & SOF_PLAY_FOREVER))	
					{
						I_StopSound(SoundObjects[i].handle);
						SoundObjects[i].flags &= ~SOF_PLAYING;		// Mark sound as not playing
					}
					else if ((SoundObjects[i].flags & SOF_PLAYING))
					{
						I_StopSound(SoundObjects[i].handle);
						SoundObjects[i].flags = 0;	//Too far away and its a one shot, so just kill it
						mprintf((0, "killing sound object %d due to too distant\n", i));
					}
				}
				else 
				{
					if (!(SoundObjects[i].flags & SOF_PLAYING))	
					{
						digi_start_sound_object(i);
					}
					else 
					{
						I_SetVolume(SoundObjects[i].handle, fixmuldiv(SoundObjects[i].volume, digi_volume, F1_0));
					}
				}
			}
		}
	}
}


int sound_paused = 0;

void digi_pause_all()
{
	int i;

	if (!Digi_initialized) return;

	if (sound_paused==0)	
	{
		if ( digi_midi_type > 0 )	
		{
			//[ISB] dead
		}
		if (digi_driver_board>0)	
		{
			for (i=0; i<MAX_SOUND_OBJECTS; i++ )	
			{
				if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_PLAYING)&& (SoundObjects[i].flags & SOF_PLAY_FOREVER) )	
				{
					I_StopSound(SoundObjects[i].handle);
					SoundObjects[i].flags &= ~SOF_PLAYING;		// Mark sound as not playing
				}
			}
		}
	}
	sound_paused++;
}

void digi_resume_all()
{
	if (!Digi_initialized) return;

	Assert( sound_paused > 0 );

	if (sound_paused==1)	
	{
		// resume sound here
		if ( digi_midi_type > 0 )	
		{
			//if (wSongHandle < 0xffff )	
			//{
			//   // stop the last MIDI song from playing
			//	_disable();
			//	sosMIDIResumeSong( wSongHandle );
			//	_enable();
			//	mprintf(( 0, "Resumed song %d\n", wSongHandle ));
			//}
			/*_disable();
			sosMIDISetMasterVolume(midi_volume); //[ISB] TODO
			_enable();*/
		}
	}
	sound_paused--;
}


void digi_stop_all()
{
	int i;

	if (!Digi_initialized) return;

	if ( digi_midi_type > 0 )	
	{
		if (wSongHandle < 0xffff )	
		{
			S_StopSong();
			wSongHandle = 0xffff;
		}
		if (SongData) 	
		{
			free(SongData);
			SongData = NULL;
		}
	}

	if (digi_driver_board>0)	
	{
		for (i=0; i<MAX_SOUND_OBJECTS; i++ )	
		{
			if ( SoundObjects[i].flags & SOF_USED )	
			{
				if ( SoundObjects[i].flags & SOF_PLAYING )	
				{
					I_StopSound(SoundObjects[i].handle);
				}
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
			}
		}
	}
}

#ifndef NDEBUG
int verify_sound_channel_free(int channel)
{
	int i;
	if (digi_driver_board>0)	
	{
		for (i=0; i<MAX_SOUND_OBJECTS; i++ )	
		{
			if ( SoundObjects[i].flags & SOF_USED )	
			{
				if ( SoundObjects[i].flags & SOF_PLAYING )	
				{
					if ( SoundObjects[i].handle == channel )	
					{
						mprintf(( 0, "ERROR STARTING SOUND CHANNEL ON USED SLOT!!\n" ));
						Int3();	// Get John!
					}
				}
			}
		}
	}
	return 0;
}
#endif

/////////////////////////////////////////////////////////////////////////////

#include "misc/stb_vorbis.h"
#include <string>

bool PlayHQSong(const char* filename, bool loop)
{
	// Load ogg into memory:

	std::string name = filename;
	name = name.substr(0, name.size() - 4); // cut off extension

	FILE* file = fopen(("music/" + name + ".ogg").c_str(), "rb");
	if (!file) return false;
	fseek(file, 0, SEEK_END);
	auto size = ftell(file);
	fseek(file, 0, SEEK_SET);
	std::vector<uint8_t> filedata(size);
	fread(filedata.data(), filedata.size(), 1, file);
	fclose(file);

	// Decompress it:

	int error = 0;
	int stream_byte_offset = 0;
	stb_vorbis* handle = stb_vorbis_open_pushdata(filedata.data(), filedata.size(), &stream_byte_offset, &error, nullptr);
	if (handle == nullptr)
		return false;

	stb_vorbis_info stream_info = stb_vorbis_get_info(handle);
	int song_sample_rate = stream_info.sample_rate;
	int song_channels = stream_info.channels;
	std::vector<float> song_data;

	while (true)
	{
		float** pcm = nullptr;
		int pcm_samples = 0;
		int bytes_used = stb_vorbis_decode_frame_pushdata(handle, filedata.data() + stream_byte_offset, filedata.size() - stream_byte_offset, nullptr, &pcm, &pcm_samples);

		if (song_channels > 1)
		{
			for (int i = 0; i < pcm_samples; i++)
			{
				song_data.push_back(pcm[0][i]);
				song_data.push_back(pcm[1][i]);
			}
		}
		else
		{
			for (int i = 0; i < pcm_samples; i++)
			{
				song_data.push_back(pcm[0][i]);
				song_data.push_back(pcm[0][i]);
			}
		}

		stream_byte_offset += bytes_used;
		if (bytes_used == 0 || stream_byte_offset == filedata.size())
			break;
	}

	stb_vorbis_close(handle);

	I_PlayHQSong(song_sample_rate, std::move(song_data), loop);

	return true;
}

void StopHQSong()
{
	I_StopHQSong();
}
