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

MidiSequencer::MidiSequencer(MidiSynth* newSynth, int newSampleRate)
{
	synth = newSynth;
	lastRenderedTick = nextTick = ticks = 0;
	song = nullptr;
	loop = false;
	sampleRate = newSampleRate;
}

MidiSynth* MidiSequencer::GetSynth()
{
	return synth;
}

int MidiSequencer::StartSong(hmpheader_t* newSong, bool newLoop)
{
	song = newSong;
	loop = newLoop;
	RewindSong(false);

	return 0;
}

void MidiSequencer::RewindSong(bool resetLoop)
{
	midichunk_t* chunk;
	ticks = lastRenderedTick = 0;
	int cumlTime;

	for (int i = 0; i < song->numChunks; i++)
	{
		chunk = &song->chunks[i];
		cumlTime = 0;
		if (chunk->numEvents > 0)
		{
			if (resetLoop)
			{
				chunk->nextEvent = 0;
				chunk->nextEventTime = chunk->events[0].delta;
				//Find the first event at or after the loop
				//This will mess up on unaligned events, they'll play too early.
				//Dan Wentz implies that events need to be aligned when discussing Descent MIDI composition,
				//so maybe it's safe to assume HMI SOS would do the same? This should be investigated in detail. 
				while (cumlTime < song->loopStart)
				{
					chunk->nextEvent++;
					if (chunk->nextEvent >= chunk->numEvents) //not enough events
					{
						chunk->nextEvent = -1;
						break;
					}
					cumlTime += chunk->nextEventTime;
					chunk->nextEventTime = chunk->events[chunk->nextEvent].delta;
				}
			}
			else
			{
				chunk->nextEvent = 0;
				chunk->nextEventTime = chunk->events[0].delta;
			}
		}
	}
}

void MidiSequencer::StopSong()
{
	synth->StopSound();
	song = nullptr;
}

uint64_t MidiSequencer::Tick()
{
	int i;
	int nextTick = INT_MAX;
	midichunk_t* chunk;
	midievent_t* ev;

	for (i = 0; i < song->numChunks; i++)
	{
		chunk = &song->chunks[i];

		while (chunk->nextEvent != -1 && chunk->nextEventTime == ticks)
		{
			ev = &chunk->events[chunk->nextEvent];
			//loop hack
			if (loop) //Specs say to have loops on track 1 but this seems to vary some? I'm very frequently seeing it on Track 2
			{
				if (ev->type == EVENT_CONTROLLER && ev->param1 == 111)
				{
					//Cause an immediate rewind
					//TODO: Evalulate whether or not the events on this tick should be played. descent2.com is unclear. 
					return UINT64_MAX;
				}
			}
			synth->DoMidiEvent(ev);
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
int MidiSequencer::Render(int ticksToRender, unsigned short* buffer)
{
	//If there's no song, just render out the requested amount of ticks so that lingering notes fade out even over the fade to black
	if (!song)
	{
		synth->RenderMIDI(ticksToRender, buffer);
		return ticksToRender;
	}

	uint64_t currentTick = lastRenderedTick;
	uint64_t destinationTick = lastRenderedTick + ticksToRender;

	uint64_t numTicks;
	uint64_t numTicksRendered = 0;
	uint64_t nextTick;

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
			synth->RenderMIDI(numTicks, buffer);
			numTicksRendered += numTicks;
			lastRenderedTick = currentTick;
			buffer += numTicks * 2;
		}

		if (currentTick == ticks) //Still ticks to render
		{
			nextTick = Tick();
			if (nextTick == UINT64_MAX && loop)
			{
				//printf("Song end hit, looping!\n");
				numTicks = ticks - lastRenderedTick; //Render as much as possible
				if (numTicks > 0)
				{
					synth->RenderMIDI(numTicks, buffer);
					numTicksRendered += numTicks;
					lastRenderedTick = currentTick;
					buffer += numTicks * 2;
				}
				RewindSong(true);
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