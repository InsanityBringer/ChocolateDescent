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

#include "fluid_midi.h"
#include "platform/s_midi.h"

#ifdef USE_FLUIDSYNTH

MidiFluidSynth::MidiFluidSynth()
{
	FluidSynthSettings = new_fluid_settings();
	FluidSynth = new_fluid_synth(FluidSynthSettings);
	//AudioDriver = new_fluid_audio_driver(FluidSynthSettings, FluidSynth);
	AudioDriver = nullptr;

	if (FluidSynthSettings)
	{
		fluid_settings_setint(FluidSynthSettings, "synth.chorus.active", 0);
		fluid_settings_setint(FluidSynthSettings, "synth.reverb.active", 0);
		fluid_settings_setnum(FluidSynthSettings, "synth.gain", 0.5);
	}

	//if (FluidSynth == nullptr) return 1;
}

void MidiFluidSynth::SetSampleRate(uint32_t newSampleRate)
{
	fluid_settings_setnum(FluidSynthSettings, "synth.sample-rate", (double)newSampleRate);
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

void MidiFluidSynth::RenderMIDI(int numTicks, unsigned short* buffer)
{
	fluid_synth_write_s16(FluidSynth, numTicks, (void*)buffer, 0, 2, (void*)buffer, 1, 2);
}

//I_MidiEvent(chunk->events[chunk->nextEvent]);
void MidiFluidSynth::DoMidiEvent(midievent_t *ev)
{
	//printf("event %d channel %d param 1 %d param 2 %d\n", ev->type, ev->channel, ev->param1, ev->param2);
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
	//SetDefaults();
	for (int chan = 0; chan < 16; chan++)
	{
		fluid_synth_cc(FluidSynth, chan, 123, 0);
	}
}

void MidiFluidSynth::SetDefaults()
{
	//fluid_synth_system_reset(FluidSynth);
	for (int chan = 0; chan < 16; chan++)
	{
		fluid_synth_cc(FluidSynth, chan, 7, 0); //volume. Set to 0 by default in HMI
		fluid_synth_cc(FluidSynth, chan, 39, 0); //fine volume.
		fluid_synth_cc(FluidSynth, chan, 1, 0); //modulation wheel
		fluid_synth_cc(FluidSynth, chan, 11, 127); //expression
		fluid_synth_cc(FluidSynth, chan, 64, 0); //pedals
		fluid_synth_cc(FluidSynth, chan, 65, 0);
		fluid_synth_cc(FluidSynth, chan, 66, 0);
		fluid_synth_cc(FluidSynth, chan, 67, 0);
		fluid_synth_cc(FluidSynth, chan, 68, 0);
		fluid_synth_cc(FluidSynth, chan, 69, 0);
		fluid_synth_cc(FluidSynth, chan, 42, 64); //pan

		fluid_synth_channel_pressure(FluidSynth, chan, 0);
		fluid_synth_pitch_bend(FluidSynth, chan, 0x2000);
		fluid_synth_bank_select(FluidSynth, chan, 0);
		fluid_synth_program_change(FluidSynth, chan, 0);

		for (int key = 0; key < 128; key++) //this isn't going to cause problems with the polyphony limit is it...
			fluid_synth_key_pressure(FluidSynth, chan, key, 0);

		fluid_synth_cc(FluidSynth, chan, 121, 0); //send all notes off TODO: This can be adjusted in HMI. 
	}
}

void MidiFluidSynth::PerformBranchResets(BranchEntry* entry, int chan)
{
	int i;
	for (i = 0; i < entry->controlChangeCount; i++)
	{
		fluid_synth_cc(FluidSynth, chan, entry->controlChanges[i].controller, entry->controlChanges[i].state);
	}
}

#endif
