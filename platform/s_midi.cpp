/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <mutex>

#include "platform/platform_filesys.h"
#include "platform/i_sound.h"
#include "s_midi.h"
#include "s_sequencer.h"
#include "misc/byteswap.h"
#include "misc/error.h"
#include "platform/timer.h"

#ifdef USE_FLUIDSYNTH
#include "platform/fluidsynth/fluid_midi.h"
#endif

#ifdef _WINDOWS
#include "platform/win32/win32midi.h"
#endif

//[ISB] Uncomment to enable MIDI file diagonstics. Extremely slow on windows. And probably linux tbh.
//Will probably overflow your console buffer, so make it really long if you must
//#define DEBUG_MIDI_READING

//Uncomment to enable diagonstics of SOS's special MIDI controllers. 
//#define DEBUG_SPECIAL_CONTROLLERS

GenDevices PreferredGenDevice = GenDevices::FluidSynthDevice;
int CurrentDevice = 0;
int PreferredMMEDevice = -1;

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
char SoundFontFilename[CHOCOLATE_MAX_FILE_PATH_SIZE] = "TestSoundfont.sf2";
#else
char SoundFontFilename[_MAX_PATH] = "TestSoundfont.sf2";
#endif
MidiSynth* synth = nullptr;
MidiSequencer* sequencer = nullptr;
MidiPlayer* player = nullptr;
std::thread* midiThread = nullptr;

#ifdef DEBUG_MIDI_READING
char* EventNames[] = { "Unknown 0", "Unknown 1", "Unknown 2", "Unknown 3", "Unknown 4", "Unknown 5",
"Unknown 6", "Unknown 7", "Note Off", "Note On", "Aftertouch", "Controller", "Patch", "Pressure",
"Pitch", "Sysex" };

void S_PrintEvent(midievent_t* ev)
{
	if (ev->type == 0xFF || ev->type == EVENT_CONTROLLER)
	{
		if (ev->type != 0xFF)
		{
			fprintf(stderr, "  Event Type: %s\n  Affected channel: %d\n", EventNames[ev->type], ev->channel);
		}
		else
		{
			fprintf(stderr, "  Event Type: Meta\n");
		}
		fprintf(stderr, "  Delta: %d\n", ev->delta);

		switch (ev->type)
		{
		case EVENT_NOTEOFF:
		case EVENT_NOTEON:
		case EVENT_AFTERTOUCH:
			fprintf(stderr, "   Note: %d\n   Velocity: %d\n", ev->param1, ev->param2);
			break;
		case EVENT_CONTROLLER:
			fprintf(stderr, "   Controller ID: %d\n   Controller value: %d\n", ev->param1, ev->param2);
			break;
		case EVENT_PATCH:
			fprintf(stderr, "   Patch: %d\n", ev->param1);
			break;
		case EVENT_PITCH:
			fprintf(stderr, "   Pitch: %d\n", ev->param1 + (ev->param2 << 8));
			break;
		case 0xff:
			fprintf(stderr, "   Type: %d\n   Length: %d\n", ev->param1, ev->dataLength);
			break;
		}
	}
}

#endif

int S_InitMusic(int device)
{
	if (CurrentDevice != 0) return 0; //already initalized
	//[ISB] TODO: I really need to add a switcher to allow the use of multiple synths. Agh
	//I_InitMIDI();
	//MidiSynth* synth = nullptr;
	switch (device)
	{
	case _MIDI_GEN:
	{
		GenDevices genMidiDevice = PreferredGenDevice;

		//Validate that the preferred device is actually available. 
		//Messy macro abuse ahoy. 
#ifndef USE_FLUIDSYNTH
		if (genMidiDevice == GenDevices::FluidSynthDevice)
		{
#ifdef _WINDOWS
			genMidiDevice = GenDevices::MMEDevice;
#else
			genMidiDevice = GenDevices::NullDevice;
#endif
		}
#endif

#ifndef _WINDOWS
		if (genMidiDevice == GenDevices::MMEDevice)
		{
#ifdef USE_FLUIDSYNTH
			genMidiDevice = GenDevices::FluidSynthDevice;
#else
			genMidiDevice = GenDevices::NullDevice;
#endif
		}
#endif

		switch (genMidiDevice)
		{
#ifdef USE_FLUIDSYNTH
		case GenDevices::FluidSynthDevice:
		{
			MidiFluidSynth* fluidSynth = new MidiFluidSynth();
			if (fluidSynth == nullptr)
			{
				Error("S_InitMusic: Fatal: Cannot allocate fluid synth.");
				return 1;
			}
			fluidSynth->SetSampleRate(plat_get_preferred_midi_sample_rate());
			fluidSynth->CreateSynth();
			fluidSynth->SetSoundfont(SoundFontFilename);
			synth = (MidiSynth*)fluidSynth;
		}
			break;
#endif
#ifdef _WINDOWS
		case GenDevices::MMEDevice:
		{
			MidiWin32Synth* winsynth = new MidiWin32Synth();
			if (winsynth == nullptr)
			{
				Error("S_InitMusic: Fatal: Cannot allocate win32 synth.");
				return 1;
			}
			winsynth->CreateSynth();
			synth = (MidiSynth*)winsynth;
		}
			break;
#endif
		default:
			synth = new DummyMidiSynth();
			break;
		}
	}
		break;
	}
	if (synth == nullptr)
	{
		Warning("S_InitMusic: Unknown device.\n");
		return 1;
	}
	sequencer = new MidiSequencer(synth, plat_get_preferred_midi_sample_rate());

	if (sequencer == nullptr)
	{
		Error("S_InitMusic: Fatal: Cannot allocate sequencer.");
		return 1;
	}

	//With a synth and sequencer, start the player
	player = new MidiPlayer(sequencer, synth);
	player->Start();

	//Start the platform-dependent MIDI system
	if (plat_start_midi(sequencer))
	{
		Warning("S_InitMusic: plat_start_midi failed.");
		return 1;
	}

	CurrentDevice = device;

	return 0;
}

void S_ShutdownMusic()
{
	if (CurrentDevice == 0) return;
	plat_close_midi();

	if (player != nullptr)
	{
		player->Shutdown();
	}
	if (synth)
	{
		synth->Shutdown();
		delete synth;
	}
	if (sequencer)
	{
		delete sequencer;
	}
	CurrentDevice = 0;
}

void music_set_volume(int volume)
{
	plat_set_music_volume(volume); //Still needed for HQ music
	if (synth)
		synth->SetVolume(volume);
}

#ifndef _WIN32
std::vector<std::string> music_get_MME_devices()
{
	std::vector<std::string> options;
	return options;
}
#endif

int S_ReadDelta(int* pointer, uint8_t* data)
{
	int done = 0, position = 0, value = 0;
	uint8_t b;
	while (!done)
	{
		b = data[*pointer];
		if (b & 0x80) //We're done now
		{
			done = 1;
			b &= 0x7F;
		}
		value += (b << (position * 7));
		position++;
		*pointer = *pointer + 1;
	}
	return value;
}

int S_ReadMIDIDelta(int* pointer, uint8_t* data)
{
	int done = 0, position = 0, value = 0;
	uint8_t b;
	while (!done)
	{
		b = data[*pointer];
		if ((b & 0x80) == 0) //We're done now
		{
			done = 1;
		}
		b &= 0x7F;
		value += (b << (position * 7));
		position++;
		*pointer = *pointer + 1;
	}
	return value;
}

uint16_t S_StartSong(int length, uint8_t* data, bool loop, uint32_t* handle)
{
	if (CurrentDevice == 0) return 1;
	HMPFile* song = new HMPFile(length, data);
	*handle = 0; //heh
	plat_start_midi_song(song, loop);
	if (player)
		player->SetSong(song, loop);
	return 0;
}

uint16_t S_StopSong()
{
	if (CurrentDevice == 0) return 1;
	
	plat_stop_midi_song();
	if (player)
		player->StopSong();
	return 0;
}

HMPTrack::HMPTrack(int chunknum, int channum)
{
	chunkNum = chunknum;
	chunkChannel = channum;

	//Set basic sequencing data
	nextEvent = 0;
	nextEventTime = 0;

	//Create empty event list.
	events = std::vector<midievent_t>();

	//Needed to count local branches.
	numBranches = 0;
}

void HMPTrack::StartSequence()
{
	playhead = 0;
	if (events.size() > 0)
	{
		nextEvent = 0;
		nextEventTime = events[0].delta;
	}
	else
		nextEvent = -1;
}

void HMPTrack::Rescale(int oldTempo, int newTempo)
{
	int i;

	for (i = 0; i < events.size(); i++)
	{
		events[i].delta *= newTempo / oldTempo;
	}

	//Rescale this too, in case it isn't 0;
	nextEventTime *= newTempo / oldTempo;
}

void HMPTrack::SetPlayhead(uint32_t ticks)
{
	int diff, cumlTime = 0;
	playhead = ticks;

	//Return to the first event and advance through events until the first > playhead is found
	if (events.size() > 0)
	{
		nextEvent = 0;
		nextEventTime = events[0].delta;

		while (nextEventTime < ticks)
		{
			cumlTime += events[nextEvent].delta;
			nextEventTime += events[nextEvent].delta;
			nextEvent++; 
			if (nextEvent >= events.size())
			{
				nextEvent = -1;
				break;
			}
		}

		/*if (nextEvent != -1)
		{
			if (cumlTime > ticks)
			{
				diff = cumlTime - ticks;
				//if (diff < 0)
				//	fprintf(stderr, "HMPTrack::SetPlayhead: negative delta %d adjusting next delta. nextEventTime is %d, setting to %d\n", diff, nextEventTime, ticks);

				//printf("Adjusting delta by %d\n", diff);
				nextEventTime -= diff;
			}
		}*/
	}
	else
		nextEvent = -1;
}

void HMPTrack::BranchToByteOffset(uint32_t offset)
{
	//assumption: called in a track that actually has events
	playhead = 0;
	nextEvent = 0;
	nextEventTime = playhead = events[0].delta;
	midievent_t* ev = &events[0];

	while (ev->startPtr < offset)
	{
		nextEvent++;
		if (nextEvent >= events.size())
			return;

		ev = &events[nextEvent];
		playhead += ev->delta;
		nextEventTime = playhead;
	}
	nextEventTime += ev->delta;
}

void HMPTrack::AdvancePlayhead(uint32_t ticks)
{
	playhead += ticks;
}

midievent_t* HMPTrack::NextEvent()
{
	midievent_t* ev;
	//No more events left at all
	if (nextEvent < 0) return nullptr;
	//Playhead is before the next event start
	if (playhead < nextEventTime) return nullptr;

	ev = &events[nextEvent];
	nextEvent++;
	if (nextEvent >= events.size())
		nextEvent = -1;
	else
		nextEventTime += events[nextEvent].delta;

	return ev;
}

HMPFile::HMPFile(int len, uint8_t* data)
{
	int branchTableOffset, pointer, i, j, k;
	uint8_t count;
	std::vector<uint8_t> branchesPerTrack;
	int subPointer;

	loopStart = 0;
	memcpy(header, data, 32); //Check for version 1 HMP

	branchTableOffset = BS_MakeInt(&data[32]);
	//3 dummy dwords here
	numChunks = BS_MakeInt(&data[48]);
	ticksPerQuarter = BS_MakeInt(&data[52]);
	tempo = BS_MakeInt(&data[56]);
	seconds = BS_MakeInt(&data[60]);

	//Up next would be device mapping information, but this isn't useful for Descent ATM.
	//It might become more vital when emulation of other sound cards is implemented.

	tracks = std::vector<HMPTrack>();

	//Jump forward and read all the chunks.
	pointer = 776; //Pointer is adjusted by the reading function
	for (i = 0; i < numChunks; i++)
	{
		branchTickTable.push_back(std::vector<int>(128));
		pointer = ReadChunk(pointer, data);
	}

	pointer = branchTableOffset;
	for (i = 0; i < numChunks; i++)
	{
		count = data[pointer];
		if (count > 127) return; //check bounds
		branchesPerTrack.push_back(count);
		pointer++;
	}

	for (i = 0; i < numChunks; i++)
	{
		for (j = 0; j < branchesPerTrack[i]; j++)
		{
			BranchEntry branch;
			branch.offset = BS_MakeInt(&data[pointer]); pointer += 4;
			branch.branchID = data[pointer++];
			branch.program = data[pointer++];
			branch.loopCount = data[pointer++];
			branch.controlChangeCount = data[pointer++] / 2; // adjust for byte count in file
			branch.controlChangeOffset = BS_MakeInt(&data[pointer]); pointer += 4;
			pointer += 12;

			if (branch.controlChangeCount < 128)
			{
				subPointer = branch.controlChangeOffset;
				for (k = 0; k < branch.controlChangeCount; k++)
				{
					branch.controlChanges[k].controller = data[subPointer++] & 127;
					branch.controlChanges[k].state = data[subPointer++] & 127;
				}
			}

			tracks[i].AddBranch(branch);
		}
	}
}

int HMPFile::ReadChunk(int ptr, uint8_t* data)
{
	uint8_t b;
	int command, delta, tick;
	int basePointer = ptr;
	int destPointer;
	int chunkNum = BS_MakeInt(&data[ptr]);
	int chunkLen = BS_MakeInt(&data[ptr + 4]);
	int chunkChannel = BS_MakeInt(&data[ptr + 8]);
	int numBranches = 0;
#ifdef DEBUG_SPECIAL_CONTROLLERS
	int i = 0;
#endif

	ptr += 12;

	//Chunk length contains the header fields.
	destPointer = basePointer + chunkLen;

	HMPTrack track = HMPTrack(chunkNum, chunkChannel);

	tick = 0; //Temp hack for loop point

	int startOfData = ptr;

#ifdef DEBUG_MIDI_READING
	fprintf(stderr, "Chunk %d:\n Length: %d\n Track: %d\n Events:\n", chunkNum, chunkLen, chunkTrack);
#endif

	while (ptr < destPointer) 
	{
		midievent_t ev = {};
		ev.startPtr = ptr - startOfData;
		delta = S_ReadDelta(&ptr, data);
		b = data[ptr]; ptr++;
		tick += delta;

		if (b == 0xff)
			command = 0xff;
		else
			command = (b >> 4) & 0xf;

		ev.status = b;
		ev.delta = delta;

		switch (command) //Actually load the params now
		{
		case EVENT_NOTEOFF:
		case EVENT_NOTEON:
		case EVENT_AFTERTOUCH:
		case EVENT_CONTROLLER:
		case EVENT_PITCH:
			ev.param1 = data[ptr++];
			ev.param2 = data[ptr++];

			if (command == EVENT_CONTROLLER)
			{
				switch (ev.param1)
				{
				case HMI_CONTROLLER_GLOBAL_LOOP_START:
				{
					int j;
					if (loopStart == 0) //descent 1 game02.hmp has two loop points. On different tracks, so probably need to match them.
					{
						loopStart = tick;
					}
					branchTickTable[chunkNum][numBranches] = tick; //TODO: TEMP HACK
					numBranches++;
#ifdef DEBUG_SPECIAL_CONTROLLERS
					printf("Loop start (110) on track %d channel %d, event %d with delta %d. Loop start at %d, loop count %d, around byte %d\n", chunkNum, ev.channel, i, delta, loopStart, ev.param2, ev.startPtr);
#endif
				}
				break;

				case HMI_CONTROLLER_LOCAL_BRANCH_POS:
				{
					branchTickTable[chunkNum][numBranches] = tick;
#ifdef DEBUG_SPECIAL_CONTROLLERS
					printf("Local branch pos (108) on track %d channel %d, event %d with delta %d. Branch point %d, at tick %d, around byte %d\n", chunkNum, ev.channel, i, delta, numBranches, tick, ev.startPtr);
#endif
					numBranches++;
				}
					break;

#ifdef DEBUG_SPECIAL_CONTROLLERS
				case HMI_CONTROLLER_GLOBAL_LOOP_END:
					printf("Loop end (111) on track %d channel %d, event %d with delta %d. Specified param %d, around byte %d\n", chunkNum, ev.channel, i, delta, ev.param2, ev.startPtr);
					break;
				case HMI_CONTROLLER_ENABLE_CONTROLLER_RESTORE:
					printf("Controller restore enable (103) on track %d channel %d, event %d with delta %d. Specified param %d\n", chunkNum, ev.channel, i, delta, ev.param2);
					break;
				case HMI_CONTROLLER_DISABLE_CONTROLLER_RESTORE:
					printf("Controller restore disable on (104) track %d channel %d, event %d with delta %d. Specified param %d\n", chunkNum, ev.channel, i, delta, ev.param2);
					break;
				case HMI_CONTROLLER_LOCAL_BRANCH:
					printf("Local branch (109) on track %d channel %d, event %d with delta %d. Specified param %d, around byte %d\n", chunkNum, ev.channel, i, delta, ev.param2, ev.startPtr);
					break;
				case HMI_CONTROLLER_GLOBAL_BRANCH_POS:
					printf("Global branch pos (113) on track %d channel %d, event %d with delta %d. Specified param %d\n", chunkNum, ev.channel, i, delta, ev.param2);
					break;
				case HMI_CONTROLLER_GLOBAL_BRANCH:
					printf("Global branch (114) on track %d channel %d, event %d with delta %d. Specified param %d\n", chunkNum, ev.channel, i, delta, ev.param2);
					break;
				case HMI_CONTROLLER_LOCAL_LOOP_START:
					printf("Local loop pos (116) on track %d channel %d, event %d with delta %d. Specified param %d\n", chunkNum, ev.channel, i, delta, ev.param2);
					break;
				case HMI_CONTROLLER_CALL_TRIGGER:
					printf("Call trigger (but why) on track %d channel %d, event %d with delta %d. Specified param %d\n", chunkNum, ev.channel, i, delta, ev.param2);
					break;
				case HMI_CONTROLLER_LOCK_CHAN:
					printf("Channel lock (106) on track %d channel %d, event %d with delta %d. Specified param %d\n", chunkNum, ev.channel, i, delta, ev.param2);
					break;
				case HMI_CONTROLLER_CHAN_PRIORITY:
					printf("Channel priority (107) on track %d channel %d, event %d with delta %d. Specified param %d\n", chunkNum, ev.channel, i, delta, ev.param2);
					break;
#endif
				}
			}
			break;
		case EVENT_PATCH:
		case EVENT_PRESSURE:
			ev.param1 = data[ptr++];
			break;
		case 0xff:
			ev.param1 = data[ptr++];
			ev.dataLength = S_ReadMIDIDelta(&ptr, data);
			if (ev.dataLength != 0)
			{
				//TODO: Determine if any meta events whatsoever are important for us. 
				/*chunk->events[i].data = (uint8_t*)malloc(chunk->events[i].dataLength * sizeof(uint8_t));*/
				/*memcpy(chunk->events[i].data, &data[pointer], chunk->events[i].dataLength);*/
				ptr += ev.dataLength;
			}
			break;
		default:
			fprintf(stderr, "Unknown event %d in hmp file", command);
			break;
		}

#ifdef DEBUG_MIDI_READING
		S_PrintEvent(&ev);
#endif
		//TODO: Events are passed by value rather than by reference or via a pointer, so they get copied
		//This needs to be changed if dynamic allocation is used for meta events
		track.AddEvent(ev);

#ifdef DEBUG_SPECIAL_CONTROLLERS
		i++;
#endif
	}
	track.SetBranchCount(numBranches);
	numBranches = 0;
	tracks.push_back(track);
	return ptr;
}

void HMPFile::Rescale(int newTempo)
{
	int i;

	for (i = 0; i < tracks.size(); i++)
	{
		tracks[i].Rescale(tempo, newTempo);
	}

	tempo = newTempo;
}

//-----------------------------------------------------------------------------
// MIDI Player 2.0
//
// Once again a global part of the MIDI system, not dependent on the backend.
// This will service both the OpenAL backend and dpJudas's mixer through a
// similar interface.
//
// This will allow the execution of a windows hard synth through the same
// interface as soft synth songs. 
//-----------------------------------------------------------------------------

//Amount of ticks to render before attempting to queue again. 
//Was running into problems with OpenAL backend occasionally starving
#define NUMSOFTTICKS 4

MidiPlayer::MidiPlayer(MidiSequencer* newSequencer, MidiSynth* newSynth)
{
	sequencer = newSequencer;
	synth = newSynth;
	nextSong = curSong = nullptr;
	nextLoop = false;
	nextTimerTick = 0;
	numSubTicks = 0;
	TickFracDelta = 0;
	currentTickFrac = 0;
	shouldEnd = false;
	shouldStop = false;
	initialized = false;
	songLoaded = false;

	hasEnded = false;
	hasChangedSong = false;
	midiThread = nullptr;

	songBuffer = new uint16_t[(MIDI_SAMPLERATE / 120) * 4 * NUMSOFTTICKS];
	memset(songBuffer, 0, sizeof(uint16_t) * (MIDI_SAMPLERATE / 120) * 4 * NUMSOFTTICKS);
}

bool MidiPlayer::IsError()
{
	return songBuffer == nullptr;
}

void MidiPlayer::Start()
{
	//Kick off the MIDI thread
	midiThread = new std::thread(&MidiPlayer::Run, this);
}

void MidiPlayer::SetSong(HMPFile* newSong, bool loop)
{
	std::unique_lock<std::mutex> lock(songMutex);
	if (!initialized) return; //already ded
	nextSong = newSong;
	nextLoop = loop;
	lock.unlock();
	//[ISB] I don't like this, but I can't think of a better way to do it atm...
	//This is very contestable, but maybe we can get away with it by merit of this only happening on main thread?
	while (!hasChangedSong);
	hasChangedSong = false;

	//[ISB] This should be on the main thread, so lets just check for an error now and die
	if (IsError())
	{
		Shutdown();
		Error("MidiPlayer::SetSong: Couldn't allocate buffer for music playback\n");
	}
}

void MidiPlayer::StopSong()
{
	std::unique_lock<std::mutex> lock(songMutex);
	if (!initialized) return; //already ded
	shouldStop = true;
	lock.unlock();
	//[ISB] I need to learn how to write threaded programs tbh
	//Avoid race condition by waiting for the MIDI thread to get the message
	while (!hasChangedSong);
	hasChangedSong = false;
}

void MidiPlayer::Shutdown()
{
	if (midiThread)
	{
		std::unique_lock<std::mutex> lock(songMutex);
		if (!initialized) return; //already ded
		shouldEnd = true;
		lock.unlock();
		while (!hasEnded);
		hasEnded = false;
		midiThread->join();
		delete midiThread;
	}
	sequencer->StopSong();
}

void MidiPlayer::Run()
{
	uint64_t currentTime, delta;
	initialized = true;
	nextTimerTick = I_GetUS();
	midi_start_source();
	int i;

	for (;;)
	{
		//printf("I'm goin\n");
		std::unique_lock<std::mutex> lock(songMutex);
		if (shouldEnd)
		{
			initialized = false;
			break;
		}
		else if (shouldStop)
		{
			if (curSong)
			{
				sequencer->StopSong();
				delete curSong;
			}
			shouldStop = false;
			curSong = nullptr;
			hasChangedSong = true;
		}
		else if (nextSong)
		{
			if (curSong)
			{
				sequencer->StopSong();
				delete curSong;
			}
			//printf("Starting new song\n");
			sequencer->StartSong(nextSong, nextLoop);
			midi_set_music_samplerate(MIDI_SAMPLERATE);
			TickFracDelta = (65536 * nextSong->GetTempo()) / 120;
			curSong = nextSong;
			nextSong = nullptr;
			hasChangedSong = true;
			songLoaded = true;
			nextTimerTick = I_GetUS() + 1;
		}
		lock.unlock();

		//There's an occasional bug where FluidSynth crashes if it's called right after creation, so only start doing anything
		//when a song gets loaded and some events exist. 
		if (!songLoaded)
		{
			I_DelayUS(4000);
		}
		else
		{
			//Soft synth operation
			if (synth->ClassifySynth() == MIDISYNTH_SOFT)
			{
				//Ugh. This is hideous.
				//Queue buffers as fast as possible. When done, sleep for a while. This comes close enough to avoiding starvation.
				//Anything less than 5 120hz ticks of latency will result in OpenAL occasionally starving. It's the most I can do...
				midi_dequeue_midi_buffers();
				while (midi_queue_slots_available())
				{
					for (i = 0; i < NUMSOFTTICKS; i++)
					{
						currentTickFrac += TickFracDelta;
						while (currentTickFrac >= 65536)
						{
							sequencer->Tick();
							currentTickFrac -= 65536;
						}
						sequencer->Render(MIDI_SAMPLERATE / 120, songBuffer + (MIDI_SAMPLERATE / 120 * 2) * i);
					}
					midi_queue_buffer(MIDI_SAMPLERATE / 120 * NUMSOFTTICKS, songBuffer);
				}

				midi_check_status();
				I_DelayUS(4000);
			}
			else if (synth->ClassifySynth() == MIDISYNTH_LIVE)
			{
				currentTime = I_GetUS();
				while (currentTime > nextTimerTick)
				{
					currentTickFrac += TickFracDelta;
					while (currentTickFrac >= 65536)
					{
						sequencer->Tick();
						currentTickFrac -= 65536;
					}

					nextTimerTick += (1000000 / 120);

					delta = nextTimerTick - I_GetUS();
					if (delta > 2000)
					{
						I_DelayUS(delta - 2000);
					}
				}
			}
		}
	}
	midi_stop_source();
	shouldEnd = false;
	hasEnded = true;
	//printf("Midi thread rip\n");
}
