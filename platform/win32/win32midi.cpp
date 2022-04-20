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
	UINT device = MIDI_MAPPER;
	UINT max_device = midiOutGetNumDevs();
	if (PreferredMMEDevice >= 0)
	{
		mprintf((0, "Using explicit MME device %d\n", PreferredMMEDevice));
		if (PreferredMMEDevice < max_device)
			device = PreferredMMEDevice;
	}
		
	MMRESULT res = midiOutOpen(&OutputDevice, device, NULL, NULL, CALLBACK_NULL);

	if (res != MMSYSERR_NOERROR)
	{
		mprintf((1, "Failed to create Windows MIDI output device with error code %d\n", res));
	}
}

void MidiWin32Synth::DoMidiEvent(midievent_t* ev)
{
	midievent_t heh = {};
	uint32_t msg;
	if (!OutputDevice) return;

	int channel = ev->GetChannel();
	switch (ev->GetType())
	{
	case EVENT_NOTEON:
	case EVENT_NOTEOFF:
	case EVENT_AFTERTOUCH:
	case EVENT_PRESSURE:
	case EVENT_PITCH:
	case EVENT_PATCH:
	case EVENT_CONTROLLER:
		//Catch volume events, since they need to be caught for volume scaling
		if (ev->GetType() == EVENT_CONTROLLER && ev->param1 == 7)
		{
			VolumeLevels[ev->GetChannel()] = ev->param2;
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
		break;
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
	//TODO: This does enough work it should probably be implemented as a long buffer.
	//The long buffer will always be the same so it can be baked. 
	ev.status = 0xFF;
	DoMidiEvent(&ev);

	for (int chan = 0; chan < 16; chan++)
	{
		ev.status = (EVENT_CONTROLLER << 4) | chan;
		ev.param2 = 0;
		//Send controller reset
		ev.param1 = 121;
		DoMidiEvent(&ev);
		//Send notes off
		ev.param1 = 123;
		DoMidiEvent(&ev);
		//Send volume control
		ev.param1 = 7;
		DoMidiEvent(&ev);
		//Send fine volume control
		ev.param1 = 39;
		DoMidiEvent(&ev);
		//Send bank select
		ev.param1 = 0;
		DoMidiEvent(&ev);

		//Send default program
		ev.status = (EVENT_PATCH << 4) | chan;
		ev.param1 = 0;
		DoMidiEvent(&ev);

		ev.status = (EVENT_PITCH << 4) | chan;
		ev.param1 = 0; ev.param2 = 0x40;
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

	if (!OutputDevice) return;

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

std::vector<std::string> music_get_MME_devices()
{
	UINT numdevs = midiOutGetNumDevs();
	MIDIOUTCAPS caps = {};
	std::vector<std::string> options;

	for (UINT i = 0; i < numdevs; i++)
	{
		midiOutGetDevCaps(i, &caps, sizeof(caps));

		options.push_back(std::string(caps.szPname, strlen(caps.szPname)));
	}

	return options;
}
