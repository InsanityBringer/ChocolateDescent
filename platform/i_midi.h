#pragma once

#include "s_midi.h"

int I_InitMIDI();
void I_ShutdownMIDI();
void I_SetSoundfontFilename(const char* filename);
void I_RenderMIDI(int numTicks, int samplesPerTick, float* lbuffer, float* rbuffer);

void I_MidiEvent(midievent_t* ev);