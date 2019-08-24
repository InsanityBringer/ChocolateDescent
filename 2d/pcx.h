
#pragma once

#include "2d/gr.h"

#define PCX_ERROR_NONE 				0
#define PCX_ERROR_OPENING			1
#define PCX_ERROR_NO_HEADER		2
#define PCX_ERROR_WRONG_VERSION	3
#define PCX_ERROR_READING			4
#define PCX_ERROR_NO_PALETTE		5
#define PCX_ERROR_WRITING			6
#define PCX_ERROR_MEMORY			7

 // Reads filename into bitmap bmp, and fills in palette.  If bmp->bm_data==NULL, 
 // then bmp->bm_data is allocated and the w,h are filled.  
 // If palette==NULL the palette isn't read in.  Returns error code.

extern int pcx_read_bitmap(const char* filename, grs_bitmap * bmp, int bitmap_type, uint8_t * palette);

// Writes the bitmap bmp to filename, using palette. Returns error code.

extern int pcx_write_bitmap(const char* filename, grs_bitmap* bmp, uint8_t* palette);

extern char* pcx_errormsg(int error_number);
