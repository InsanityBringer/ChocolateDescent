#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fluidsynth.h"

#include "i_midi.h"
#include "s_midi.h"

fluid_settings_t *FluidSynthSettings;
fluid_synth_t *FluidSynth;
fluid_audio_driver_t* AudioDriver;

int I_InitMIDI()
{
	FluidSynthSettings = new_fluid_settings();
	FluidSynth = new_fluid_synth(FluidSynthSettings);
	//AudioDriver = new_fluid_audio_driver(FluidSynthSettings, FluidSynth);

	if (FluidSynth == nullptr) return 1;

	return 0;
}

void I_ShutdownMIDI()
{
	//delete_fluid_audio_driver(AudioDriver);
	delete_fluid_synth(FluidSynth);
	delete_fluid_settings(FluidSynthSettings);
}

void I_SetSoundfontFilename(const char* filename)
{
	fluid_synth_sfload(FluidSynth, filename, 1);
}

void I_RenderMIDI(int numTicks, int samplesPerTick, float* lbuffer, float* rbuffer)
{
	fluid_synth_write_float(FluidSynth, numTicks * samplesPerTick, (void*)lbuffer, 0, 1, (void*)rbuffer, 0, 1);
}

//I_MidiEvent(chunk->events[chunk->nextEvent]);
void I_MidiEvent(midievent_t *ev)
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