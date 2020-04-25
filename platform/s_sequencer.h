
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
	bool loop; //Whether or not the song should loop at the end or not
} sequencerstate_t;

class MidiSequencer
{
	hmpheader_t* song;
	int ticks;
	uint64_t nextTick;
	uint64_t lastRenderedTick;

	int sampleRate;
	//[ISB] TODO: This is imprecise, ugh. Song is slightly faster than it should be...
	bool loop; //Whether or not the song should loop at the end or not

	MidiSynth* synth;

public:
	MidiSequencer(MidiSynth* newSynth, int newSampleRate);

	int StartSong(hmpheader_t* song, bool loop);
	void StopSong();
	//if resetLoop is true, the song is rewound to the point in time set by the loop point. If not, it is rewound to the beginning of the song. 
	void RewindSong(bool resetLoop);

	uint64_t Tick();
	int Render(int ticksToRender, unsigned short* buffer);
	MidiSynth* GetSynth();
};