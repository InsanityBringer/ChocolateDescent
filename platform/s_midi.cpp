/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <mutex>

#include "platform/i_sound.h"
#include "platform/i_midi.h"
#include "s_midi.h"
#include "s_sequencer.h"
//#include "mem/mem.h" //[ISB] not thread safe
#include "misc/byteswap.h"
#include "misc/error.h"
#include "platform/timer.h"

//[ISB] Uncomment to enable MIDI file diagonstics. Extremely slow on windows. And probably linux tbh.
//Will probably overflow your console buffer, so make it really long if you must
//#define DEBUG_MIDI_READING

int CurrentDevice = 0;

char SoundFontFilename[256] = "TestSoundfont.sf2";
MidiPlayer *player = nullptr;
MidiSynth* synth = nullptr;
MidiSequencer* sequencer = nullptr;
std::thread* midiThread = nullptr;

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

MidiPlayer::MidiPlayer(MidiSequencer* newSequencer, MidiSynth* newSynth)
{
	sequencer = newSequencer;
	synth = newSynth;
	nextSong = curSong = nullptr;
	nextLoop = false;
	nextTimerTick = 0;
	shouldEnd = false;
	shouldStop = false;
	initialized = false;

	hasEnded = false;
	hasChangedSong = false;

	songBuffer = new uint16_t[MIDI_TICKSPERSECOND * MIDI_SAMPLESPERTICK * 2];
}

bool MidiPlayer::IsError()
{
	return songBuffer == nullptr;
}

void MidiPlayer::SetSong(hmpheader_t* newSong, bool loop)
{
	std::unique_lock<std::mutex> lock(songMutex);
	if (!initialized) return; //already ded
	nextSong = newSong;
	nextLoop = loop;
	lock.unlock();
	//[ISB] I don't like this, but I can't think of a better way to do it atm...
	//This is very contestable, but maybe we can get away with it by merit of this only happening on main thread?
	while (!hasChangedSong);
	hasChangedSong = false;

	//[ISB] This should be on the main thread, so lets just check for an error now and die
	if (IsError())
	{
		Shutdown();
		Error("MidiPlayer::SetSong: Couldn't allocate buffer for music playback\n");
	}
}

void MidiPlayer::StopSong()
{
	std::unique_lock<std::mutex> lock(songMutex);
	if (!initialized) return; //already ded
	shouldStop = true;
	lock.unlock();
	//[ISB] I need to learn how to write threaded programs tbh
	//Avoid race condition by waiting for the MIDI thread to get the message
	while (!hasChangedSong);
	hasChangedSong = false;
}

void MidiPlayer::Shutdown()
{
	std::unique_lock<std::mutex> lock(songMutex);
	if (!initialized) return; //already ded
	shouldEnd = true;
	lock.unlock();
	while (!hasEnded);
	hasEnded = false; 
	midiThread->join();
	sequencer->StopSong();
	synth->Shutdown();
}

void MidiPlayer::Run()
{
	initialized = true;
	nextTimerTick = I_GetUS();
	I_StartMIDISong();

	for (;;)
	{
		//printf("I'm goin\n");
		std::unique_lock<std::mutex> lock(songMutex);
		if (shouldEnd)
		{
			initialized = false;
			break;
		}
		else if (shouldStop)
		{
			if (curSong)
			{
				sequencer->StopSong();
				S_FreeHMPData(curSong);
			}
			shouldStop = false;
			curSong = nullptr;
			hasChangedSong = true;
		}
		else if (nextSong)
		{
			if (curSong)
			{
				sequencer->StopSong();
				S_FreeHMPData(curSong);
			}
			//printf("Starting new song\n");
			sequencer->StartSong(nextSong, nextLoop);
			if (songBuffer)
			{
				delete[] songBuffer;
			}
			songBuffer = new uint16_t[6 * (44100 / nextSong->bpm) * 2];
			//I_StartMIDISong(nextSong, nextLoop);
			curSong = nextSong;
			nextSong = nullptr;
			hasChangedSong = true;
		}
		lock.unlock();

		//Soft synth operation
		if (synth->ClassifySynth() == MIDISYNTH_SOFT)
		{
			I_DequeueMusicBuffers();
			if (I_CanQueueMusicBuffer())
			{
				int ticks = sequencer->Render(5, songBuffer);
				if (curSong != nullptr)
					I_QueueMusicBuffer(ticks, MIDI_SAMPLERATE / curSong->bpm, songBuffer);
				else //[ISB] current design of the stupid midi playback code can't handle being starved... I need to junk it and rewrite. 
					I_QueueMusicBuffer(ticks, MIDI_SAMPLESPERTICK, songBuffer);
			}
		}

		uint64_t startTick = I_GetUS();
		uint64_t numTicks = nextTimerTick - startTick;
		if (numTicks > 2000) //[ISB] again inspired by dpJudas, with 2000 US number from GZDoom
			I_DelayUS(numTicks - 2000);
		while (I_GetUS() < nextTimerTick);
		if (curSong == nullptr) //no song, run at 120hz
			nextTimerTick += 8333;
		else
			nextTimerTick += 1000000 / curSong->bpm;
	}
	I_StopMIDISong();
	shouldEnd = false;
	hasEnded = true;
	//printf("Midi thread rip\n");
}

int S_InitMusic(int device)
{
	if (CurrentDevice != 0) return 0; //already initalized
	//[ISB] TODO: I really need to add a switcher to allow the use of multiple synths. Agh
	//I_InitMIDI();
	MidiSynth* synth = nullptr;
	switch (device)
	{
	case _MIDI_GEN:
	{
#ifdef USE_FLUIDSYNTH
		MidiFluidSynth* fluidSynth = new MidiFluidSynth();
		if (fluidSynth == nullptr)
		{
			Error("S_InitMusic: Cannot allocate fluid synth");
			return 1;
		}
		fluidSynth->SetSoundfont(SoundFontFilename);
		synth = (MidiSynth*)fluidSynth;
#else
		synth = new DummyMidiSynth();
#endif
	}
		break;
	}
	if (synth == nullptr)
	{
		Warning("S_InitMusic: Unknown device\n");
		return 1;
	}

	sequencer = new MidiSequencer(synth);

	if (sequencer == nullptr)
	{
		Error("S_InitMusic: Cannot allocate sequencer");
		return 1;
	}
	player = new MidiPlayer(sequencer, synth);

	if (player == nullptr || player->IsError())
	{
		Error("S_InitMusic: Cannot allocate MIDI player");
		return 1;
	}

	CurrentDevice = device;

	//Kick off the MIDI thread
	midiThread = new std::thread(&MidiPlayer::Run, player);

	return 0;
}

void S_ShutdownMusic()
{
	if (CurrentDevice == 0) return;
	//I_ShutdownMIDI();
	if (player != nullptr)
	{
		player->Shutdown();
	}
	if (midiThread)
	{
		delete midiThread;
	}
	CurrentDevice = 0;
}

int S_ReadHMPChunk(int pointer, uint8_t* data, midichunk_t* chunk, hmpheader_t* song)
{
	int done, oldpointer, position, value, i;
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
	if (chunk->events == NULL)
	{
		Error("Out of memory allocating a chunk's events");
		return pointer;
	}
	memset(chunk->events, 0, sizeof(midievent_t) * eventCount);

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

			if (command == EVENT_CONTROLLER && chunk->events[i].param1 == 110)
			{
				int j;
				if (song->loopStart == 0) //descent 1 game02.hmp has two loop points. uh?
				{
					for (j = 0; j < i; j++)
					{
						song->loopStart += chunk->events[j].delta;
					}
				}
				//printf("Loop start on track %d channel %d, event %d with delta %d. Loop start at %d\n", chunk->chunkNum, chunk->events[i].channel, i, value, song->loopStart);
			}
			//if (command == EVENT_CONTROLLER && chunk->events[i].param1 == 111)
				//printf("Loop end on track %d channel %d, event %d with delta %d\n", chunk->chunkNum, chunk->events[i].channel, i, value);
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
			ev = &chunk->events[j];
			if (ev->dataLength != 0)
			{
				free(ev->data);
			}
		}
		free(chunk->events);
	}
	free(song->chunks);
}

uint16_t S_StartSong(int length, uint8_t* data, bool loop, uint32_t* handle)
{
	if (CurrentDevice == 0) return 1;
	hmpheader_t* song = (hmpheader_t*)malloc(sizeof(hmpheader_t));
	if (S_LoadHMP(length, data, song)) 
	{
		*handle = 0xFFFF;
		//Failed to load MIDI, oops
		free(song);
		return 1;
	}
	*handle = 0; //heh
	player->SetSong(song, loop);
	//I_StartMIDISong(song, loop);
	return 0;
}

uint16_t S_StopSong()
{
	if (CurrentDevice == 0) return 1;
	
	player->StopSong();
	//I_StopMIDISong();
	return 0;
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
	//printf("bpm %d\n", song->bpm);
	song->seconds = BS_MakeInt(&data[60]);
	song->loopStart = 0;

	song->chunks = (midichunk_t*)malloc(sizeof(midichunk_t) * song->numChunks);
	if (song->chunks == nullptr)
	{
		Error("Can't allocate MIDI chunk list\n");
		return 1;
	}
	memset(song->chunks, 0, sizeof(midichunk_t) * song->numChunks);

	pointer = 776;
	for (i = 0; i < song->numChunks; i++)
	{
		pointer = S_ReadHMPChunk(pointer, data, &song->chunks[i], song);
	}

	return 0;
}