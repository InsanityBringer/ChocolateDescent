/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#pragma once

#include "s_midi.h"

int I_InitMIDI();
void I_ShutdownMIDI();
void I_SetSoundfontFilename(const char* filename);
void I_RenderMIDI(int numTicks, int samplesPerTick, unsigned short* buffer);
void I_StopAllNotes();

void I_MidiEvent(midievent_t* ev);