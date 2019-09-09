
/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt
*/
#pragma once

#include "s_midi.h"

typedef struct
{
	hmpheader_t* song; //Song currently playing
	int ticks; //Number of ticks passed so far. 
	int nextTick; //The next MIDI tick where an event actually occurs. 
	int lastRenderedTick; //Where rendering last left off

	int sampleRate; //Sample rate of the current song
	//[ISB] TODO: This is imprecise, ugh. Song is slightly faster than it should be...
	int samplesPerTick; //Amount of samples per each MIDI tick
} sequencerstate_t;

int S_StartMIDISong(hmpheader_t* song);
void S_StopSequencer();
void S_RewindSequencer();
int S_GetTicksPerSecond();
int S_GetSamplesPerTick();
void S_SetSequencerTick(int hack);
int S_SequencerTick();
int S_SequencerRender(int ticksToRender, unsigned short* buffer);