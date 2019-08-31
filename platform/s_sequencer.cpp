#include <stdlib.h>
#include <limits.h>

#include "s_midi.h"
#include "s_sequencer.h"

sequencerstate_t Sequencer;

int S_StartMIDISong(hmpheader_t* song)
{
	Sequencer.ticks = 0;
	Sequencer.song = song;
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

		if (chunk->nextEvent != -1 && chunk->nextEventTime == Sequencer.ticks)
		{
			//I_MidiEvent(chunk->events[chunk->nextEvent]);
			chunk->nextEvent++; 
			if (chunk->nextEvent >= chunk->numEvents) 
				chunk->nextEvent == -1;
			else
			{
				chunk->nextEventTime += chunk->events[chunk->nextEvent].delta;
				if (chunk->nextEventTime < nextTick)
					nextTick = chunk->nextEventTime;
			}
		}
	}

	return nextTick;
}

void S_SequencerRender(int numTicks, float* lbuffer, float* rbuffer)
{

}