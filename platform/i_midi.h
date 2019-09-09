#pragma once

#include "s_midi.h"

int I_InitMIDI();
void I_ShutdownMIDI();
void I_SetSoundfontFilename(const char* filename);
void I_RenderMIDI(int numTicks, int samplesPerTick, unsigned short* buffer);

void I_MidiEvent(midievent_t* ev);