/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#pragma once

#include "platform/s_midi.h"

#include <Windows.h>

class MidiWin32Synth : MidiSynth
{
	HMIDIOUT OutputDevice;
	uint8_t VolumeLevels[16];
	int MasterVolume;

public:
	MidiWin32Synth();
	void SetSoundfont(const char* filename);
	void SetSampleRate(uint32_t newSampleRate) override;
	void CreateSynth() override;
	int ClassifySynth() override
	{
		return MIDISYNTH_LIVE;
	}
	void DoMidiEvent(midievent_t* ev) override;
	void RenderMIDI(int numTicks, unsigned short* buffer) override;
	void StopSound() override;
	void Shutdown() override;
	void SetDefaults() override;
	void PerformBranchResets(BranchEntry* entry, int chan) override;
	void SetVolume(int volume) override;
};