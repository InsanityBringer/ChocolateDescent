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

sequencerstate_t Sequencer;

int S_StartMIDISong(hmpheader_t* song)
{
	Sequencer.song = song;
	Sequencer.samplesPerTick = 367; //[ISB] aaaaaa
	S_RewindSequencer();

	return 0;
}

void S_RewindSequencer()
{
	midichunk_t *chunk;
	Sequencer.ticks = Sequencer.lastRenderedTick = 0;
	
	for (int i = 0; i < Sequencer.song->numChunks; i++)
	{
		chunk = &Sequencer.song->chunks[i];
		if (chunk->numEvents > 0)
		{
			chunk->nextEvent = 0;
			chunk->nextEventTime = chunk->events[0].delta;
		}
	}
}

void S_StopSequencer()
{
	I_StopAllNotes();
}

void S_SetSequencerTick(int hack)
{
	Sequencer.ticks = hack;
}

int S_GetTicksPerSecond()
{
	return 120; //[ISB] todo: different tempos?
}

int S_GetSamplesPerTick()
{
	return Sequencer.samplesPerTick;
}

//Returns the next tick that an event has to be performed
int S_SequencerTick()
{
	int i;
	int nextTick = INT_MAX;
	midichunk_t* chunk;

	for (i = 0; i < Sequencer.song->numChunks; i++)
	{
		chunk = &Sequencer.song->chunks[i];

		while (chunk->nextEvent != -1 && chunk->nextEventTime == Sequencer.ticks)
		{
			I_MidiEvent(&chunk->events[chunk->nextEvent]);
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

	return nextTick;
}

int S_SequencerRender(int ticksToRender, unsigned short* buffer)
{
	int currentTick = Sequencer.lastRenderedTick;
	int destinationTick = Sequencer.lastRenderedTick + ticksToRender;

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
		numTicks = currentTick - Sequencer.lastRenderedTick;
		//If there's still ticks to render, render them
		if (numTicks > 0)
		{
			I_RenderMIDI(numTicks, Sequencer.samplesPerTick, buffer);
			numTicksRendered += numTicks;
			Sequencer.lastRenderedTick = currentTick;
			buffer += Sequencer.samplesPerTick * numTicks * 2;
		}

		if (currentTick == Sequencer.ticks) //Still ticks to render
		{
			nextTick = S_SequencerTick();
			if (nextTick == INT_MAX)
			{
				printf("Song end hit, looping!\n");
				numTicks = Sequencer.ticks - Sequencer.lastRenderedTick; //Render as much as possible
				if (numTicks > 0)
				{
					I_RenderMIDI(numTicks, Sequencer.samplesPerTick, buffer);
					numTicksRendered += numTicks;
					Sequencer.lastRenderedTick = currentTick;
					buffer += Sequencer.samplesPerTick * numTicks * 2;
				}
				S_RewindSequencer();
				done = true;
			}
			else
				Sequencer.ticks = nextTick;
		}

		currentTick = Sequencer.ticks;
	}
	printf("Renderer ended at tick %d, sequencer at tick %d. %d ticks rendered.\n", Sequencer.lastRenderedTick, Sequencer.ticks, numTicksRendered);
	return numTicksRendered;
}