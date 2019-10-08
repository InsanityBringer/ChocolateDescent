/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fluidsynth.h"

#include "platform/i_midi.h"
#include "platform/s_midi.h"

#ifdef USE_FLUIDSYNTH

MidiFluidSynth::MidiFluidSynth()
{
	FluidSynthSettings = new_fluid_settings();
	FluidSynth = new_fluid_synth(FluidSynthSettings);
	//AudioDriver = new_fluid_audio_driver(FluidSynthSettings, FluidSynth);
	AudioDriver = nullptr;
	//I_SetSoundfontFilename(SoundFontFilename);

	//if (FluidSynth == nullptr) return 1;
}

void MidiFluidSynth::Shutdown()
{
	//delete_fluid_audio_driver(AudioDriver);
	delete_fluid_synth(FluidSynth);
	delete_fluid_settings(FluidSynthSettings);
}

void MidiFluidSynth::SetSoundfont(const char* filename)
{
	fluid_synth_sfload(FluidSynth, filename, 1);
}

void MidiFluidSynth::RenderMIDI(int numTicks, int samplesPerTick, unsigned short* buffer)
{
	fluid_synth_write_s16(FluidSynth, numTicks * samplesPerTick, (void*)buffer, 0, 2, (void*)buffer, 1, 2);
}

//I_MidiEvent(chunk->events[chunk->nextEvent]);
void MidiFluidSynth::DoMidiEvent(midievent_t *ev)
{
	switch (ev->type)
	{
	case EVENT_NOTEON:
		fluid_synth_noteon(FluidSynth, ev->channel, ev->param1, ev->param2);
		break;
	case EVENT_NOTEOFF:
		fluid_synth_noteoff(FluidSynth, ev->channel, ev->param1);
		break;
	case EVENT_AFTERTOUCH:
		fluid_synth_key_pressure(FluidSynth, ev->channel, ev->param1, ev->param2);
		break;
	case EVENT_PRESSURE:
		fluid_synth_channel_pressure(FluidSynth, ev->channel, ev->param1);
		break;
	case EVENT_PITCH:
		fluid_synth_pitch_bend(FluidSynth, ev->channel, ev->param1 + (ev->param2 << 7));
		break;
	case EVENT_PATCH:
		fluid_synth_program_change(FluidSynth, ev->channel, ev->param1);
		//fluid_synth_program_change(FluidSynth, ev->channel, rand() & 127);
		break;
	case EVENT_CONTROLLER:
		fluid_synth_cc(FluidSynth, ev->channel, ev->param1, ev->param2);
		break;
		//[ISB] TODO: Sysex
	}
}

void MidiFluidSynth::StopSound()
{
	for (int chan = 0; chan < 16; chan++)
	{
		fluid_synth_cc(FluidSynth, chan, 0x79, 0);
		fluid_synth_cc(FluidSynth, chan, 0x7B, 0);
	}
}

#endif