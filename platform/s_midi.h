/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt
*/
#pragma once

#include "stdint.h"

#define EVENT_NOTEOFF 8
#define EVENT_NOTEON 9
#define EVENT_AFTERTOUCH 10
#define EVENT_CONTROLLER 11
#define EVENT_PATCH 12
#define EVENT_PRESSURE 13
#define EVENT_PITCH 14
#define EVENT_SYSEX 15

//[ISB] TODO: These need to be checked against the descent.cfg constants
#define _MIDI_FM 2
#define _MIDI_OPL3 3

typedef struct
{
	int delta;

	uint8_t channel;
	uint8_t type;
	uint8_t param1;
	uint8_t param2;

	int dataLength;
	uint8_t* data;
} midievent_t;

typedef struct
{
	int chunkNum;
	int chunkLen;
	int chunkTrack;

	int numEvents;
	midievent_t* events;

	//Track sequencing data
	int nextEvent; //Next event to play, -1 if this track is finished
	int nextEventTime; //At what MIDI tick should this event be played?
} midichunk_t;

typedef struct
{
	char header[16];
	int length;
	int numChunks;
	int bpm;
	int seconds;

	midichunk_t* chunks;
} hmpheader_t;

int S_InitMusic(int device);
void S_ShutdownMusic();

uint16_t S_StartSong(int length, uint8_t* data, bool loop, uint32_t* handle);
uint16_t S_StopSong();

int S_LoadHMP(int length, uint8_t* data, hmpheader_t* song);
void S_FreeHMPData(hmpheader_t* song);
