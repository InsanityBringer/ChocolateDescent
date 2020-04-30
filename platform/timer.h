/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#pragma once

#include "misc/types.h"
#include "fix/fix.h"

//Not 100% precise, but should do the job
//Call I_MarkEnd(US_60FPS) as desired
#define US_60FPS 16667
#define US_70FPS 14286

void timer_init();

fix timer_get_fixed_seconds(); // Rolls about every 9 hours...
fix timer_get_approx_seconds(); // Returns time since program started... accurate to 1/120th of a second

//[ISB] Replacement for the annoying ticker, increments 18 times/sec
uint32_t I_GetTicks();

uint32_t I_GetMS();
uint64_t I_GetUS();

//[ISB] replacement for delay?
void I_Delay(int ms);

void I_DelayUS(uint64_t us);

//Quick 'n dirty framerate limiting tools.
//Call I_MarkStart at the beginning of the loop
void I_MarkStart();
//Call I_MarkEnd with the desired delay time to make the thread relax for just long enough
void I_MarkEnd(uint64_t numUS);
