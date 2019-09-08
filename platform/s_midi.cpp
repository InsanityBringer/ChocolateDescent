/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "s_midi.h"
#include "mem/mem.h"
#include "misc/byteswap.h"
#include "misc/error.h"

//[ISB] Uncomment to enable MIDI file diagonstics. Extremely slow on windows. And probably linux tbh.
//Will probably overflow your console buffer, so make it really long if you must
//#define DEBUG_MIDI_READING

#ifdef DEBUG_MIDI_READING
char* EventNames[] = { "Unknown 0", "Unknown 1", "Unknown 2", "Unknown 3", "Unknown 4", "Unknown 5",
"Unknown 6", "Unknown 7", "Note Off", "Note On", "Aftertouch", "Controller", "Patch", "Pressure",
"Pitch", "Sysex" };

void S_PrintEvent(midievent_t* ev)
{
	if (ev->type == 0xFF || ev->type == EVENT_CONTROLLER)
	{
		if (ev->type != 0xFF)
		{
			fprintf(stderr, "  Event Type: %s\n  Affected channel: %d\n", EventNames[ev->type], ev->channel);
		}
		else
		{
			fprintf(stderr, "  Event Type: Meta\n");
		}
		fprintf(stderr, "  Delta: %d\n", ev->delta);

		switch (ev->type)
		{
		case EVENT_NOTEOFF:
		case EVENT_NOTEON:
		case EVENT_AFTERTOUCH:
			fprintf(stderr, "   Note: %d\n   Velocity: %d\n", ev->param1, ev->param2);
			break;
		case EVENT_CONTROLLER:
			fprintf(stderr, "   Controller ID: %d\n   Controller value: %d\n", ev->param1, ev->param2);
			break;
		case EVENT_PATCH:
			fprintf(stderr, "   Patch: %d\n", ev->param1);
			break;
		case EVENT_PITCH:
			fprintf(stderr, "   Pitch: %d\n", ev->param1 + (ev->param2 << 8));
			break;
		case 0xff:
			fprintf(stderr, "   Type: %d\n   Length: %d\n", ev->param1, ev->dataLength);
			break;
		}
	}
}

#endif

int S_InitMusic(int device)
{
	return 0;
}

int S_ReadHMPChunk(int pointer, uint8_t* data, midichunk_t* chunk)
{
	int done, oldpointer, position, value, i, shift;
	int command;
	uint8_t b;
	int eventCount = 0;
	int basepointer = pointer; //[ISB] this code really needs to be made less terrible tbh

	chunk->chunkNum = BS_MakeInt(&data[pointer]);
	chunk->chunkLen = BS_MakeInt(&data[pointer + 4]);
	chunk->chunkTrack = BS_MakeInt(&data[pointer + 8]);

#ifdef DEBUG_MIDI_READING
	fprintf(stderr, "Chunk %d:\n Length: %d\n Track: %d\n Events:\n", chunk->chunkNum, chunk->chunkLen, chunk->chunkTrack);
#endif

	//Count events. Couldn't we have gotten something useful in the header like an event count? eh
	pointer += 12; oldpointer = pointer;
	while (pointer < (basepointer + chunk->chunkLen))
	{
		done = 0; position = 0; value = 0;
		while (!done)
		{
			b = data[pointer];
			if (b & 0x80) //We're done now
			{
				done = 1;
				b &= 0x7F;
			}
			value += (b << (position * 8));
			position++;
			pointer++;
		}
		b = data[pointer]; pointer++;
		if (b == 0xff)
			command = 0xff;
		else
			command = (b >> 4) & 0xf;

		switch (command) //For now, just advance the pointer enough
		{
		case EVENT_NOTEOFF:
		case EVENT_NOTEON:
		case EVENT_AFTERTOUCH:
		case EVENT_CONTROLLER:
		case EVENT_PITCH:
			pointer += 2;
			break;
		case EVENT_PATCH:
		case EVENT_PRESSURE:
			pointer += 1;
			break;
		case 0xff:
			pointer++; //skip command
			value = data[pointer]; pointer += value + 1;
			break;
		default:
			fprintf(stderr, "Unknown event %d in hmp file", command);
			break;
		}

		eventCount++;
	}

	//Actually allocate and read events now
	chunk->numEvents = eventCount;
	chunk->events = (midievent_t*)malloc(sizeof(midievent_t) * eventCount);

	if (chunk->events == nullptr)
	{
		fprintf(stderr, "Can't allocate MIDI events");
		exit(1);
	}

	pointer = oldpointer;

	for (i = 0; i < eventCount; i++)
	{
		done = 0; position = 0; value = 0;
		memset(&chunk->events[i], 0, sizeof(midievent_t));
		while (!done)
		{
			b = data[pointer];
			if (b & 0x80) //We're done now
			{
				done = 1;
				//b &= 0x7F;
			}
			value += ((b & 0x7F) << position);
			position+=7;
			pointer++;
		}
		chunk->events[i].delta = value;
		b = data[pointer]; pointer++;
		if (b == 0xff)
			command = 0xff;
		else
			command = (b >> 4) & 0xf;
		chunk->events[i].type = command;
		chunk->events[i].channel = b & 0xf;

		switch (command) //Actually load the params now
		{
		case EVENT_NOTEOFF:
		case EVENT_NOTEON:
		case EVENT_AFTERTOUCH:
		case EVENT_CONTROLLER:
		case EVENT_PITCH:
			chunk->events[i].param1 = data[pointer++];
			chunk->events[i].param2 = data[pointer++];
			break;
		case EVENT_PATCH:
		case EVENT_PRESSURE:
			chunk->events[i].param1 = data[pointer++];
			break;
		case 0xff:
			chunk->events[i].param1 = data[pointer++];
			chunk->events[i].dataLength = data[pointer++];
			if (chunk->events[i].dataLength != 0)
			{
				chunk->events[i].data = (uint8_t*)malloc(chunk->events[i].dataLength * sizeof(uint8_t));
				memcpy(chunk->events[i].data, &data[pointer], chunk->events[i].dataLength); pointer += chunk->events[i].dataLength;
			}
			break;
		default:
			fprintf(stderr, "Unknown event %d in hmp file", command);
			break;
		}

#ifdef DEBUG_MIDI_READING
		S_PrintEvent(&chunk->events[i]);
#endif
	}

	//Ready the chunk for sequencing
	if (chunk->numEvents != 0)
	{
		chunk->nextEvent = 0;
		chunk->nextEventTime = chunk->events[0].delta;
		//chunk->nextEventTime = 0;
	}

	return pointer;
}

void S_FreeHMPData(hmpheader_t* song)
{
	int i, j;
	midichunk_t* chunk;
	midievent_t* ev;
	for (i = 0; i < song->numChunks; i++)
	{
		chunk = &song->chunks[i];
		//Need to check for special events. Agh
		for (j = 0; j < chunk->numEvents; j++)
		{
			ev = &chunk->events[i];
			if (ev->dataLength != 0)
			{
				free(ev->data);
			}
		}
		free(chunk);
	}
}

int S_LoadHMP(int length, uint8_t* data, hmpheader_t* song)
{
	int i, pointer;
	memcpy(song->header, data, 16); //Check for version 1 HMP
	if (strcmp(song->header, "HMIMIDIP"))
	{
		return 1;
	}
	song->length = BS_MakeInt(&data[32]);
	song->numChunks = BS_MakeInt(&data[48]);
	song->bpm = BS_MakeInt(&data[56]);
	song->seconds = BS_MakeInt(&data[60]);

	song->chunks = (midichunk_t*)malloc(sizeof(midichunk_t) * song->numChunks);

	if (song->chunks == nullptr)
	{
		fprintf(stderr, "Can't allocate MIDI chunk list");
		exit(1);
	}

	pointer = 776;
	for (i = 0; i < song->numChunks; i++)
	{
		pointer = S_ReadHMPChunk(pointer, data, &song->chunks[i]);
	}

	return 0;
}