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
} sequencerstate_t;