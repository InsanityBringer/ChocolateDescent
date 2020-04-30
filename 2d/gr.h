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

#define SWAP_0_255			0			// swap black and white
#define TRANSPARENCY_COLOR	255			// palette entry of transparency color -- 255 on the PC

#define GR_FADE_LEVELS 34
#define GR_ACTUAL_FADE_LEVELS 32

extern int Gr_scanline_darkening_level;

typedef struct _grs_point 
{
	fix	x, y;
} grs_point;

//these are control characters that have special meaning in the font code

#define CC_COLOR			1		//next char is new foreground color
#define CC_LSPACING		2		//next char specifies line spacing
#define CC_UNDERLINE		3		//next char is underlined

//now have string versions of these control characters (can concat inside a string)

#define CC_COLOR_S	 	"\x1"		//next char is new foreground color
#define CC_LSPACING_S 	"\x2"		//next char specifies line spacing
#define CC_UNDERLINE_S	"\x3"		//next char is underlined

typedef struct _grs_font 
{
	short		ft_w, ft_h;		// Width and height in pixels
	short		ft_flags;		// Proportional?
	short		ft_baseline;	//
	uint8_t		ft_minchar,		// The first and last chars defined by
		ft_maxchar;		// This font
	short		ft_bytewidth;	// Width in unsigned chars
	uint8_t* ft_data;			// Ptr to raw data.
	uint8_t** ft_chars;		// Ptrs to data for each char (required for prop font)
	short* ft_widths;		// Array of widths (required for prop font)
	uint8_t* ft_kerndata;	// Array of kerning triplet data
	uint8_t* ft_datablock; //this format sucks aaaaaaaaaaaaaaaaaaaaaa
} grs_font;

#define BM_LINEAR   0
#define BM_MODEX    1
#define BM_SVGA     2
#define BM_RGB15    3   //5 bits each r,g,b stored at 16 bits
#define BM_SVGA15   4

#define BM_FLAG_TRANSPARENT			1
#define BM_FLAG_SUPER_TRANSPARENT	2
#define BM_FLAG_NO_LIGHTING			4
#define BM_FLAG_RLE						8			// A run-length encoded bitmap.
#define BM_FLAG_PAGED_OUT				16			// This bitmap's data is paged out.
#define BM_FLAG_RLE_BIG				32			// for bitmaps that RLE to > 255 per row (i.e. cockpits)

typedef struct _grs_bitmap {
	short       bm_x, bm_y;      // Offset from parent's origin
	short       bm_w, bm_h;      // width,height
	int8_t       	bm_type;        // 0=Linear, 1=ModeX, 2=SVGA
	int8_t		bm_flags;		// bit 0 on means it has transparency.
								// bit 1 on means it has supertransparency
								// bit 2 on means it doesn't get passed through lighting.
	short	    bm_rowsize;     // unsigned char offset to next row
	unsigned char* bm_data;		// ptr to pixel data...
								//   Linear = *parent+(rowsize*y+x)
								//   ModeX = *parent+(rowsize*y+x/4)
								//   SVGA = *parent+(rowsize*y+x)
	unsigned short bm_selector;
	uint8_t			avg_color;	//	Average color of all pixels in texture map.
	int8_t			unused;		//	to 4-byte align.
} grs_bitmap;

//shortcuts
#define cv_w cv_bitmap.bm_w
#define cv_h cv_bitmap.bm_h

typedef struct _grs_canvas 
{
	grs_bitmap  cv_bitmap;      // the bitmap for this canvas
	short       cv_color;       // current color
	short       cv_drawmode;    // fill,XOR,etc.
	grs_font* cv_font;        // the currently selected font
	short       cv_font_fg_color;   // current font foreground color (-1==Invisible)
	short       cv_font_bg_color;   // current font background color (-1==Invisible)
} grs_canvas;

typedef struct _grs_screen // This is a video screen
{     
	grs_canvas  sc_canvas;      // Represents the entire screen
	short       sc_mode;        // Video mode number
	short       sc_w, sc_h;     // Actual Width and Height
	fix			sc_aspect;		//aspect ratio (w/h) for this screen
} grs_screen;

// Num Cols Rows Bpp Mode Pages Aspect
// --- ---- ---- --- ---- ----- ------
// 0   320  200  8   C    1.0   1.2:1
// 1   320  200  8   U    4.0   1.2
// 2   320  240  8   U    3.4   1.0
// 3   360  200  8   U    3.6   1.4
// 4   360  240  8   U    3.0   1.1
// 5   376  282  8   U    2.5   1.0
// 6   320  400  8   U    2.0   0.6
// 7   320  480  8   U    1.7   0.5
// 8   360  400  8   U    1.8   0.7
// 9   360  480  8   U    1.5   0.6
// 10  360  360  8   U    2.0   0.8
// 11  376  308  8   U    2.3   0.9
// 12  376  564  8   U    1.2   0.5
// 13  640  400  8   V    4.1   1.2     (Assuming 1 Meg video RAM)
// 14  640  480  8   V    3.4   1.0
// 15  800  600  8   V    2.2   1.0
// 16  1024 768  8   V    1.0   1.0
// 17  640  480  15  V    1.0   1.0
// 18  800  600  15  V    1.0   1.0

#define SM_ORIGINAL		-1
#define SM_320x200C     0
#define SM_320x200U     1
#define SM_320x240U     2
#define SM_360x200U     3
#define SM_360x240U     4
#define SM_376x282U     5
#define SM_320x400U     6
#define SM_320x480U     7
#define SM_360x400U     8
#define SM_360x480U     9
#define SM_360x360U     10
#define SM_376x308U     11
#define SM_376x564U     12
#define SM_640x400V     13
#define SM_640x480V     14
#define SM_800x600V     15
#define SM_1024x768V    16
#define SM_640x480V15   17
#define SM_800x600V15   18
#define SM_1280x1024V   23 //[ISB] h a c k since d1 makes 320x100 mode 19. ugh

#define SM_320x200x8	1
#define SM_320x200x8UL	2
#define SM_320x200x16	3

////=========================================================================
// System functions:
// setup and set mode. this creates a grs_screen structure and sets
// grd_curscreen to point to it.  grs_curcanv points to this screen's
// canvas.  Saves the current VGA state and screen mode.

int gr_init(int mode);

//shut down the 2d.  Restore the screen mode.
void gr_close();

//  0=Mode set OK
//  1=No VGA adapter installed
//  2=Program doesn't support this VESA granularity
//  3=Monitor doesn't support that VESA mode.:
//  4=Video card doesn't support that VESA mode.
//  5=No VESA driver found.
//  6=Bad Status after VESA call/
//  7=Not enough DOS memory to call VESA functions.
//  8=Error using DPMI.
//  9=Error setting logical line width.
// 10=Error allocating selector for A0000h
// 11=Not a valid mode support by gr.lib
// Returns one of the above without setting mode
int gr_check_mode(int mode);


//=========================================================================
// Canvas functions:

// Makes a new canvas. allocates memory for the canvas and its bitmap,
// including the raw pixel buffer.

grs_canvas* gr_create_canvas(int w, int h);

// Creates a canvas that is part of another canvas.  this can be used to make
// a window on the screen.  the canvas structure is malloc'd; the address of
// the raw pixel data is inherited from the parent canvas.

grs_canvas* gr_create_sub_canvas(grs_canvas* canv, int x, int y, int w, int h);

// Initialize the specified canvas. the raw pixel data buffer is passed as
// a parameter. no memory allocation is performed.

void gr_init_canvas(grs_canvas* canv, unsigned char* pixdata, int pixtype, int w, int h);

// Initialize the specified sub canvas. no memory allocation is performed.

void gr_init_sub_canvas(grs_canvas* newc, grs_canvas* src, int x, int y, int w, int h);

// Free up the canvas and its pixel data.

void gr_free_canvas(grs_canvas* canv);

// Free up the canvas. do not free the pixel data, which belongs to the
// parent canvas.

void gr_free_sub_canvas(grs_canvas* canv);

// Clear the current canvas to the specified color
void gr_clear_canvas(int color);

//=========================================================================
// Bitmap functions:

// Allocate a bitmap and its pixel data buffer.
grs_bitmap* gr_create_bitmap(int w, int h);

// Allocated a bitmap and makes its data be raw_data that is already somewhere.
grs_bitmap* gr_create_bitmap_raw(int w, int h, unsigned char* raw_data);

// Creates a bitmap which is part of another bitmap
grs_bitmap* gr_create_sub_bitmap(grs_bitmap* bm, int x, int y, int w, int h);

// Free the bitmap and its pixel data
void gr_free_bitmap(grs_bitmap* bm);

// Free the bitmap, but not the pixel data buffer
void gr_free_sub_bitmap(grs_bitmap* bm);

void gr_bm_pixel(grs_bitmap* bm, int x, int y, unsigned char color);
void gr_bm_upixel(grs_bitmap* bm, int x, int y, unsigned char color);
void gr_bm_ubitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest);
void gr_bm_ubitbltm(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest);

//[ISB] apparently this function was never prototyped anywhere
void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap* src, grs_bitmap* dest);

//=========================================================================
// Color functions:

// When this function is called, the guns are set to gr_palette, and
// the palette stays the same until gr_close is called

void gr_use_palette_table(const char* filename);

//=========================================================================
// Drawing functions:

// For solid, XOR, or other fill modes.
//[ISB] seems to be unused
//void gr_set_drawmode(int mode);

// Sets the color in the current canvas.  should be a macro
// Use: gr_setcolor(int color);
void gr_setcolor(int color);

// Draw a polygon into the current canvas in the current color and drawmode.
// verts points to an ordered list of x,y pairs.  the polygon should be
// convex; a concave polygon will be handled in some reasonable manner,
// but not necessarily shaded as a concave polygon. It shouldn't hang.
// probably good solution is to shade from minx to maxx on each scan line.
// int should really be fix
void gr_poly(int nverts, int* verts);
void gr_upoly(int nverts, int* verts);


// Draws a point into the current canvas in the current color and drawmode.
void gr_pixel(int x, int y);
void gr_upixel(int x, int y);

// Gets a pixel;
unsigned char gr_gpixel(grs_bitmap* bitmap, int x, int y);
unsigned char gr_ugpixel(grs_bitmap* bitmap, int x, int y);

// Draws a line into the current canvas in the current color and drawmode.
int gr_line(fix x0, fix y0, fix x1, fix y1);
int gr_uline(fix x0, fix y0, fix x1, fix y1);

// Draws an anti-aliased line into the current canvas in the current color and drawmode.
//int gr_aaline(fix x0, fix y0, fix x1, fix y1);
//int gr_uaaline(fix x0, fix y0, fix x1, fix y1); //[ISB] cut

// Draw the bitmap into the current canvas at the specified location.
void gr_bitmap(int x, int y, grs_bitmap* bm);
void gr_ubitmap(int x, int y, grs_bitmap* bm);
// bitmap function with transparency
void gr_bitmapm(int x, int y, grs_bitmap* bm);
void gr_ubitmapm(int x, int y, grs_bitmap* bm);

// Draw a rectangle into the current canvas.
void gr_rect(int left, int top, int right, int bot);
void gr_urect(int left, int top, int right, int bot);

// Draw a filled circle
int gr_disk(fix x, fix y, fix r);
int gr_udisk(fix x, fix y, fix r);

// Draw an outline circle
int gr_circle(fix x, fix y, fix r);
int gr_ucircle(fix x, fix y, fix r);

// Draw an unfilled rectangle into the current canvas
void gr_box(int left, int top, int right, int bot);
void gr_ubox(int left, int top, int right, int bot);

void gr_scanline(int x1, int x2, int y);
void gr_uscanline(int x1, int x2, int y);


// Reads in a font file... current font set to this one.
grs_font* gr_init_font(const char* fontfile);
void gr_remap_font(grs_font* font, char* fontname);
void gr_close_font(grs_font* font);
void gr_remap_color_fonts();

// Writes a string using current font. Returns the next column after last char.
void gr_set_fontcolor(int fg, int bg);
void gr_set_curfont(grs_font* newfont);
int gr_string(int x, int y, const char* s);
int gr_ustring(int x, int y, const char* s);
int gr_printf(int x, int y, const char* format, ...);
int gr_uprintf(int x, int y, const char* format, ...);
void gr_get_string_size(const char* s, int* string_width, int* string_height, int* average_width);

//	From roller.c
void rotate_bitmap(grs_bitmap* bp, grs_point* vertbuf, int light_value);

// From scale.c
#ifdef BUILD_DESCENT2
void scale_bitmap(grs_bitmap* bp, grs_point* vertbuf, int orientation);
#else
void scale_bitmap(grs_bitmap* bp, grs_point* vertbuf);
#endif

//===========================================================================
// Global variables
extern grs_canvas* grd_curcanv;             //active canvas
extern grs_screen* grd_curscreen;           //active screen
extern unsigned char Test_bitmap_data[64 * 64];

//extern unsigned int FixDivide(unsigned int x, unsigned int y); //[ISB] never called

extern void gr_show_canvas(grs_canvas* canv);
extern void gr_set_current_canvas(grs_canvas* canv);

//flags for fonts
#define FT_COLOR			1
#define FT_PROPORTIONAL	2
#define FT_KERNED			4

// Special effects
//extern void gr_snow_out(int num_dots); //[ISB] not in PC?

//extern void test_rotate_bitmap(void); //[ISB] not used?
extern void rotate_bitmap(grs_bitmap* bp, grs_point* vertbuf, int light_value);

extern uint8_t gr_palette[256 * 3];
extern uint8_t gr_fade_table[256 * GR_FADE_LEVELS];
extern uint8_t gr_inverse_table[32 * 32 * 32];

extern uint16_t gr_palette_selector;
extern uint16_t gr_inverse_table_selector;
extern uint16_t gr_fade_table_selector;

// Remaps a bitmap into the current palette. If transparent_color is between 0 and 255
// then all occurances of that color are mapped to whatever color the 2d uses for 
// transparency. This is normally used right after a call to iff_read_bitmap like
// this:
//		iff_error = iff_read_bitmap(filename,new,BM_LINEAR,newpal);
//		if (iff_error != IFF_NO_ERROR) Error("Can't load IFF file <%s>, error=%d",filename,iff_error);
//		if ( iff_has_transparency )
//			gr_remap_bitmap( new, newpal, iff_transparent_color );
//		else
//			gr_remap_bitmap( new, newpal, -1 );
extern void gr_remap_bitmap(grs_bitmap* bmp, uint8_t* palette, int transparent_color, int super_transparent_color);

// Same as above, but searches using gr_find_closest_color which uses 18-bit accurracy
// instaed of 15bit when translating colors.
extern void gr_remap_bitmap_good(grs_bitmap* bmp, uint8_t* palette, int transparent_color, int super_transparent_color);

extern void gr_palette_step_up(int r, int g, int b);

extern void gr_bitmap_check_transparency(grs_bitmap* bmp);

//#define GR_GETCOLOR(r,g,b) (gr_inverse_table[( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )])
//#define gr_getcolor(r,g,b) (gr_inverse_table[( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )])
//#define BM_XRGB(r,g,b) (gr_inverse_table[( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )])

#define BM_RGB(r,g,b) ( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )
#define BM_XRGB(r,g,b) gr_find_closest_color( (r)*2,(g)*2,(b)*2 )
#define GR_GETCOLOR(r,g,b) gr_find_closest_color( (r)*2,(g)*2,(b)*2 )
#define gr_getcolor(r,g,b) gr_find_closest_color( (r)*2,(g)*2,(b)*2 )

// Given: r,g,b, each in range of 0-63, return the color index that
// best matches the input.
int gr_find_closest_color(int r, int g, int b);

extern void gr_merge_textures(uint8_t* lower, uint8_t* upper, uint8_t* dest);
extern void gr_merge_textures_1(uint8_t* lower, uint8_t* upper, uint8_t* dest);
extern void gr_merge_textures_2(uint8_t* lower, uint8_t* upper, uint8_t* dest);
extern void gr_merge_textures_3(uint8_t* lower, uint8_t* upper, uint8_t* dest);

void gr_sync_display();
int gr_set_mode(int mode);

//[ISB] I MADE BAD DECISIONS AGAIN
#define VGA_current_mode grd_curscreen->sc_mode

//shortcut to look at current font
#define grd_curfont grd_curcanv->cv_font