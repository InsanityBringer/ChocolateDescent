#pragma once

#include <vector>

#include "s_midi.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

//Number of possible sound channels
#define _MAX_VOICES 32

//Id of a bad handle
#define _ERR_NO_SLOTS 0xFFFE

//-----------------------------------------------------------------------------
// Init and shutdown
//-----------------------------------------------------------------------------

//Initalizes the audio library. Returns 0 on success, 1 on failure.
int I_InitAudio();

//Shutdown the audio library
void I_ShutdownAudio();

//-----------------------------------------------------------------------------
// Handles and data
//-----------------------------------------------------------------------------

//Gets a handle to refer to this sown
int I_GetSoundHandle();

//Provide the PCM data for this sound effect
void I_SetSoundData(int handle, unsigned char* data, int length, int sampleRate);

void I_SetSoundInformation(int handle, int volume, int angle);

void I_SetAngle(int handle, int angle);

void I_SetVolume(int handle, int volume);

void I_SetLoopPoints(int handle, int start, int end);

//-----------------------------------------------------------------------------
// Emitting noise at player
//-----------------------------------------------------------------------------

void I_PlaySound(int handle, int loop);
void I_StopSound(int handle);

int I_CheckSoundPlaying(int handle);

int I_CheckSoundDone(int handle);

//-----------------------------------------------------------------------------
// Emitting pleasing rythmic sequences at player
//-----------------------------------------------------------------------------

//Starts the audio system's MIDI system with the specified sequencer for playback.
int I_StartMIDI(MidiSequencer *sequencer);
//Gets the synth's sample rate. This lets the Windows backend run the synth at its preferred sample rate
uint32_t I_GetPreferredMIDISampleRate();
//Stops the audio system's MIDI system
void I_ShutdownMIDI();
//Sets the current music volume. In the range 0-127.
void I_SetMusicVolume(int volume);

//Starts a given song
void I_StartMIDISong(hmpheader_t* song, bool loop);
void I_StopMIDISong();

//[ISB] sigh. Hindsight's 20/20.

#ifdef USE_OPENAL
bool AL_CanQueueMusicBuffer();
void AL_DequeueMusicBuffers();
void AL_QueueMusicBuffer(int numTicks, uint16_t* data);

void AL_StartMIDISong();
void AL_StopMIDISong();
#endif

//-----------------------------------------------------------------------------
// Emitting recordings of pleasing rythmic sequences at player
//-----------------------------------------------------------------------------

void I_PlayHQSong(int sample_rate, std::vector<float>&& song_data, bool loop);
void I_StopHQSong();

//-----------------------------------------------------------------------------
// Emitting buffered movie sound at player
//-----------------------------------------------------------------------------

#define MVESND_S16LSB 1
#define MVESND_U8 2

void I_InitMovieAudio(int format, int samplerate, int stereo);
void I_QueueMovieAudioBuffer(int len, short* data);
void I_DestroyMovieAudio();

void I_PauseMovieAudio();
void I_UnPauseMovieAudio();
