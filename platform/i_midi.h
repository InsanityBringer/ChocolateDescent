/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#pragma once

#include "s_midi.h"

//[ISB] TODO: Move
#ifdef USE_FLUIDSYNTH

#include "fluidsynth.h"

class MidiFluidSynth : MidiSynth
{
	fluid_settings_t* FluidSynthSettings;
	fluid_synth_t* FluidSynth;
	fluid_audio_driver_t* AudioDriver;
public:
	MidiFluidSynth();
	void SetSoundfont(const char* filename);
	void SetSampleRate(uint32_t newSampleRate) override;
	int ClassifySynth() override
	{
		return MIDISYNTH_SOFT;
	}
	void DoMidiEvent(midievent_t* ev) override;
	void RenderMIDI(int numTicks, unsigned short* buffer) override;
	void StopSound() override;
	void Shutdown() override;
};

#endif