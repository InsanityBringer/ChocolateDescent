/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <thread>
#include <mutex>

#ifdef USE_OPENAL

#include "AL/al.h"
#include "AL/alc.h"
#include "platform/i_sound.h"
#include "platform/s_midi.h"
#include "platform/s_sequencer.h"
#include "misc/error.h"
//#include "mem/mem.h" //[ISB] mem.h isn't thread safe so uh

ALCdevice *ALDevice = NULL;
ALCcontext *ALContext = NULL;

ALuint bufferNames[_MAX_VOICES];
ALuint sourceNames[_MAX_VOICES];

//MIDI fields

#define MAX_BUFFERS_QUEUED 5

hmpheader_t* CurrentSong;
bool StopMIDI = true;
bool LoopMusic;

std::mutex MIDIMutex;
std::thread MIDIThread;

ALuint BufferQueue[MAX_BUFFERS_QUEUED];
ALuint MusicSource;
int CurrentBuffers = 0;

ALushort* MusicBufferData;

int MusicVolume;

void I_ErrorCheck(const char* context)
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

int I_InitAudio()
{
	int i;
	ALDevice = alcOpenDevice(NULL);
	if (ALDevice == NULL)
	{
		Warning("I_InitAudio: Cannot open OpenAL device\n");
		return 1;
	}
	ALContext = alcCreateContext(ALDevice, 0);
	if (ALContext == NULL)
	{
		Warning("I_InitAudio: Cannot create OpenAL context\n");
		I_ShutdownAudio();
		return 1;
	}
	alcMakeContextCurrent(ALContext);

	alGenBuffers(_MAX_VOICES, &bufferNames[0]);
	I_ErrorCheck("Creating buffers");
	alGenSources(_MAX_VOICES, &sourceNames[0]);
	I_ErrorCheck("Creating sources");
	for (i = 0; i < _MAX_VOICES; i++)
	{
		alSourcef(sourceNames[i], AL_ROLLOFF_FACTOR, 0.0f);
	}
	float orientation[] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };
	alListenerfv(AL_ORIENTATION, &orientation[0]);
	I_ErrorCheck("Listener hack");
	return 0;
}

void I_ShutdownAudio()
{
	if (ALDevice)
	{
		alcMakeContextCurrent(NULL);
		if (ALContext)
			alcDestroyContext(ALContext);
		alcCloseDevice(ALDevice);
	}
}

int I_GetSoundHandle()
{
	int i;
	ALint state;
	for (i = 0; i < _MAX_VOICES; i++)
	{
		alGetSourcei(sourceNames[i], AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			alDeleteSources(1, &sourceNames[i]); //[ISB] delete the previous source before using this handle. Fixes problems with distant sounds briefly sounding loud
			alGenSources(1, &sourceNames[i]);
			return i;
		}
	}
	I_ErrorCheck("Getting handle");
	return _ERR_NO_SLOTS;
}

void I_SetSoundData(int handle, unsigned char* data, int length, int sampleRate)
{
	if (handle >= _MAX_VOICES) return;

	alSourcei(sourceNames[handle], AL_BUFFER, NULL);
	alBufferData(bufferNames[handle], AL_FORMAT_MONO8, data, length, sampleRate);
	alSourcei(sourceNames[handle], AL_BUFFER, bufferNames[handle]);
	I_ErrorCheck("Setting sound data");
}

void I_SetSoundInformation(int handle, int volume, int angle)
{
	if (handle >= _MAX_VOICES) return;
	I_SetAngle(handle, angle);
	I_SetVolume(handle, volume);
}

void I_SetAngle(int handle, int angle)
{
	if (handle >= _MAX_VOICES) return;

	float x, y;
	float flang = (angle / 65536.0f) * (3.1415927f);

	x = (float)cos(flang);
	y = (float)sin(flang);

	alSource3f(sourceNames[handle], AL_POSITION, -x, 0.0f, y);
	I_ErrorCheck("Setting sound angle");
}

void I_SetVolume(int handle, int volume)
{
	if (handle >= _MAX_VOICES) return;

	float gain = volume / 65536.0f;
	alSourcef(sourceNames[handle], AL_GAIN, gain);
	I_ErrorCheck("Setting sound volume");
}

void I_PlaySound(int handle, int loop)
{
	if (handle >= _MAX_VOICES) return;
	alSourcei(sourceNames[handle], AL_LOOPING, loop);
	alSourcePlay(sourceNames[handle]);
	I_ErrorCheck("Playing sound");
}

void I_StopSound(int handle)
{
	if (handle >= _MAX_VOICES) return;
	alSourceStop(sourceNames[handle]);
	I_ErrorCheck("Stopping sound");
}

int I_CheckSoundPlaying(int handle)
{
	if (handle >= _MAX_VOICES) return 0;
	
	int playing;
	alGetSourcei(sourceNames[handle], AL_SOURCE_STATE, &playing);
	return playing == AL_PLAYING;
}

int I_CheckSoundDone(int handle)
{
	if (handle >= _MAX_VOICES) return 0;

	int playing;
	alGetSourcei(sourceNames[handle], AL_SOURCE_STATE, &playing);
	return playing == AL_STOPPED;
}

void I_SetMusicVolume(int volume)
{
	//printf("Music volume %d\n", volume);
	MusicVolume = volume;
	if (alIsSource(MusicSource)) //[ISB] TODO okay so this isn't truly thread safe, it likely won't pose a problem, but I should fix it just in case
	{
		alSourcef(MusicSource, AL_GAIN, MusicVolume / 127.0f);
	}
}

void I_PlayHQSong(int sample_rate, std::vector<float>&& song_data, bool loop)
{
}

void I_StopHQSong()
{
}

void I_CreateMusicSource()
{
	alGenSources(1, &MusicSource);
	alSourcef(MusicSource, AL_ROLLOFF_FACTOR, 0.0f);
	alSource3f(MusicSource, AL_POSITION, 1.0f, 0.0f, 0.0f);
	alSourcef(MusicSource, AL_GAIN, MusicVolume / 127.0f);
	MusicBufferData = (ALushort*)malloc(sizeof(ALushort) * S_GetSamplesPerTick() * S_GetTicksPerSecond() * 2);
	memset(&BufferQueue[0], 0, sizeof(ALuint) * MAX_BUFFERS_QUEUED);
	I_ErrorCheck("Creating music source");

	//Immediately kick off the first buffer if possible
	int finalTicks = S_SequencerRender(S_GetTicksPerSecond(), MusicBufferData);
	alGenBuffers(1, &BufferQueue[0]);
	alBufferData(BufferQueue[0], AL_FORMAT_STEREO16, MusicBufferData, finalTicks * S_GetSamplesPerTick() * sizeof(ALushort) * 2, 44100);
	alSourceQueueBuffers(MusicSource, 1, &BufferQueue[0]);
	alSourcePlay(MusicSource);
	I_ErrorCheck("Queueing music buffers");
}

void I_DestroyMusicSource()
{
	int BuffersProcessed;
	alGetSourcei(MusicSource, AL_BUFFERS_PROCESSED, &BuffersProcessed);
	if (BuffersProcessed > 0) //Free any lingering buffers
	{
		alSourceUnqueueBuffers(MusicSource, BuffersProcessed, &BufferQueue[0]);
		alDeleteBuffers(BuffersProcessed, &BufferQueue[0]);
	}
	I_ErrorCheck("Destroying lingering music buffers");
	alDeleteSources(1, &MusicSource);
	free(MusicBufferData);
	I_ErrorCheck("Destroying music source");
}

void I_QueueMusicBuffer()
{
	std::unique_lock<std::mutex> lock(MIDIMutex);
	int BuffersProcessed;
	alGetSourcei(MusicSource, AL_BUFFERS_PROCESSED, &BuffersProcessed);
	if (BuffersProcessed > 0)
	{
		alSourceUnqueueBuffers(MusicSource, BuffersProcessed, &BufferQueue[0]);
		alDeleteBuffers(BuffersProcessed, &BufferQueue[0]);
		for (int i = 0; i < MAX_BUFFERS_QUEUED - BuffersProcessed; i++)
		{
			BufferQueue[i] = BufferQueue[i + BuffersProcessed];
		}
		I_ErrorCheck("Unqueueing music buffers");
	}
	alGetSourcei(MusicSource, AL_BUFFERS_QUEUED, &CurrentBuffers);
	if (CurrentBuffers < MAX_BUFFERS_QUEUED)
	{
		int finalTicks = S_SequencerRender(S_GetTicksPerSecond(), MusicBufferData);
		alGenBuffers(1, &BufferQueue[CurrentBuffers]);
		alBufferData(BufferQueue[CurrentBuffers], AL_FORMAT_STEREO16, MusicBufferData, finalTicks * S_GetSamplesPerTick() * sizeof(ALushort) * 2, 44100);
		alSourceQueueBuffers(MusicSource, 1, &BufferQueue[CurrentBuffers]);
		I_ErrorCheck("Queueing music buffers");
	}
}

void I_MIDIThread()
{
	std::unique_lock<std::mutex> lock(MIDIMutex);
	S_StartMIDISong(CurrentSong, LoopMusic);
	I_CreateMusicSource();
	while (!StopMIDI)
	{
		lock.unlock();
		I_QueueMusicBuffer();
		lock.lock();
	}
	S_StopSequencer();
	I_DestroyMusicSource();
}

void I_StartMIDISong(hmpheader_t* song, bool loop)
{
	std::unique_lock<std::mutex> lock(MIDIMutex);
	CurrentSong = song;
	LoopMusic = loop;
	StopMIDI = false;
	lock.unlock();
	MIDIThread = std::thread(I_MIDIThread);
}

void I_StopMIDISong()
{
	std::unique_lock<std::mutex> lock(MIDIMutex);
	StopMIDI = true;
	lock.unlock();
	MIDIThread.join();
	if (CurrentSong != NULL)
	{
		S_FreeHMPData(CurrentSong);
		free(CurrentSong);
		CurrentSong = NULL;
	}
}

#endif
