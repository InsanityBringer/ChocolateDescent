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
#include <stddef.h>
#include "2d/gr.h"
#include "mem/mem.h"
#include "2d/pcx.h"
#include "cfile/cfile.h"

/* PCX Header data type */
typedef struct {
	uint8_t		Manufacturer;
	uint8_t		Version;
	uint8_t		Encoding;
	uint8_t		BitsPerPixel;
	short		Xmin;
	short		Ymin;
	short		Xmax;
	short		Ymax;
	short		Hdpi;
	short		Vdpi;
	uint8_t		ColorMap[16][3];
	uint8_t		Reserved;
	uint8_t		Nplanes;
	short		BytesPerLine;
	uint8_t		filler[60];
} PCXHeader;


int pcx_read_bitmap(const char* filename, grs_bitmap* bmp, int bitmap_type, uint8_t* palette)
{
	PCXHeader header;
	CFILE* PCXfile;
	int i, row, col, count, xsize, ysize;
	uint8_t data, * pixdata;

	PCXfile = cfopen(filename, "rb");
	if (!PCXfile)
		return PCX_ERROR_OPENING;

	// read 128 char PCX header
	if (cfread(&header, sizeof(PCXHeader), 1, PCXfile) != 1) {
		cfclose(PCXfile);
		return PCX_ERROR_NO_HEADER;
	}

	// Is it a 256 color PCX file?
	if ((header.Manufacturer != 10) || (header.Encoding != 1) || (header.Nplanes != 1) || (header.BitsPerPixel != 8) || (header.Version != 5)) {
		cfclose(PCXfile);
		return PCX_ERROR_WRONG_VERSION;
	}

	// Find the size of the image
	xsize = header.Xmax - header.Xmin + 1;
	ysize = header.Ymax - header.Ymin + 1;

	if (bitmap_type == BM_LINEAR) 
	{
		if (bmp->bm_data == NULL) 
		{
			memset(bmp, 0, sizeof(grs_bitmap));
			bmp->bm_data = (unsigned char*)malloc(xsize * ysize);
			if (bmp->bm_data == NULL) 
			{
				cfclose(PCXfile);
				return PCX_ERROR_MEMORY;
			}
			bmp->bm_w = bmp->bm_rowsize = xsize;
			bmp->bm_h = ysize;
			bmp->bm_type = bitmap_type;
		}
	}

	//if (bmp->bm_type == BM_LINEAR) 
	{
		for (row = 0; row < ysize; row++) 
		{
			pixdata = &bmp->bm_data[bmp->bm_rowsize * row];
			for (col = 0; col < xsize; ) 
			{
				if (cfread(&data, 1, 1, PCXfile) != 1) 
				{
					cfclose(PCXfile);
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0) 
				{
					count = data & 0x3F;
					if (cfread(&data, 1, 1, PCXfile) != 1) 
					{
						cfclose(PCXfile);
						return PCX_ERROR_READING;
					}
					memset(pixdata, data, count);
					pixdata += count;
					col += count;
				}
				else 
				{
					*pixdata++ = data;
					col++;
				}
			}
		}
	}
	/*else 
	{
		for (row = 0; row < ysize; row++) 
		{
			for (col = 0; col < xsize; ) 
			{
				if (cfread(&data, 1, 1, PCXfile) != 1) 
				{
					cfclose(PCXfile);
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0) 
				{
					count = data & 0x3F;
					if (cfread(&data, 1, 1, PCXfile) != 1) 
					{
						cfclose(PCXfile);
						return PCX_ERROR_READING;
					}
					for (i = 0; i < count; i++)
						gr_bm_pixel(bmp, col + i, row, data);
					col += count;
				}
				else 
				{
					gr_bm_pixel(bmp, col, row, data);
					col++;
				}
			}
		}
	}*/

	// Read the extended palette at the end of PCX file
	if (palette != NULL) 
	{
		// Read in a character which should be 12 to be extended palette file
		if (cfread(&data, 1, 1, PCXfile) == 1) 
		{
			if (data == 12) 
			{
				if (cfread(palette, 768, 1, PCXfile) != 1) 
				{
					cfclose(PCXfile);
					return PCX_ERROR_READING;
				}
				for (i = 0; i < 768; i++)
					palette[i] >>= 2;
			}
		}
		else 
		{
			cfclose(PCXfile);
			return PCX_ERROR_NO_PALETTE;
		}
	}
	cfclose(PCXfile);
	return PCX_ERROR_NONE;
}

int pcx_encode_line(uint8_t* inBuff, int inLen, FILE* fp);

int pcx_write_bitmap(const char* filename, grs_bitmap* bmp, uint8_t* palette)
{
	int retval;
	int i;
	uint8_t data;
	PCXHeader header;
	FILE* PCXfile;

	memset(&header, 0, sizeof(PCXHeader));

	header.Manufacturer = 10;
	header.Encoding = 1;
	header.Nplanes = 1;
	header.BitsPerPixel = 8;
	header.Version = 5;
	header.Xmax = bmp->bm_w - 1;
	header.Ymax = bmp->bm_h - 1;
	header.BytesPerLine = bmp->bm_w;

	//[ISB] knew this was a bad idea.....
	PCXfile = fopen(filename, "wb");
	//errno_t err = fopen_s(&PCXfile, filename, "wb");
	if (!PCXfile)
		return PCX_ERROR_OPENING;

	if (fwrite(&header, sizeof(PCXHeader), 1, PCXfile) != 1) 
	{
		fclose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	for (i = 0; i < bmp->bm_h; i++) 
	{
		if (!pcx_encode_line(&bmp->bm_data[bmp->bm_rowsize * i], bmp->bm_w, PCXfile))
		{
			fclose(PCXfile);
			return PCX_ERROR_WRITING;
		}
	}

	// Mark an extended palette	
	data = 12;
	if (fwrite(&data, 1, 1, PCXfile) != 1) 
	{
		fclose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	// Write the extended palette
	for (i = 0; i < 768; i++)
		palette[i] <<= 2;

	retval = fwrite(palette, 768, 1, PCXfile);

	for (i = 0; i < 768; i++)
		palette[i] >>= 2;

	if (retval != 1) 
	{
		fclose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	fclose(PCXfile);
	return PCX_ERROR_NONE;

}

int pcx_encode_byte(uint8_t byt, uint8_t cnt, FILE* fid);

// returns number of bytes written into outBuff, 0 if failed 
int pcx_encode_line(uint8_t* inBuff, int inLen, FILE* fp)
{
	uint8_t cur, last;
	int srcIndex, i;
	register int total;
	register uint8_t runCount; 	// max single runlength is 63
	total = 0;
	last = *(inBuff);
	runCount = 1;

	for (srcIndex = 1; srcIndex < inLen; srcIndex++) {
		cur = *(++inBuff);
		if (cur == last) {
			runCount++;			// it encodes
			if (runCount == 63) {
				if (!(i = pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
				runCount = 0;
			}
		}
		else {   	// cur != last
			if (runCount) {
				if (!(i = pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
			}
			last = cur;
			runCount = 1;
		}
	}

	if (runCount) {		// finish up
		if (!(i = pcx_encode_byte(last, runCount, fp)))
			return 0;
		return total + i;
	}
	return total;
}

// subroutine for writing an encoded byte pair 
// returns count of bytes written, 0 if error
int pcx_encode_byte(uint8_t byt, uint8_t cnt, FILE* fid)
{
	if (cnt) {
		if ((cnt == 1) && (0xc0 != (0xc0 & byt))) {
			if (EOF == putc((int)byt, fid))
				return 0; 	// disk write error (probably full)
			return 1;
		}
		else {
			if (EOF == putc((int)0xC0 | cnt, fid))
				return 0; 	// disk write error
			if (EOF == putc((int)byt, fid))
				return 0; 	// disk write error
			return 2;
		}
	}
	return 0;
}

//text for error messges
char pcx_error_messages[] = {
	"No error.\0"
	"Error opening file.\0"
	"Couldn't read PCX header.\0"
	"Unsupported PCX version.\0"
	"Error reading data.\0"
	"Couldn't find palette information.\0"
	"Error writing data.\0"
};


//function to return pointer to error message
char* pcx_errormsg(int error_number)
{
	char* p = pcx_error_messages;

	while (error_number--) {

		if (!p) return NULL;

		p += strlen(p) + 1;

	}

	return p;

}
