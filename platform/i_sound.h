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
int plat_init_audio();

//Shutdown the audio library
void plat_close_audio();

//-----------------------------------------------------------------------------
// Handles and data
//-----------------------------------------------------------------------------

//Gets a handle to refer to this sound
int plat_get_new_sound_handle();

//Provide the PCM data for this sound effect
void plat_set_sound_data(int handle, unsigned char* data, int length, int sampleRate);

void plat_set_sound_position(int handle, int volume, int angle);

void plat_set_sound_angle(int handle, int angle);

void plat_set_sound_volume(int handle, int volume);

void plat_set_sound_loop_points(int handle, int start, int end);

//-----------------------------------------------------------------------------
// Emitting noise at player
//-----------------------------------------------------------------------------

void plat_start_sound(int handle, int loop);
void plat_stop_sound(int handle);

int plat_check_if_sound_playing(int handle);

int plat_check_if_sound_finished(int handle);

//-----------------------------------------------------------------------------
// Emitting pleasing rythmic sequences at player
//-----------------------------------------------------------------------------

//Starts the audio system's MIDI system with the specified sequencer for playback.
int plat_start_midi(MidiSequencer *sequencer);
//Gets the synth's sample rate. This lets the Windows backend run the synth at its preferred sample rate
uint32_t plat_get_preferred_midi_sample_rate();
//Stops the audio system's MIDI system
void plat_close_midi();
//Sets the current music volume. In the range 0-127.
void plat_set_music_volume(int volume);

//Starts a given song
void plat_start_midi_song(HMPFile* song, bool loop);
void plat_stop_midi_song();

//[ISB] sigh. Hindsight's 20/20.
//[FUTURE ISB] no I really should have known better.
//[FUTURE FUTURE ISB] agh, no, we're restoring this interface, since it's needed for
//hardsynths.

//Returns true if there are available buffer slots in the music source's buffer queue.
bool midi_queue_slots_available();
//Clears all finished buffers from the queue.
void midi_dequeue_midi_buffers();
//Queues a new buffer into the music source, at MIDI_SAMPLERATE sample rate.
void midi_queue_buffer(int numSamples, uint16_t* data);

//Readies the source for playing MIDI music.
void midi_start_source();
//Stops the MIDI music source.
void midi_stop_source();
//Starts the MIDI source if it hasn't started already, and starts it again if it starved.
void midi_check_status();


//-----------------------------------------------------------------------------
// Emitting recordings of pleasing rythmic sequences at player
//-----------------------------------------------------------------------------

void plat_start_hq_song(int sample_rate, std::vector<float>&& song_data, bool loop);
void plat_stop_hq_song();

//-----------------------------------------------------------------------------
// Emitting buffered movie sound at player
//-----------------------------------------------------------------------------

#define MVESND_S16LSB 1
#define MVESND_U8 2

void mvesnd_init_audio(int format, int samplerate, int stereo);
void mvesnd_queue_audio_buffer(int len, short* data);
void mvesnd_close();

void mvesnd_pause();
void mvesnd_resume();
