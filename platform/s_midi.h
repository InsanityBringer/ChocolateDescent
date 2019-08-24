/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt
*/
#pragma once

#include "misc/types.h"

#define EVENT_NOTEOFF 8
#define EVENT_NOTEON 9
#define EVENT_AFTERTOUCH 10
#define EVENT_CONTROLLER 11
#define EVENT_PATCH 12
#define EVENT_PRESSURE 13
#define EVENT_PITCH 14
#define EVENT_SYSEX 15

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

int S_LoadHMP(int length, uint8_t* data);
