/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "s_midi.h"
#include "s_sequencer.h"

MidiSequencer::MidiSequencer(MidiSynth* newSynth, int newSampleRate)
{
	synth = newSynth;
	lastRenderedTick = nextTick = ticks = 0;
	song = nullptr;
	loop = false;
	sampleRate = newSampleRate;
}

MidiSynth* MidiSequencer::GetSynth()
{
	return synth;
}

int MidiSequencer::StartSong(HMPFile* newSong, bool newLoop)
{
	song = newSong;
	loop = newLoop;
	RewindSong(false);

	return 0;
}

void MidiSequencer::RewindSong(bool resetLoop)
{
	int i;
	HMPTrack* track;

	for (i = 0; i < song->NumTracks(); i++)
	{
		track = song->GetTrack(i);
		track->StartSequence();
		synth->SetDefaults();
	}
}

void MidiSequencer::StopSong()
{
	synth->StopSound();
	song = nullptr;
}

void MidiSequencer::Tick()
{
	int i;
	int branchnum;
	HMPTrack* track;
	BranchEntry *branchData;
	midievent_t* ev;
	int loopNum = -1;
	bool eventsRemaining = false;

	if (song == nullptr)
	{
		return;
	}

	for (i = 0; i < song->NumTracks(); i++)
	{
		track = song->GetTrack(i);
		track->AdvancePlayhead(1);
		eventsRemaining |= track->CheckRemainingEvents();

		ev = track->NextEvent();
		while (ev != nullptr)
		{
			//handle special controllers
			if (ev->GetType() == EVENT_CONTROLLER)
			{
				switch (ev->param1)
				{
				case HMI_CONTROLLER_GLOBAL_LOOP_END:
					loopNum = ev->param2 & 127;
					ev = nullptr;
					break;
				case HMI_CONTROLLER_GLOBAL_BRANCH:
					printf("gbranch\n");
					ev = nullptr;
					break;
				case HMI_CONTROLLER_LOCAL_BRANCH:
					branchnum = ev->param2 - 128;
					if (branchnum >= 0)
					{
						branchData = track->GetLocalBranchData(branchnum);
						//track->SetPlayhead(song->GetBranchTick(i, branchnum));
						track->BranchToByteOffset(branchData->offset);
						//printf("lbranch %d %d %d\n", i, branchnum, branchData->offset);

						synth->PerformBranchResets(branchData, ev->GetChannel());

						ev = nullptr;
					}
					break;
				default:
					synth->DoMidiEvent(ev);
					ev = track->NextEvent();
					break;
				}
			}
			else
			{
				synth->DoMidiEvent(ev);
				ev = track->NextEvent();
			}
		}
	}

	//Check loop status
	if (loop)
	{
		//Needed for looping songs without explicit loop points.
		if (eventsRemaining == false)
		{
			RewindSong(false);
		}
		else if (loopNum != -1) //Explicit loop.
		{
			//printf("new loop code running, loop %d\n", loopNum);
			for (i = 0; i < song->NumTracks(); i++)
			{
				track = song->GetTrack(i);
				//track->SetPlayhead(song->GetLoopStart(0));
				if (loopNum+1 <= track->GetNumBranches())
				{
					branchData = track->GetLocalBranchData(loopNum);
					track->BranchToByteOffset(branchData->offset);
					synth->PerformBranchResets(branchData, track->GetChannel());
				}
			}
		}
	}
}

//Returns the next tick that an event has to be performed
void MidiSequencer::Render(int samplesToRender, unsigned short* buffer)
{
	//If there's no song, just render out the requested amount of ticks so that lingering notes fade out even over the fade to black
	synth->RenderMIDI(samplesToRender, buffer);
}
