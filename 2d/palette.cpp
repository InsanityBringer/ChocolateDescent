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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "misc/types.h"
#include "mem/mem.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "2d/i_gr.h"
#include "cfile/cfile.h"
#include "misc/error.h"
#include "platform/mono.h"
#include "fix/fix.h"
#include "2d/palette.h"
#include "misc/rand.h"

extern int gr_installed;

uint8_t gr_palette[256 * 3];
uint8_t gr_current_pal[256 * 3];
uint8_t gr_fade_table[256 * 34];

uint16_t gr_palette_selector;
uint16_t gr_fade_table_selector;

uint8_t gr_palette_gamma = 0;
int gr_palette_gamma_param = 0;
uint8_t gr_palette_faded_out = 1;

int Num_computed_colors = 0;

int grd_fades_disabled = 0;

void gr_palette_set_gamma(int gamma)
{
	if (gamma < 0) gamma = 0;
	if (gamma > 8) gamma = 8;

	if (gr_palette_gamma_param != gamma) 
	{
		gr_palette_gamma_param = gamma;
		gr_palette_gamma = gamma;
		if (!gr_palette_faded_out)
			gr_palette_load(gr_palette);
	}
}

int gr_palette_get_gamma()
{
	return gr_palette_gamma_param;
}

void gr_copy_palette(uint8_t* gr_palette, uint8_t* pal, int size)
{
	memcpy(gr_palette, pal, size);
	Num_computed_colors = 0;
}

void gr_use_palette_table(const char* filename)
{
	CFILE* fp;
	int i, fsize;

	fp = cfopen(filename, "rb");
	if (fp == NULL)
		Error("Can't open palette file <%s>", filename);

	fsize = cfilelength(fp);
	Assert(fsize == 9472);
	cfread(gr_palette, 256 * 3, 1, fp);
	cfread(gr_fade_table, 256 * 34, 1, fp);
	cfclose(fp);

	// This is the TRANSPARENCY COLOR
	for (i = 0; i < GR_FADE_LEVELS; i++)
	{
		gr_fade_table[i * 256 + 255] = 255;
	}

	Num_computed_colors = 0;	//[ISB] Flush palette cache.
}

#define SQUARE(x) ((x)*(x))

#define	MAX_COMPUTED_COLORS	32

typedef struct {
	uint8_t	r, g, b, color_num;
} color_record;

color_record Computed_colors[MAX_COMPUTED_COLORS];

//	Add a computed color (by gr_find_closest_color) to list of computed colors in Computed_colors.
//	If list wasn't full already, increment Num_computed_colors.
//	If was full, replace a random one.
void add_computed_color(int r, int g, int b, int color_num)
{
	int	add_index;

	if (Num_computed_colors < MAX_COMPUTED_COLORS) 
	{
		add_index = Num_computed_colors;
		Num_computed_colors++;
	}
	else
		add_index = (P_Rand() * MAX_COMPUTED_COLORS) >> 15;

	Computed_colors[add_index].r = r;
	Computed_colors[add_index].g = g;
	Computed_colors[add_index].b = b;
	Computed_colors[add_index].color_num = color_num;
}

void init_computed_colors(void)
{
	int	i;

	for (i = 0; i < MAX_COMPUTED_COLORS; i++)
		Computed_colors[i].r = 255;		//	Make impossible to match.
}

int gr_find_closest_color(int r, int g, int b)
{
	int i, j;
	int best_value, best_index, value;

	if (Num_computed_colors == 0)
		init_computed_colors();

	//	If we've already computed this color, return it!
	for (i = 0; i < Num_computed_colors; i++)
		if (r == Computed_colors[i].r)
			if (g == Computed_colors[i].g)
				if (b == Computed_colors[i].b) 
				{
					if (i > 4)
					{
						color_record	trec;
						trec = Computed_colors[i - 1];
						Computed_colors[i - 1] = Computed_colors[i];
						Computed_colors[i] = trec;
						return Computed_colors[i - 1].color_num;
					}
					return Computed_colors[i].color_num;
				}

	//	r &= 63;
	//	g &= 63;
	//	b &= 63;

	best_value = SQUARE(r - gr_palette[0]) + SQUARE(g - gr_palette[1]) + SQUARE(b - gr_palette[2]);
	best_index = 0;
	if (best_value == 0) 
	{
		add_computed_color(r, g, b, best_index);
		return best_index;
	}
	j = 0;
	// only go to 255, 'cause we dont want to check the transparent color.
	for (i = 1; i < 254; i++) 
	{
		j += 3;
		value = SQUARE(r - gr_palette[j]) + SQUARE(g - gr_palette[j + 1]) + SQUARE(b - gr_palette[j + 2]);
		if (value < best_value) 
		{
			if (value == 0) 
			{
				add_computed_color(r, g, b, i);
				return i;
			}
			best_value = value;
			best_index = i;
		}
	}
	add_computed_color(r, g, b, best_index);
	return best_index;
}

int gr_find_closest_color_15bpp(int rgb)
{
	return gr_find_closest_color(((rgb >> 10) & 31) * 2, ((rgb >> 5) & 31) * 2, (rgb & 31) * 2);
}

int gr_find_closest_color_current(int r, int g, int b)
{
	int i, j;
	int best_value, best_index, value;

	//	r &= 63;
	//	g &= 63;
	//	b &= 63;

	best_value = SQUARE(r - gr_current_pal[0]) + SQUARE(g - gr_current_pal[1]) + SQUARE(b - gr_current_pal[2]);
	best_index = 0;
	if (best_value == 0)
		return best_index;

	j = 0;
	// only go to 255, 'cause we dont want to check the transparent color.
	for (i = 1; i < 254; i++) 
	{
		j += 3;
		value = SQUARE(r - gr_current_pal[j]) + SQUARE(g - gr_current_pal[j + 1]) + SQUARE(b - gr_current_pal[j + 2]);
		if (value < best_value) 
		{
			if (value == 0)
				return i;
			best_value = value;
			best_index = i;
		}
	}
	return best_index;
}


static int last_r = 0, last_g = 0, last_b = 0;

void gr_palette_step_up(int r, int g, int b)
{
	int i;
	uint8_t* p;
	int temp;
	uint8_t temp_pal[768];

	if (gr_palette_faded_out) return;

	if ((r == last_r) && (g == last_g) && (b == last_b)) return;

	last_r = r;
	last_g = g;
	last_b = b;

	p = gr_palette;
	for (i = 0; i < 256; i++) 
	{
		temp = (int)(*p++) + r + gr_palette_gamma;
		if (temp < 0) temp = 0;
		else if (temp > 63) temp = 63;
		temp_pal[i * 3 + 0] = temp;
		temp = (int)(*p++) + g + gr_palette_gamma;
		if (temp < 0) temp = 0;
		else if (temp > 63) temp = 63;
		temp_pal[i * 3 + 1] = temp;
		temp = (int)(*p++) + b + gr_palette_gamma;
		if (temp < 0) temp = 0;
		else if (temp > 63) temp = 63;
		temp_pal[i * 3 + 2] = temp;
	}
	I_WritePalette(0, 255, &temp_pal[0]);
}

void gr_palette_clear()
{
	I_BlankPalette();
	gr_palette_faded_out = 1;
}

void gr_palette_load(uint8_t* pal)
{
	int i;
	uint8_t c;
	for (i = 0; i < 768; i++) 
	{
		c = pal[i] + gr_palette_gamma;
		if (c > 63) c = 63;
		gr_current_pal[i] = c;
	}
	I_WritePalette(0, 255, &gr_current_pal[0]);
	gr_palette_faded_out = 0;

	init_computed_colors();
}

int gr_palette_fade_out(uint8_t* pal, int nsteps, int allow_keys)
{
	uint8_t c;
	int i, j;
	fix fade_palette[768];
	fix fade_palette_delta[768];
	uint8_t fade_palette_raw[768];

//	allow_keys = allow_keys;

	if (gr_palette_faded_out) return 0;

#ifndef NDEBUG
	if (grd_fades_disabled)
	{
		gr_palette_clear();
		return 0;
	}
#endif

	for (i = 0; i < 768; i++) 
	{
		fade_palette[i] = i2f(pal[i] + gr_palette_gamma);
		fade_palette_delta[i] = fade_palette[i] / nsteps;
	}

	for (j = 0; j < nsteps; j++)
	{
		gr_sync_display();
		I_DoEvents();
		for (i = 0; i < 768; i++) 
		{
			fade_palette[i] -= fade_palette_delta[i];
			if (fade_palette[i] < 0)
				fade_palette[i] = 0;
			c = (uint8_t)f2i(fade_palette[i]);
			if (c > 63) c = 63;
			fade_palette_raw[i] = c;
		}
		I_WritePalette(0, 255, &fade_palette_raw[0]);
#ifndef BUILD_DESCENT2
		I_BlitCanvas(grd_curcanv);
#endif
		I_DrawCurrentCanvas(0);
	}
	gr_palette_faded_out = 1;
	return 0;
}

int gr_palette_fade_in(uint8_t* pal, int nsteps, int allow_keys)
{
	int i, j;
	uint8_t c;
	fix fade_palette[768];
	fix fade_palette_delta[768];
	uint8_t fade_palette_raw[768];

//	allow_keys = allow_keys;

	if (!gr_palette_faded_out) return 0;

#ifndef NDEBUG
	if (grd_fades_disabled)
	{
		gr_palette_clear();
		return 0;
	}
#endif

	for (i = 0; i < 768; i++) 
	{
		gr_current_pal[i] = pal[i];
		fade_palette[i] = 0;
		fade_palette_delta[i] = i2f(pal[i] + gr_palette_gamma) / nsteps;
	}

	for (j = 0; j < nsteps; j++) 
	{
		gr_sync_display();
		for (i = 0; i < 768; i++) 
		{
			fade_palette[i] += fade_palette_delta[i];
			if (fade_palette[i] > i2f(pal[i] + gr_palette_gamma))
				fade_palette[i] = i2f(pal[i] + gr_palette_gamma);
			c = (uint8_t)f2i(fade_palette[i]);
			if (c > 63) c = 63;
			fade_palette_raw[i] = c;
		}
		I_WritePalette(0, 255, &fade_palette_raw[0]);
#ifndef BUILD_DESCENT2 //[ISB] why does this cause problems in Descent 2 but not Descent 1?
		I_BlitCanvas(grd_curcanv);
#endif
		I_DrawCurrentCanvas(0);
		I_DoEvents();
	}
	gr_palette_faded_out = 0;
	return 0;
}

void gr_make_cthru_table(uint8_t* table, uint8_t r, uint8_t g, uint8_t b)
{
	int i;
	uint8_t r1, g1, b1;

	for (i = 0; i < 256; i++)
	{
		r1 = gr_palette[i * 3 + 0] + r;
		if (r1 > 63) r1 = 63;
		g1 = gr_palette[i * 3 + 1] + g;
		if (g1 > 63) g1 = 63;
		b1 = gr_palette[i * 3 + 2] + b;
		if (b1 > 63) b1 = 63;
		table[i] = gr_find_closest_color(r1, g1, b1);
	}
}

void gr_palette_read(uint8_t* palette)
{
	I_ReadPalette(palette);
}

