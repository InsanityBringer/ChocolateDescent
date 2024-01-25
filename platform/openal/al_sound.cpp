/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <thread>
#include <mutex>

#ifdef USE_OPENAL

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"
#include "platform/i_sound.h"
#include "platform/s_midi.h"
#include "platform/s_sequencer.h"
#include "misc/error.h"
//#include "mem/mem.h" //[ISB] mem.h isn't thread safe so uh

ALCdevice *ALDevice = NULL;
ALCcontext *ALContext = NULL;

int AL_initialized = 0;

ALuint bufferNames[_MAX_VOICES];
ALuint sourceNames[_MAX_VOICES];

//MIDI audio fields
//MAX_BUFFERS_QUEUED is currently in terms of buffers of 4 ticks
//The higher this and the amount of ticks in the system, the higher the latency is.
#define MAX_BUFFERS_QUEUED 4

bool LoopMusic;

std::mutex MIDIMutex;
std::thread MIDIThread;

struct ALMusicSource
{
	int MusicVolume; //Current volume level of the source. Compare against the current MusicVolume while servicing.
	int NumFreeBuffers; //Number of free buffers
	int NumUsedBuffers; //Number of used buffers
	int AvailableBuffers[MAX_BUFFERS_QUEUED]; //All available buffer names
	int UsedBuffers[MAX_BUFFERS_QUEUED]; //All currently queued buffer names. 
	ALuint SourceBuffers[MAX_BUFFERS_QUEUED]; //The source names this music source will use. 
	ALuint BufferQueue[MAX_BUFFERS_QUEUED]; 
	ALuint MusicSource; //Source name for the music
	ALuint MusicSampleRate; //Sample rate for the current music. 
	bool Playing; //True if the source has been started for the first time. 
};

//HQ audio fields
ALuint HQMusicSource;
ALuint HQMusicBuffer;
bool HQMusicPlaying = false;

int MusicVolume;

MidiPlayer* midiPlayer;

void AL_ErrorCheck(const char* context)
{
	int error;
	error = alGetError();
	if (error != AL_NO_ERROR)
	{
		fprintf(stderr, "Error in context %s: ", context);
		if (error == AL_INVALID_ENUM)
			fprintf(stderr, "Invalid enum\n");
		else if (error == AL_INVALID_NAME)
			fprintf(stderr, "Invalid name\n");
		else if (error == AL_INVALID_OPERATION)
			fprintf(stderr, "Invalid operation\n");
		else if (error == AL_INVALID_VALUE)
			fprintf(stderr, "Invalid value\n");
	}
}

void* I_CreateMusicSource();

void AL_InitSource(ALuint source)
{
	alSourcef(source, AL_ROLLOFF_FACTOR, 0.0f);
	alSource3f(source, AL_DIRECTION, 0.f, 0.f, 0.f);
	alSource3f(source, AL_VELOCITY, 0.f, 0.f, 0.f);
	alSource3f(source, AL_POSITION, 0.f, 0.f, 0.f);
	alSourcef(source, AL_MAX_GAIN, 1.f);
	alSourcef(source, AL_GAIN, 1.f);
	alSourcef(source, AL_PITCH, 1.f);
	alSourcef(source, AL_DOPPLER_FACTOR, 0.f);
	AL_ErrorCheck("Init AL source");
}

int plat_init_audio()
{
	int i;
	ALDevice = alcOpenDevice(NULL);
	if (ALDevice == NULL)
	{
		Warning("plat_init_audio: Cannot open OpenAL device\n");
		return 1;
	}
	ALContext = alcCreateContext(ALDevice, 0);
	if (ALContext == NULL)
	{
		Warning("plat_init_audio: Cannot create OpenAL context\n");
		plat_close_audio();
		return 1;
	}
	alcMakeContextCurrent(ALContext);

	alGenBuffers(_MAX_VOICES, &bufferNames[0]);
	AL_ErrorCheck("Creating buffers");
	alGenSources(_MAX_VOICES, &sourceNames[0]);
	AL_ErrorCheck("Creating sources");
	for (i = 0; i < _MAX_VOICES; i++)
	{
		AL_InitSource(sourceNames[i]);
	}

	if (!alIsExtensionPresent("AL_EXT_FLOAT32"))
	{
		printf("OpenAL implementation doesn't support floating point samples for HQ Music.\n");
	}
	if (!alIsExtensionPresent("AL_SOFT_loop_points"))
	{
		printf("OpenAL implementation doesn't support OpenAL soft loop points. Are you not using OpenAL soft?\n");
	}
	AL_ErrorCheck("Checking exts");

	AL_initialized = 1;

	return 0;
}

void plat_close_audio()
{
	if (ALDevice)
	{
		alcMakeContextCurrent(NULL);
		if (ALContext)
			alcDestroyContext(ALContext);
		alcCloseDevice(ALDevice);
		AL_initialized = 0;
	}
}

int plat_get_new_sound_handle()
{
	ALint state;
	for (int i = 0; i < _MAX_VOICES; i++)
	{
		alGetSourcei(sourceNames[i], AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			return i;
		}
	}
	AL_ErrorCheck("Getting handle");
	return _ERR_NO_SLOTS;
}

void plat_set_sound_data(int handle, unsigned char* data, int length, int sampleRate)
{
	if (handle >= _MAX_VOICES) return;

	alSourcei(sourceNames[handle], AL_BUFFER, 0);
	alBufferData(bufferNames[handle], AL_FORMAT_MONO8, data, length, sampleRate);
	plat_set_sound_loop_points(handle, 0, length);

	AL_ErrorCheck("Setting sound data");
}

void plat_set_sound_position(int handle, int volume, int angle)
{
	if (handle >= _MAX_VOICES) return;
	plat_set_sound_angle(handle, angle);
	plat_set_sound_volume(handle, volume);
}

void plat_set_sound_angle(int handle, int angle)
{
	if (handle >= _MAX_VOICES) return;

	float x, y;
	float flang = (angle / 65536.0f) * (3.1415927f);

	x = (float)cos(flang);
	y = (float)sin(flang);

	alSource3f(sourceNames[handle], AL_POSITION, -x, 0.0f, -y);
	AL_ErrorCheck("Setting sound angle");
}

void plat_set_sound_volume(int handle, int volume)
{
	if (handle >= _MAX_VOICES) return;

	float gain = volume / 32768.0f;
	alSourcef(sourceNames[handle], AL_GAIN, gain);
	AL_ErrorCheck("Setting sound volume");
}

void plat_set_sound_loop_points(int handle, int start, int end)
{
	if (start == -1) start = 0;
	if (end == -1)
	{
		ALint len;
		alGetBufferi(bufferNames[handle], AL_SIZE, &len);
		end = len;
		AL_ErrorCheck("Getting buffer length");
	}
	ALint loopPoints[2];
	loopPoints[0] = start;
	loopPoints[1] = end;
	alBufferiv(bufferNames[handle], AL_LOOP_POINTS_SOFT, &loopPoints[0]);
	AL_ErrorCheck("Setting loop points");
}

void plat_start_sound(int handle, int loop)
{
	if (handle >= _MAX_VOICES) return;
	alSourcei(sourceNames[handle], AL_BUFFER, bufferNames[handle]);
	alSourcei(sourceNames[handle], AL_LOOPING, loop);
	alSourcePlay(sourceNames[handle]);
	AL_ErrorCheck("Playing sound");
}

void plat_stop_sound(int handle)
{
	if (handle >= _MAX_VOICES) return;
	alSourceStop(sourceNames[handle]);
	AL_ErrorCheck("Stopping sound");
}

int plat_check_if_sound_playing(int handle)
{
	if (handle >= _MAX_VOICES) return 0;
	
	int playing;
	alGetSourcei(sourceNames[handle], AL_SOURCE_STATE, &playing);
	return playing == AL_PLAYING;
}

int plat_check_if_sound_finished(int handle)
{
	if (handle >= _MAX_VOICES) return 0;

	int playing;
	alGetSourcei(sourceNames[handle], AL_SOURCE_STATE, &playing);
	return playing == AL_STOPPED;
}

//-----------------------------------------------------------------------------
// Emitting pleasing rythmic sound at player
//-----------------------------------------------------------------------------
//bool playing = false;

int plat_start_midi(MidiSequencer* sequencer)
{
	return 0;
}

void plat_close_midi()
{
}

void plat_set_music_volume(int volume)
{
	if (!AL_initialized) return;
	//printf("Music volume %d\n", volume);
	MusicVolume = volume;

	//[ISB] Midi volume is now handled at the synth level, not the mixer level. 
	/*if (alIsSource(MusicSource)) //[ISB] TODO okay so this isn't truly thread safe, it likely won't pose a problem, but I should fix it just in case
	{
		alSourcef(MusicSource, AL_GAIN, MusicVolume / 127.0f);
	}*/
	if (alIsSource(HQMusicSource)) //[ISB] heh
	{
		alSourcef(HQMusicSource, AL_GAIN, MusicVolume / 127.0f);
	}
	AL_ErrorCheck("Setting music volume");
}

void plat_start_hq_song(int sample_rate, std::vector<float>&& song_data, bool loop)
{
	alGenSources(1, &HQMusicSource);
	alSourcef(HQMusicSource, AL_ROLLOFF_FACTOR, 0.0f);
	alSource3f(HQMusicSource, AL_POSITION, 1.0f, 0.0f, 0.0f);
	alSourcef(HQMusicSource, AL_GAIN, MusicVolume / 127.0f);
	alSourcei(HQMusicSource, AL_LOOPING, loop);
	AL_ErrorCheck("Creating HQ music source");

	alGenBuffers(1, &HQMusicBuffer);
	alBufferData(HQMusicBuffer, AL_FORMAT_STEREO_FLOAT32, (ALvoid*)song_data.data(), song_data.size() * sizeof(float), sample_rate);
	AL_ErrorCheck("Creating HQ music buffer");
	alSourcei(HQMusicSource, AL_BUFFER, HQMusicBuffer);
	alSourcePlay(HQMusicSource);
	AL_ErrorCheck("Playing HQ music");
	HQMusicPlaying = true;
}

void plat_stop_hq_song()
{
	if (HQMusicPlaying)
	{
		alSourceStop(HQMusicSource);
		alDeleteSources(1, &HQMusicSource);
		alDeleteBuffers(1, &HQMusicBuffer);
		AL_ErrorCheck("Stopping HQ music");
		HQMusicPlaying = false;
	}
}

void* I_CreateMusicSource()
{
	int i;
	ALMusicSource* source = new ALMusicSource;

	alGenSources(1, &source->MusicSource);
	alSourcef(source->MusicSource, AL_ROLLOFF_FACTOR, 0.0f);
	alSource3f(source->MusicSource, AL_POSITION, 0.0f, 0.0f, 0.0f);
	alSourcef(source->MusicSource, AL_GAIN, MusicVolume / 127.0f);
	source->MusicVolume = MusicVolume;
	memset(&source->BufferQueue[0], 0, sizeof(ALuint) * MAX_BUFFERS_QUEUED);
	alGenBuffers(MAX_BUFFERS_QUEUED, source->SourceBuffers);
	for (i = 0; i < MAX_BUFFERS_QUEUED; i++)
		source->AvailableBuffers[i] = i;
	source->NumFreeBuffers = MAX_BUFFERS_QUEUED;
	source->NumUsedBuffers = 0;
	source->Playing = false;
	AL_ErrorCheck("Creating music source");

	return source;
}

void I_DestroyMusicSource(void* opaquesource)
{
	ALMusicSource* source = (ALMusicSource*)opaquesource;
	int BuffersProcessed;
	alGetSourcei(source->MusicSource, AL_BUFFERS_QUEUED, &BuffersProcessed);
	/*if (BuffersProcessed > 0) //Free any lingering buffers
	{
		alSourceUnqueueBuffers(MusicSource, BuffersProcessed, &BufferQueue[0]);
		alDeleteBuffers(BuffersProcessed, &BufferQueue[0]);
	}
	I_ErrorCheck("Destroying lingering music buffers");*/
	alDeleteSources(1, &source->MusicSource);
	AL_ErrorCheck("Destroying music source");
	alDeleteBuffers(MAX_BUFFERS_QUEUED, source->SourceBuffers);
	AL_ErrorCheck("Destroying lingering buffers");
	source->MusicSource = 0;

	delete source;
}

void midi_set_music_samplerate(void* opaquesource, uint32_t samplerate)
{
	ALMusicSource* source = (ALMusicSource*)opaquesource;
	source->MusicSampleRate = samplerate;
}

void midi_check_status(void* opaquesource)
{
	ALMusicSource* source = (ALMusicSource*)opaquesource;
	ALenum playstatus;
	if (!source->Playing)
	{
		source->Playing = true;
		alSourcePlay(source->MusicSource);
		AL_ErrorCheck("Playing music source");
	}
	else
	{
		alGetSourcei(source->MusicSource, AL_SOURCE_STATE, &playstatus);
		if (playstatus != AL_PLAYING)
		{
			//If this happens, the buffer starved, kick it back up
			//This should happen as rarely as humanly possible, otherwise there's a pop because of the brief stall in the stream.
			//I need to find ways to reduce the amount of overhead in the setup. 
			printf("midi buffer starved\n");
			alSourcePlay(source->MusicSource);
		}
	}
}

bool midi_check_finished(void* opaquesource)
{
	ALMusicSource* source = (ALMusicSource*)opaquesource;
	if (source->MusicSource == 0) return true;

	ALenum playstatus;
	alGetSourcei(source->MusicSource, AL_SOURCE_STATE, &playstatus);

	return playstatus != AL_PLAYING;
}

bool midi_queue_slots_available(void* opaquesource)
{
	if (!AL_initialized) return false;
	ALMusicSource* source = (ALMusicSource*)opaquesource;
	/*alGetSourcei(MusicSource, AL_BUFFERS_QUEUED, &CurrentBuffers);
	AL_ErrorCheck("Checking can queue buffers");
	return CurrentBuffers < MAX_BUFFERS_QUEUED;*/
	return source->NumFreeBuffers > 0;
}

void midi_dequeue_midi_buffers(void* opaquesource)
{
	int i;
	if (!AL_initialized) return;
	ALMusicSource* source = (ALMusicSource*)opaquesource;
	//I should probably keep track of all active music sources to do this immediately, but this will work for now. 
	//Check if the music volume changed and keep it up to date.
	if (source->MusicVolume != MusicVolume)
	{
		alSourcef(source->MusicSource, AL_GAIN, MusicVolume / 127.0f);
		source->MusicVolume = MusicVolume;
	}

	int BuffersProcessed;
	alGetSourcei(source->MusicSource, AL_BUFFERS_PROCESSED, &BuffersProcessed);
	if (BuffersProcessed > 0)
	{
		alSourceUnqueueBuffers(source->MusicSource, BuffersProcessed, &source->BufferQueue[0]);
		for (i = 0; i < BuffersProcessed; i++)
		{
			source->AvailableBuffers[source->NumFreeBuffers] = source->UsedBuffers[i];

			//printf("dq %d\n", source->UsedBuffers[i]);

			source->NumFreeBuffers++;
			source->NumUsedBuffers--;
		}
		for (i = 0; i < MAX_BUFFERS_QUEUED - BuffersProcessed; i++)
		{
			source->BufferQueue[i] = source->BufferQueue[i + BuffersProcessed];
			source->UsedBuffers[i] = source->UsedBuffers[i + BuffersProcessed];
		}
		//printf("Killing %d buffers\n", BuffersProcessed);
	}
	AL_ErrorCheck("Unqueueing music buffers");
}

void midi_queue_buffer(void* opaquesource, int numTicks, uint16_t *data)
{
	if (!AL_initialized) return;
	ALMusicSource* source = (ALMusicSource*)opaquesource;

	//printf("Queuing %d ticks\n", numTicks);
	if (source->NumFreeBuffers > 0)
	{
		int freeBufferNum = source->AvailableBuffers[source->NumFreeBuffers - 1];
		source->UsedBuffers[source->NumUsedBuffers] = freeBufferNum;
		
		source->BufferQueue[source->NumUsedBuffers] = source->SourceBuffers[freeBufferNum];
		alBufferData(source->BufferQueue[source->NumUsedBuffers], AL_FORMAT_STEREO16, data, numTicks * sizeof(ALushort) * 2, source->MusicSampleRate);
		alSourceQueueBuffers(source->MusicSource, 1, &source->BufferQueue[source->NumUsedBuffers]);
		AL_ErrorCheck("Queueing music buffers");

		//printf("q %d\n", freeBufferNum);

		source->NumUsedBuffers++;
		source->NumFreeBuffers--;
	}
}

void plat_start_midi_song(HMPFile* song, bool loop)
{
	//Hey, how'd I get here? What do I do?
}

void plat_stop_midi_song()
{
}

uint32_t plat_get_preferred_midi_sample_rate()
{
	return MIDI_SAMPLERATE;
}

void* midi_start_source()
{
	if (!AL_initialized) return nullptr;
	void* source = I_CreateMusicSource();
	AL_ErrorCheck("Creating source");
	return source;
}

void midi_stop_source(void* opaquesource)
{
	if (!AL_initialized) return;
	ALMusicSource* source = (ALMusicSource*)opaquesource;
	alSourceStop(source->MusicSource);
	midi_dequeue_midi_buffers(opaquesource);
	I_DestroyMusicSource(opaquesource);
	AL_ErrorCheck("Destroying source");
}

//-----------------------------------------------------------------------------
// Emitting buffered movie sound at player
//-----------------------------------------------------------------------------
#define NUMMVESNDBUFFERS 100

//Next buffer position, and earliest buffer position still currently queued
int mveSndBufferHead, mveSndBufferTail;
//Ring buffer of all the buffer names
ALuint mveSndRingBuffer[NUMMVESNDBUFFERS];

ALenum mveSndFormat;
ALint mveSndSampleRate;
ALuint mveSndSourceName;
ALboolean mveSndPlaying;

void I_CreateMovieSource()
{
	alGenSources(1, &mveSndSourceName);
	alSourcef(mveSndSourceName, AL_ROLLOFF_FACTOR, 0.0f);
	alSource3f(mveSndSourceName, AL_POSITION, 0.0f, 0.0f, 0.0f);
	AL_ErrorCheck("Creating movie source");
}

void mvesnd_init_audio(int format, int samplerate, int stereo)
{
	//printf("format: %d, samplerate: %d, stereo: %d\n", format, samplerate, stereo);
	switch (format)
	{
	case MVESND_U8:
		if (stereo)
			mveSndFormat = AL_FORMAT_STEREO8;
		else
			mveSndFormat = AL_FORMAT_MONO8;
		break;
	case MVESND_S16LSB:
		if (stereo)
			mveSndFormat = AL_FORMAT_STEREO16;
		else
			mveSndFormat = AL_FORMAT_MONO16;
		break;
	}
	mveSndSampleRate = samplerate;

	mveSndBufferHead = 0; mveSndBufferTail = 0;
	mveSndPlaying = AL_FALSE;

	I_CreateMovieSource();
}

void I_DequeueMovieAudioBuffers(int all)
{
	int i, n;
	//Dequeue processed buffers in the ring buffer
	alGetSourcei(mveSndSourceName, AL_BUFFERS_PROCESSED, &n);
	for (i = 0; i < n; i++)
	{
		alSourceUnqueueBuffers(mveSndSourceName, 1, &mveSndRingBuffer[mveSndBufferTail]);
		alDeleteBuffers(1, &mveSndRingBuffer[mveSndBufferTail]);

		mveSndBufferTail++;
		if (mveSndBufferTail == NUMMVESNDBUFFERS)
			mveSndBufferTail = 0;
	}
	AL_ErrorCheck("Dequeing movie buffers");
	//kill all remaining buffers if we're told to stop
	if (all)
	{
		alGetSourcei(mveSndSourceName, AL_BUFFERS_QUEUED, &n);
		for (i = 0; i < n; i++)
		{
			alSourceUnqueueBuffers(mveSndSourceName, 1, &mveSndRingBuffer[mveSndBufferTail]);
			alDeleteBuffers(1, &mveSndRingBuffer[mveSndBufferTail]);

			mveSndBufferTail++;
			if (mveSndBufferTail == NUMMVESNDBUFFERS)
				mveSndBufferTail = 0;
		}
		AL_ErrorCheck("Dequeing excess movie buffers");
	}
}

void mvesnd_queue_audio_buffer(int len, short* data)
{
	//I don't currently have a tick function (should I fix this?), so do this now
	I_DequeueMovieAudioBuffers(0);

	//Generate and fill out a new buffer
	alGenBuffers(1, &mveSndRingBuffer[mveSndBufferHead]);
	alBufferData(mveSndRingBuffer[mveSndBufferHead], mveSndFormat, (ALvoid*)data, len, mveSndSampleRate);

	AL_ErrorCheck("Creating movie buffers");
	//Queue the buffer, and if this source isn't playing, kick it off
	alSourceQueueBuffers(mveSndSourceName, 1, &mveSndRingBuffer[mveSndBufferHead]);
	if (mveSndPlaying == AL_FALSE)
	{
		alSourcePlay(mveSndSourceName);
		mveSndPlaying = AL_TRUE;
	}

	mveSndBufferHead++;
	if (mveSndBufferHead == NUMMVESNDBUFFERS)
		mveSndBufferHead = 0;

	AL_ErrorCheck("Queuing movie buffers");
}

void mvesnd_close()
{
	if (mveSndPlaying)
		alSourceStop(mveSndSourceName);

	I_DequeueMovieAudioBuffers(1);

	alDeleteSources(1, &mveSndSourceName);
}

void mvesnd_pause()
{
	alSourcePause(mveSndSourceName);
}

void mvesnd_resume()
{
	alSourcePlay(mveSndSourceName);
}

#endif
