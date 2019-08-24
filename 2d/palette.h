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

void gr_palette_set_gamma(int gamma);
int gr_palette_get_gamma();
extern uint8_t gr_palette_faded_out;
void gr_palette_clear();
int gr_palette_fade_out(uint8_t* pal, int nsteps, int allow_keys);
int gr_palette_fade_in(uint8_t* pal, int nsteps, int allow_keys);
void gr_palette_load(uint8_t* pal);
void gr_make_cthru_table(uint8_t* table, uint8_t r, uint8_t g, uint8_t b);
int gr_find_closest_color_current(int r, int g, int b);
void gr_palette_read(uint8_t* palette);
