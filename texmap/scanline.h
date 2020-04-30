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

extern void c_tmap_scanline_per();
extern void c_tmap_scanline_per_dither();
extern void c_tmap_scanline_per_nolight();
extern void c_tmap_scanline_pln();
extern void c_tmap_scanline_pln_aa();
extern void c_tmap_scanline_pln_nolight();
extern void c_tmap_scanline_plt();
extern void c_tmap_scanline_plt_nolight();
extern void c_tmap_scanline_lin();
extern void c_tmap_scanline_lin_nolight();
extern void c_tmap_scanline_flat();
extern void c_tmap_scanline_shaded();

extern void c_tmap_scanline_per_trans();
extern void c_tmap_scanline_per_nolight_trans();
extern void c_tmap_scanline_lin_trans();
extern void c_tmap_scanline_lin_nolight_trans();
