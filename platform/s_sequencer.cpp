/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/
//[ISB] I must give the ZDoom source credit here,
//as the overall structure of the sequencer was inspired from ZDoom's
//soft synth sequencer. 
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "s_midi.h"
#include "s_sequencer.h"

#include "i_midi.h"

//sequencerstate_t Sequencer;

MidiSequencer::MidiSequencer(MidiSynth* newSynth)
{
	synth = newSynth;
}

int MidiSequencer::StartSong(hmpheader_t* newSong, bool newLoop)
{
	song = newSong;
	loop = newLoop;
	samplesPerTick = 367; //[ISB] aaaaaa
	RewindSong();

	return 0;
}

int S_StartMIDISong(hmpheader_t* song, bool loop)
{
	fprintf(stderr, "S_StartMIDISong: STUB\n");
	return 0;
}

void MidiSequencer::RewindSong()
{
	midichunk_t* chunk;
	ticks = lastRenderedTick = 0;

	for (int i = 0; i < song->numChunks; i++)
	{
		chunk = &song->chunks[i];
		if (chunk->numEvents > 0)
		{
			chunk->nextEvent = 0;
			chunk->nextEventTime = chunk->events[0].delta;
		}
	}
}

void S_RewindSequencer()
{
	fprintf(stderr, "S_RewindSequencer: STUB\n");
}

void MidiSequencer::StopSong()
{
	synth->StopSound();
}

void S_StopSequencer()
{
	fprintf(stderr, "S_StopSequencer: STUB\n");
}

void S_SetSequencerTick(int hack)
{
	fprintf(stderr, "S_SetSequencerTick: STUB\n");
}

int S_GetTicksPerSecond()
{
	return 120; //[ISB] todo: different tempos?
}

int S_GetSamplesPerTick()
{
	fprintf(stderr, "S_GetSamplesPerTick: STUB\n");
	return 0;//Sequencer.samplesPerTick;
}

int MidiSequencer::Tick()
{
	int i;
	int nextTick = INT_MAX;
	midichunk_t* chunk;

	for (i = 0; i < song->numChunks; i++)
	{
		chunk = &song->chunks[i];

		while (chunk->nextEvent != -1 && chunk->nextEventTime == ticks)
		{
			synth->DoMidiEvent(&chunk->events[chunk->nextEvent]);
			//chunk->nextEventTime += chunk->events[chunk->nextEvent].delta;
			chunk->nextEvent++;
			if (chunk->nextEvent >= chunk->numEvents)
				chunk->nextEvent = -1;
			else
			{
				chunk->nextEventTime += chunk->events[chunk->nextEvent].delta;
			}
		}

		if (chunk->nextEvent != -1) //Need to evalulate all chunks, even ones that didn't have events played this current tick, to be sure the next event time is accurate
		{
			if (chunk->nextEventTime < nextTick /*&& chunk->nextEventTime != Sequencer.ticks*/) //[ISB] hack, otherwise multiple events on one tick cause problems
				nextTick = chunk->nextEventTime;
		}
	}

	//[ISB] hack
	if (synth->ClassifySynth() == MIDISYNTH_LIVE)
		ticks = nextTick;

	return nextTick;
}

//Returns the next tick that an event has to be performed
int S_SequencerTick()
{
	fprintf(stderr, "S_SequencerTick: STUB\n");
	return 0;
}

int MidiSequencer::Render(int ticksToRender, unsigned short* buffer)
{
	int currentTick = lastRenderedTick;
	int destinationTick = lastRenderedTick + ticksToRender;

	int numTicks;
	int numTicksRendered = 0;
	int nextTick;

	bool done = false;

	while (!done)
	{
		//currentTick = Sequencer.ticks;
		if (currentTick > destinationTick)
		{
			currentTick = destinationTick;
			done = true; //Abort from the loop when the buffer will be filled up
		}
		numTicks = currentTick - lastRenderedTick;
		//If there's still ticks to render, render them
		if (numTicks > 0)
		{
			synth->RenderMIDI(numTicks, samplesPerTick, buffer);
			numTicksRendered += numTicks;
			lastRenderedTick = currentTick;
			buffer += samplesPerTick * numTicks * 2;
		}

		if (currentTick == ticks) //Still ticks to render
		{
			nextTick = Tick();
			if (nextTick == INT_MAX && loop)
			{
				//printf("Song end hit, looping!\n");
				numTicks = ticks - lastRenderedTick; //Render as much as possible
				if (numTicks > 0)
				{
					synth->RenderMIDI(numTicks, samplesPerTick, buffer);
					numTicksRendered += numTicks;
					lastRenderedTick = currentTick;
					buffer += samplesPerTick * numTicks * 2;
				}
				S_RewindSequencer();
				done = true;
			}
			else
				ticks = nextTick;
		}

		currentTick = ticks;
	}
	//printf("Renderer ended at tick %d, sequencer at tick %d. %d ticks rendered.\n", lastRenderedTick, ticks, numTicksRendered);
	return numTicksRendered;
}

int S_SequencerRender(int ticksToRender, unsigned short* buffer)
{
	fprintf(stderr, "S_SequencerRender: STUB\n");
	return 0;
}