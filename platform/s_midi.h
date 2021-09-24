/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt
*/
#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <stdint.h>

#define EVENT_NOTEOFF 8
#define EVENT_NOTEON 9
#define EVENT_AFTERTOUCH 10
#define EVENT_CONTROLLER 11
#define EVENT_PATCH 12
#define EVENT_PRESSURE 13
#define EVENT_PITCH 14
#define EVENT_SYSEX 15

//HMI special events determined from docs found on archive.org
//Note that these are from 4.0 and not all may apply to Descent
//This is in need of research

//Sets whether controllers are reset on a branch
#define HMI_CONTROLLER_ENABLE_CONTROLLER_RESTORE 103
#define HMI_CONTROLLER_DISABLE_CONTROLLER_RESTORE 104

//Adjust priority
#define HMI_CONTROLLER_LOCK_CHAN 106
#define HMI_CONTROLLER_CHAN_PRIORITY 107

//Local (per track) branch
#define HMI_CONTROLLER_LOCAL_BRANCH_POS 108
#define HMI_CONTROLLER_LOCAL_BRANCH 109

//Global loop
#define HMI_CONTROLLER_GLOBAL_LOOP_START 110
#define HMI_CONTROLLER_GLOBAL_LOOP_END 111

//Global branch
#define HMI_CONTROLLER_GLOBAL_BRANCH_POS 113
#define HMI_CONTROLLER_GLOBAL_BRANCH 114

//Local loop
#define HMI_CONTROLLER_LOCAL_LOOP_START 116
#define HMI_CONTROLLER_LOCAL_LOOP_END 117

//Call trigger, unused by descent
#define HMI_CONTROLLER_CALL_TRIGGER 119

#define _MIDI_GEN 0xA001
#define _MIDI_FM 0xA002
#define _MIDI_AWE32 0xA008 //[ISB] no idea what this would entail...
#define _MIDI_OPL3 0xA009
#define _MIDI_GUS 0xA00A

#define MIDI_SAMPLERATE 48000 //changed from 44100 since 44100/120 = 367.5 which causes timing anomalies for MIDI

extern char SoundFontFilename[];

typedef struct
{
	uint32_t delta;

	uint8_t channel;
	uint8_t type;
	uint8_t param1;
	uint8_t param2;

	int dataLength;
	uint8_t* data;

	//Used for branches. Ugh. 
	uint32_t startPtr;  
} midievent_t;

struct BranchControlChange
{
	uint8_t controller, state;
};

struct BranchEntry
{
	int offset;
	uint8_t branchID;
	uint8_t program;
	uint8_t loopCount;
	uint8_t controlChangeCount;
	int controlChangeOffset;
	BranchControlChange controlChanges[128];
};

class HMPTrack
{
	int chunkNum;
	int chunkChannel;

	//Track sequencing data
	//TODO: Tracks need their own playhead
	int nextEvent; //Next event to play, -1 if this track is finished
	uint32_t nextEventTime; //At what MIDI tick should this event be played? This value's unit is what the particular sequencer wants.
	uint32_t playhead; //Tick of the current playhead. Events can be obtained with NextEvent, so long as playhead <= nextEventTime.

	//TODO: Convert midievent_t into class
	std::vector<midievent_t> events;
	std::vector<BranchEntry> branches;

	int numBranches;

public:
	HMPTrack(int chunkn, int n);

	void AddEvent(midievent_t ev)
	{
		events.push_back(ev);
	}

	//Rescales this track to be in terms of a new tempo.
	void Rescale(int oldTempo, int newTempo);
	//Resets the playhead to the beginning of the track
	void StartSequence();
	//Forces the playhead to a specific tick value. Mainly for looping, can be used to implement HMI branches too.
	void SetPlayhead(uint32_t ticks);
	//Advances the playhead by the amount of ticks specified. Use NextEvent to get the new events available.
	void AdvancePlayhead(uint32_t ticks);
	//Gets the next sequenced MIDI event, if it is behind or at the playhead. Returns nullptr if there are no more available events,
	//or all events are past the playhead.
	midievent_t* NextEvent();
	//Branches to the specified byte offset in the MIDI stream.
	void BranchToByteOffset(uint32_t offset);

	bool CheckRemainingEvents()
	{
		return nextEvent != -1;
	}

	void AddBranch(BranchEntry& branch)
	{
		branches.push_back(branch);
	}

	void SetBranchCount(int branches)
	{
		numBranches = branches;
	}

	BranchEntry* GetLocalBranchData(int num)
	{
		return &branches[num];
	}

	int GetNumBranches()
	{
		return branches.size();
	}

	int GetChannel()
	{
		return chunkChannel;
	}
};

class HMPFile
{
	char header[32]; //Should be "HMIMIDIP", null padded to 32 bytes.
	int numChunks; //Amount of chunks. Actually tracks.
	int ticksPerQuarter; //Ticks per quarter note. Should always be 60, other values cause problems in Descent, according to Parabolicus. 
	int tempo; //formerly bpm. Ticks per second.
	int seconds; //Length of the song in seconds. I don't think this has a technical use, but useful for debugging.

	std::vector<HMPTrack> tracks;

	//TODO: This needs to be per track.
	uint32_t loopStart;

	uint8_t controllerResetTable[128]; //Table of controllers that should be reset on a loop or branch. Nonzero if they should be reset.
	std::vector<std::vector<int>> branchTickTable;

	//private member for reading events from a chunk. Returns the new pointer.
	int ReadChunk(int ptr, uint8_t* data);

public:
	HMPFile(int len, uint8_t* data);

	int GetTempo() const
	{
		return tempo;
	}

	//Rescale is used to adjust the tempo of the song. 
	//This is needed because the soft synth playback works in terms of samples, not ticks
	void Rescale(int newTempo);

	//Get a global loop start for this song, 
	uint32_t GetLoopStart(int level) //level is currently unused, for a future rewrite that supports multiple loops
	{
		return loopStart;
	}

	int NumTracks() const
	{
		return tracks.size();
	}

	HMPTrack* GetTrack(int num)
	{
		return &tracks[num];
	}

	//TODO: Move to Track
	int GetBranchTick(int track, int branchnum)
	{
		return branchTickTable[track][branchnum];
	}
};

int S_InitMusic(int device);
void S_ShutdownMusic();

uint16_t S_StartSong(int length, uint8_t* data, bool loop, uint32_t* handle);
uint16_t S_StopSong();

//MIDI Synth modes. Affects how the attached Sequencer will work
#define MIDISYNTH_SOFT 1 //Synth renders out audio into a PCM wave buffer.
#define MIDISYNTH_LIVE 2 //Synth performs live MIDI events at 120 ticks/sec

class MidiSequencer;

//Class which represents a midi synthesizer. Wraps a Sequencer.
class MidiSynth
{
public:
	//Synth classification. Returns MIDISYNTH_SOFT if using a softsynth, MIDISYNTH_LIVE if using a hardware synth
	virtual int ClassifySynth() = 0;
	//Changes the sample rate for soft synths.
	virtual void SetSampleRate(uint32_t newSampleRate) = 0;
	//Executes a midi event.
	virtual void DoMidiEvent(midievent_t* ev) = 0;
	//Renders a softsynth's output into a buffer. For stereo sounds samples are interleaved.
	virtual void RenderMIDI(int numTicks, unsigned short* buffer) = 0;
	//Stops all sounds from the synthesizer.
	virtual void StopSound() = 0;
	//Closes the midi synth.
	virtual void Shutdown() = 0;
	//Resets the default state for the synth, because HMI SOS has unusual specifications.
	virtual void SetDefaults() = 0;
	virtual void PerformBranchResets(BranchEntry* entry, int chan) = 0;
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
	void SetDefaults() override { }
	void PerformBranchResets(BranchEntry* entry, int chan) override { }
};

//Class which represents the midi thread. Has a Sequencer and a Synthesizer, and invokes the current audio backend to run
class MidiPlayer
{
	//The digi code can call shutdown before an init, so uh...
	bool initialized; 
	bool songLoaded;
	MidiSequencer* sequencer;
	MidiSynth* synth;

	bool shouldStop;
	HMPFile* nextSong;
	HMPFile* curSong;
	bool nextLoop;
	std::thread *midiThread;
	std::mutex songMutex;

	uint16_t* songBuffer;

	uint64_t nextTimerTick;
	bool shouldEnd;

	int numSubTicks; //Queue midi ticks in terms of "sub-ticks", 100 sample slices. 
	int currentTickFrac, TickFracDelta;

	//Status flags, these indicate when an event has happened
	//Hardly a shining example of how to do communication
	//between threads, but it works. Volatile to avoid register
	//caching. Thanks TheZombieKiller for pointing this out
	volatile bool hasEnded;
	volatile bool hasChangedSong;
public:
	MidiPlayer(MidiSequencer *newSequencer, MidiSynth *newSynth);
	void SetSong(HMPFile* song, bool loop);
	void Start();
	void StopSong();
	void Run();
	void Shutdown();
	//dumb things, pls improve
	bool IsError();
};
