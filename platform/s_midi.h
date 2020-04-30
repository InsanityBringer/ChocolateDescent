/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt
*/
#pragma once

#include <thread>
#include <mutex>
#include <stdint.h>

#define EVENT_NOTEOFF 8
#define EVENT_NOTEON 9
#define EVENT_AFTERTOUCH 10
#define EVENT_CONTROLLER 11
#define EVENT_PATCH 12
#define EVENT_PRESSURE 13
#define EVENT_PITCH 14
#define EVENT_SYSEX 15

#define _MIDI_GEN 0xA001
#define _MIDI_FM 0xA002
#define _MIDI_AWE32 0xA008 //[ISB] no idea what this would entail...
#define _MIDI_OPL3 0xA009
#define _MIDI_GUS 0xA00A

#define MIDI_SAMPLERATE 44100

extern char SoundFontFilename[];

typedef struct
{
	uint64_t delta;

	uint8_t channel;
	uint8_t type;
	uint8_t param1;
	uint8_t param2;

	int dataLength;
	uint8_t* data;
} midievent_t;

typedef struct
{
	int chunkNum;
	int chunkLen;
	int chunkTrack;

	int numEvents;
	midievent_t* events;

	//Track sequencing data
	int nextEvent; //Next event to play, -1 if this track is finished
	uint64_t nextEventTime; //At what MIDI tick should this event be played? In samples
} midichunk_t;

typedef struct
{
	char header[16];
	int length;
	int numChunks;
	int bpm;
	int seconds;

	//Go straight to this offset when looping the song
	int loopStart;

	midichunk_t* chunks;
} hmpheader_t;

int S_InitMusic(int device);
void S_ShutdownMusic();

uint16_t S_StartSong(int length, uint8_t* data, bool loop, uint32_t* handle);
uint16_t S_StopSong();

int S_LoadHMP(int length, uint8_t* data, hmpheader_t* song);
void S_FreeHMPData(hmpheader_t* song);

//[ISB] MIDI playback rewrite

//MIDI Synth modes. Affects how the attached Sequencer will work
#define MIDISYNTH_SOFT 1 //Synth renders out audio into a PCM wave buffer.
#define MIDISYNTH_LIVE 2 //Synth performs live MIDI events at 120 ticks/sec

class MidiSequencer;

//Class which represents a midi synthesizer. Wraps a Sequencer.
class MidiSynth
{
	MidiSequencer* sequencer;
public:
	virtual int ClassifySynth() = 0;
	virtual void SetSampleRate(uint32_t newSampleRate) = 0;
	virtual void DoMidiEvent(midievent_t* ev) = 0;
	virtual void RenderMIDI(int numTicks, unsigned short* buffer) = 0;
	virtual void StopSound() = 0;
	virtual void Shutdown() = 0;
};

class DummyMidiSynth : public MidiSynth
{
public:
	int ClassifySynth() override { return MIDISYNTH_SOFT; }
	void SetSampleRate(uint32_t newSampleRate) override { }
	void DoMidiEvent(midievent_t* ev) override { }
	void RenderMIDI(int numTicks, unsigned short* buffer) override { }
	void StopSound() override { }
	void Shutdown() override { }
};

//Class which represents the midi thread. Has a Sequencer and a Synthesizer, and invokes the current audio backend to run
class MidiPlayer
{
	//The digi code can call shutdown before an init, so uh...
	bool initialized; 
	MidiSequencer* sequencer;
	MidiSynth* synth;

	bool shouldStop;
	hmpheader_t* nextSong;
	hmpheader_t* curSong;
	bool nextLoop;
	std::thread *midiThread;
	std::mutex songMutex;

	uint16_t* songBuffer;

	uint64_t nextTimerTick;
	bool shouldEnd;

	//Status flags, these indicate when an event has happened
	//Hardly a shining example of how to do communication
	//between threads, but it works. Volatile to avoid register
	//caching. Thanks TheZombieKiller for pointing this out
	volatile bool hasEnded;
	volatile bool hasChangedSong;
public:
	MidiPlayer(MidiSequencer *newSequencer, MidiSynth *newSynth);
	void SetSong(hmpheader_t* song, bool loop);
	void Start();
	void StopSong();
	void Run();
	void Shutdown();
	//dumb things, pls improve
	bool IsError();
};