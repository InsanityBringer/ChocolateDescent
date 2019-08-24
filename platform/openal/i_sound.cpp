/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef USE_OPENAL

#include "AL/al.h"
#include "AL/alc.h"
#include "platform/i_sound.h"

ALCdevice *ALDevice = NULL;
ALCcontext *ALContext = NULL;

ALuint bufferNames[_MAX_VOICES];
ALuint sourceNames[_MAX_VOICES];

void I_ErrorCheck(char* context)
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
	float flang = (angle / 65536.0f) * (3.1415927f * 2);

	x = (float)cos(flang) * .05f;
	y = (float)sin(flang) * .05f;

	alSource3f(sourceNames[handle], AL_POSITION, -y, 0.0f, x);
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

#else

#include "platform/i_sound.h"

void I_ErrorCheck(char* context) { }
int I_InitAudio() { return 0; }
void I_ShutdownAudio() { }
int I_GetSoundHandle() { return 0; }
void I_SetSoundData(int handle, unsigned char* data, int length, int sampleRate) { }
void I_SetSoundInformation(int handle, int volume, int angle) { }
void I_SetAngle(int handle, int angle) { }
void I_SetVolume(int handle, int volume) { }
void I_PlaySound(int handle, int loop) { }
void I_StopSound(int handle) { }
int I_CheckSoundPlaying(int handle) { return 0; }
int I_CheckSoundDone(int handle) { return 0; }

#endif
