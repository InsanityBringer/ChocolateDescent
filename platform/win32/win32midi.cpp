/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include <stdlib.h>
#include "win32midi.h"
#include "platform/mono.h"

MidiWin32Synth::MidiWin32Synth()
{
	OutputDevice = 0;
	memset(VolumeLevels, 0, sizeof(VolumeLevels));
	MasterVolume = 127;
}

void MidiWin32Synth::SetSoundfont(const char* filename)
{
	//todo: uhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh
	//should be property at higher level perhaps
}

void MidiWin32Synth::SetSampleRate(uint32_t newSampleRate)
{
	//todo: same with me
}

void MidiWin32Synth::CreateSynth()
{
	MMRESULT res = midiOutOpen(&OutputDevice, MIDI_MAPPER, NULL, NULL, CALLBACK_NULL);

	if (res != MMSYSERR_NOERROR)
	{
		mprintf((1, "Failed to create Windows MIDI output device with error code %d\n", res));
	}
}

void MidiWin32Synth::DoMidiEvent(midievent_t* ev)
{
	midievent_t heh = {};
	uint32_t msg;
	if (!OutputDevice || ev->type == 0xff) return; //TODO

	//Catch volume events, since they need to be caught for volume scaling
	if (ev->type == EVENT_CONTROLLER && ev->param1 == 7)
	{
		VolumeLevels[ev->channel] = ev->param2;
		heh.status = ev->status;
		heh.param1 = ev->param1;
		heh.param2 = ev->param2 * MasterVolume / 127;
		msg = heh.EncodeShortMessage();
	}
	else
		msg = ev->EncodeShortMessage();


	MMRESULT res = midiOutShortMsg(OutputDevice, (DWORD)msg);
	if (res != MMSYSERR_NOERROR)
	{
		fprintf(stderr, "Failed to write short event with error code %d\n", res);
		return;
	}
}

void MidiWin32Synth::RenderMIDI(int numTicks, unsigned short* buffer)
{
	//todo API change
}

void MidiWin32Synth::StopSound()
{
	midievent_t ev = {};
	for (int chan = 0; chan < 16; chan++)
	{
		ev.status = (EVENT_CONTROLLER << 4) | chan;

		//Send notes off
		ev.param1 = 123;
		DoMidiEvent(&ev);
	}
}

void MidiWin32Synth::Shutdown()
{
	if (OutputDevice)
	{
		midiOutClose(OutputDevice);
		OutputDevice = 0;
	}
}

void MidiWin32Synth::SetDefaults()
{
	midievent_t ev = {};
	for (int chan = 0; chan < 16; chan++)
	{
		ev.status = (EVENT_CONTROLLER << 4) | chan;

		//Send notes off
		ev.param1 = 123;
		DoMidiEvent(&ev);
		//Send volume control
		ev.param1 = 7;
		DoMidiEvent(&ev);
		//Send fine volume control
		ev.param1 = 39;
		DoMidiEvent(&ev);
	}
}

void MidiWin32Synth::PerformBranchResets(BranchEntry* entry, int chan)
{
	midievent_t ev = {};
	for (int i = 0; i < entry->controlChangeCount; i++)
	{
		ev.status = (EVENT_CONTROLLER << 4) | chan;
		ev.param1 = entry->controlChanges[i].controller;
		ev.param2 = entry->controlChanges[i].state;
		DoMidiEvent(&ev);
	}
}

void MidiWin32Synth::SetVolume(int volume)
{
	MasterVolume = volume;
	midievent_t ev = {};
	for (int chan = 0; chan < 16; chan++)
	{
		ev.status = (EVENT_CONTROLLER << 4) | chan;

		//Send volume control
		ev.param1 = 7;
		ev.param2 = VolumeLevels[chan] * MasterVolume / 127;
		
		//hack: needs to bypass monitoring for volume levels. 
		MMRESULT res = midiOutShortMsg(OutputDevice, (DWORD)ev.EncodeShortMessage());
		if (res != MMSYSERR_NOERROR)
		{
			fprintf(stderr, "Failed to write short event with error code %d\n", res);
			return;
		}
	}
}