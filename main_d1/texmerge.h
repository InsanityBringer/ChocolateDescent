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
/*
 * $Source: f:/miner/source/main/rcs/texmerge.h $
 * $Revision: 2.0 $
 * $Author: john $
 * $Date: 1995/02/27 11:28:05 $
 *
 * Definitions for texture merging caching stuff.
 *
 */

#ifndef _TEXMERGE_H
#define _TEXMERGE_H

#include "2d/gr.h"

int texmerge_init(int num_cached_textures);
grs_bitmap* texmerge_get_cached_bitmap(int tmap_bottom, int tmap_top);
void texmerge_close(void);
void texmerge_flush();

void merge_textures_new(int type, grs_bitmap* bottom_bmp, grs_bitmap* top_bmp, ubyte* dest_data);
void merge_textures_super_xparent(int type, grs_bitmap* bottom_bmp, grs_bitmap* top_bmp, ubyte* dest_data);

#endif