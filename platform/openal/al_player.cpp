/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#ifdef USE_OPENAL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <mutex>

#include "platform/i_sound.h"
#include "platform/i_midi.h"
#include "platform/s_midi.h"
#include "platform/s_sequencer.h"
#include "misc/byteswap.h"
#include "misc/error.h"
#include "platform/timer.h"

MidiPlayer::MidiPlayer(MidiSequencer* newSequencer, MidiSynth* newSynth)
{
	sequencer = newSequencer;
	synth = newSynth;
	nextSong = curSong = nullptr;
	nextLoop = false;
	nextTimerTick = 0;
	shouldEnd = false;
	shouldStop = false;
	initialized = false;

	hasEnded = false;
	hasChangedSong = false;
	midiThread = nullptr;

	songBuffer = new uint16_t[6 * (MIDI_SAMPLERATE / 120) * 2];
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

void MidiPlayer::SetSong(hmpheader_t* newSong, bool loop)
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
	initialized = true;
	nextTimerTick = I_GetUS();
	AL_StartMIDISong();

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
				S_FreeHMPData(curSong);
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
				S_FreeHMPData(curSong);
			}
			//printf("Starting new song\n");
			sequencer->StartSong(nextSong, nextLoop);
			if (songBuffer)
			{
				delete[] songBuffer;
			}
			songBuffer = new uint16_t[6 * (44100 / nextSong->bpm) * 2];
			//I_StartMIDISong(nextSong, nextLoop);
			curSong = nextSong;
			nextSong = nullptr;
			hasChangedSong = true;
		}
		lock.unlock();

		//Soft synth operation
		if (synth->ClassifySynth() == MIDISYNTH_SOFT)
		{
			AL_DequeueMusicBuffers();
			if (AL_CanQueueMusicBuffer())
			{
				if (curSong != nullptr)
				{
					int ticks = sequencer->Render(5 * (MIDI_SAMPLERATE / curSong->bpm), songBuffer);
					AL_QueueMusicBuffer(ticks, songBuffer);
				}
				else //[ISB] current design of the stupid midi playback code can't handle being starved... I need to junk it and rewrite. 
				{
					int ticks = sequencer->Render(5 * (MIDI_SAMPLERATE / 120), songBuffer);
					AL_QueueMusicBuffer(ticks, songBuffer);
				}
			}
		}

		uint64_t startTick = I_GetUS();
		uint64_t numTicks = nextTimerTick - startTick;
		if (numTicks > 2000) //[ISB] again inspired by dpJudas, with 2000 US number from GZDoom
			I_DelayUS(numTicks - 2000);
		while (I_GetUS() < nextTimerTick);
		if (curSong == nullptr) //no song, run at 120hz
			nextTimerTick += 8333;
		else
			nextTimerTick += 1000000 / curSong->bpm;
	}
	AL_StopMIDISong();
	shouldEnd = false;
	hasEnded = true;
	//printf("Midi thread rip\n");
}
#endif