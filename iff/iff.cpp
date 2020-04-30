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

#define COMPRESS		1	//do the RLE or not? (for debugging mostly)
#define WRITE_TINY	0	//should we write a TINY chunk?

#define MIN_COMPRESS_WIDTH	65	//don't compress if less than this wide

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include "mem/mem.h"
#include "iff/iff.h"
#include "cfile/cfile.h"
#include "misc/error.h"

//Internal constants and structures for this library

//Type values for bitmaps
#define TYPE_PBM		0
#define TYPE_ILBM		1

//Compression types
#define cmpNone	0
#define cmpByteRun1	1

//Masking types
#define mskNone	0
#define mskHasMask	1
#define mskHasTransparentColor 2

//Palette entry structure
typedef struct pal_entry { int8_t r, g, b; } pal_entry;

//structure of the header in the file
typedef struct iff_bitmap_header {
	short w, h;						//width and height of this bitmap
	short x, y;						//generally unused
	short type;						//see types above
	short transparentcolor;		//which color is transparent (if any)
	short pagewidth, pageheight; //width & height of source screen
	int8_t nplanes;					//number of planes (8 for 256 color image)
	int8_t masking, compression;	//see constants above
	int8_t xaspect, yaspect;		//aspect ratio (usually 5/6)
	pal_entry palette[256];		//the palette for this bitmap
	uint8_t* raw_data;				//ptr to array of data
	short row_size;				//offset to next row
} iff_bitmap_header;

uint8_t iff_transparent_color;
uint8_t iff_has_transparency;	// 0=no transparency, 1=iff_transparent_color is valid

typedef struct fake_file {
	uint8_t* data;
	int position;
	int length;
} FFILE;

#define MIN(a,b) ((a<b)?a:b)

#define MAKE_SIG(a,b,c,d) (((int32_t)(a)<<24)+((int32_t)(b)<<16)+((c)<<8)+(d))

#define form_sig MAKE_SIG('F','O','R','M')
#define ilbm_sig MAKE_SIG('I','L','B','M')
#define body_sig MAKE_SIG('B','O','D','Y')
#define pbm_sig  MAKE_SIG('P','B','M',' ')
#define bmhd_sig MAKE_SIG('B','M','H','D')
#define anhd_sig MAKE_SIG('A','N','H','D')
#define cmap_sig MAKE_SIG('C','M','A','P')
#define tiny_sig MAKE_SIG('T','I','N','Y')
#define anim_sig MAKE_SIG('A','N','I','M')
#define dlta_sig MAKE_SIG('D','L','T','A')

#ifndef NDEBUG
//void printsig(int32_t s)
//{
//	char *t=(char *) &s;
//
///*  printf("%c%c%c%c",*(&s+3),*(&s+2),*(&s+1),s);*/
//	printf("%c%c%c%c",t[3],t[2],t[1],t[0]);
//}
#endif

int32_t get_sig(FFILE* f)
{
	char s[4];

	//	if ((s[3]=cfgetc(f))==EOF) return(EOF);
	//	if ((s[2]=cfgetc(f))==EOF) return(EOF);
	//	if ((s[1]=cfgetc(f))==EOF) return(EOF);
	//	if ((s[0]=cfgetc(f))==EOF) return(EOF);

	if (f->position >= f->length) return EOF;
	s[3] = f->data[f->position++];
	if (f->position >= f->length) return EOF;
	s[2] = f->data[f->position++];
	if (f->position >= f->length) return EOF;
	s[1] = f->data[f->position++];
	if (f->position >= f->length) return EOF;
	s[0] = f->data[f->position++];

	return(*((int32_t*)s));
}

int put_sig(int32_t sig, FILE* f)
{
	char* s = (char*)& sig;

	fputc(s[3], f);
	fputc(s[2], f);
	fputc(s[1], f);
	return fputc(s[0], f);

}


int get_word(FFILE* f)
{
	unsigned char c0, c1;

	//	c1=cfgetc(f);
	//	c0=cfgetc(f);

	if (f->position >= f->length) return EOF;
	c1 = f->data[f->position++];
	if (f->position >= f->length) return EOF;
	c0 = f->data[f->position++];

	if (c0 == 0xff) return(EOF);

	return(((int)c1 << 8) + c0);

}

int put_byte(unsigned char c, FILE* f)
{
	return fputc(c, f);
}

int put_word(int n, FILE* f)
{
	unsigned char c0, c1;

	c0 = (n & 0xff00) >> 8;
	c1 = n & 0xff;

	put_byte(c0, f);
	return put_byte(c1, f);
}

int put_long(int32_t n, FILE* f)
{
	int n0, n1;

	n0 = (int)((n & 0xffff0000l) >> 16);
	n1 = (int)(n & 0xffff);

	put_word(n0, f);
	return put_word(n1, f);

}

char get_byte(FFILE* f)
{
	//return cfgetc(f);
	return f->data[f->position++];
}

int32_t get_long(FFILE* f)
{
	unsigned char c0, c1, c2, c3;

	//c3=cfgetc(f);
	//c2=cfgetc(f);
	//c1=cfgetc(f);
	//c0=cfgetc(f);

	if (f->position >= f->length) return EOF;
	c3 = f->data[f->position++];
	if (f->position >= f->length) return EOF;
	c2 = f->data[f->position++];
	if (f->position >= f->length) return EOF;
	c1 = f->data[f->position++];
	if (f->position >= f->length) return EOF;
	c0 = f->data[f->position++];

	//printf("get_long %x %x %x %x\n",c3,c2,c1,c0);

	//  if (c0==0xff) return(EOF);

	return(((int32_t)c3 << 24) + ((int32_t)c2 << 16) + ((int32_t)c1 << 8) + c0);

}

int parse_bmhd(FFILE* ifile, int32_t len, iff_bitmap_header* bmheader)
{
	len++;  /* so no "parm not used" warning */

//  debug("parsing bmhd len=%ld\n",len);

	bmheader->w = get_word(ifile);
	bmheader->h = get_word(ifile);
	bmheader->x = get_word(ifile);
	bmheader->y = get_word(ifile);

	bmheader->nplanes = get_byte(ifile);
	bmheader->masking = get_byte(ifile);
	bmheader->compression = get_byte(ifile);
	get_byte(ifile);        /* skip pad */

	bmheader->transparentcolor = get_word(ifile);
	bmheader->xaspect = get_byte(ifile);
	bmheader->yaspect = get_byte(ifile);

	bmheader->pagewidth = get_word(ifile);
	bmheader->pageheight = get_word(ifile);

	iff_transparent_color = (uint8_t)bmheader->transparentcolor;

	iff_has_transparency = 0;

	if (bmheader->masking == mskHasTransparentColor)
		iff_has_transparency = 1;

	else if (bmheader->masking != mskNone && bmheader->masking != mskHasMask)
		return IFF_UNKNOWN_MASK;

	//  debug("w,h=%d,%d x,y=%d,%d\n",w,h,x,y);
	//  debug("nplanes=%d, masking=%d ,compression=%d, transcolor=%d\n",nplanes,masking,compression,transparentcolor);

	return IFF_NO_ERROR;
}


//  the buffer pointed to by raw_data is stuffed with a pointer to decompressed pixel data
int parse_body(FFILE* ifile, int32_t len, iff_bitmap_header* bmheader)
{
	unsigned char* p = bmheader->raw_data;
	int width = 0, depth = 0;
	signed char n;
	int nn, wid_cnt, end_cnt, plane;
	char ignore = 0;
	unsigned char* data_end;
	int end_pos;
	int row_count = 0;

	end_pos = ifile->position + len;
	if (len & 1)
		end_pos++;

	if (bmheader->type == TYPE_PBM) {
		width = bmheader->w;
		depth = 1;
	}
	else if (bmheader->type == TYPE_ILBM) {
		width = (bmheader->w + 7) / 8;
		depth = bmheader->nplanes;
	}

	end_cnt = (width & 1) ? -1 : 0;

	data_end = p + width * bmheader->h * depth;

	if (bmheader->compression == cmpNone) {        /* no compression */
		int y;

		for (y = bmheader->h; y; y--) {
			int x;
			//			for (x=bmheader->w;x;x--) *p++=cfgetc(ifile);
			//			cfread(p, bmheader->w, 1, ifile);
			//			p += bmheader->w;

			for (x = 0; x < width * depth; x++)
				* p++ = ifile->data[ifile->position++];

			if (bmheader->masking == mskHasMask)
				ifile->position += width;				//skip mask!

//			if (bmheader->w & 1) ignore = cfgetc(ifile);
			if (bmheader->w & 1) ifile->position++;
		}

		//cnt = len - bmheader->h * ((bmheader->w+1)&~1);

	}
	else if (bmheader->compression == cmpByteRun1)
		for (wid_cnt = width, plane = 0; ifile->position < end_pos && p < data_end;) {
			unsigned char c;

			//			if (old_cnt-cnt > 2048) {
			//              printf(".");
			//				old_cnt=cnt;
			//			}

			if (wid_cnt == end_cnt) {
				wid_cnt = width;
				plane++;
				if ((bmheader->masking == mskHasMask && plane == depth + 1) ||
					(bmheader->masking != mskHasMask && plane == depth))
					plane = 0;
			}

			Assert(wid_cnt > end_cnt);

			//n=cfgetc(ifile);
			n = ifile->data[ifile->position++];

			if (n >= 0) {                       // copy next n+1 bytes from source, they are not compressed
				nn = (int)n + 1;
				wid_cnt -= nn;
				if (wid_cnt == -1) { --nn; Assert(width & 1); }
				if (plane == depth)	//masking row
					ifile->position += nn;
				else
					while (nn--)* p++ = ifile->data[ifile->position++];
				if (wid_cnt == -1) ifile->position++;
			}
			else if (n >= -127) {             // next -n + 1 bytes are following byte
				c = ifile->data[ifile->position++];
				nn = (int)-n + 1;
				wid_cnt -= nn;
				if (wid_cnt == -1) { --nn; Assert(width & 1); }
				if (plane != depth)	//not masking row
				{
					memset(p, c, nn); p += nn;
				}
			}

#ifndef NDEBUG
			if ((p - bmheader->raw_data) % width == 0)
				row_count++;

			Assert((p - bmheader->raw_data) - (width * row_count) < width);
#endif

		}

#ifndef BUILD_DESCENT2
	if (bmheader->masking == mskHasMask && p == data_end && ifile->position == end_pos - 2)		//I don't know why...
		ifile->position++;		//...but if I do this it works

	if (p == data_end && ifile->position == end_pos - 1)		//must be a pad byte
		//ignore = cfgetc(ifile);		//get pad byte
		ifile->position++;
	else
		if (ifile->position != end_pos || p != data_end) {
			//			debug("IFF Error: p=%x, data_end=%x, cnt=%d\n",p,data_end,cnt);
			return IFF_CORRUPT;
		}
#else
	if (p != data_end)				//if we don't have the whole bitmap...
		return IFF_CORRUPT;		//...the give an error

	//Pretend we read the whole chuck, because if we didn't, it's because
	//we didn't read the last mask like or the last rle record for padding
	//or whatever and it's not important, because we check to make sure
	//we got the while bitmap, and that's what really counts.

	ifile->position = end_pos;
#endif

	if (ignore) ignore++;   // haha, suppress the evil warning message

	return IFF_NO_ERROR;
}

//modify passed bitmap
int parse_delta(FFILE* ifile, int32_t len, iff_bitmap_header* bmheader)
{
	unsigned char* p = bmheader->raw_data;
	int y;
	int32_t chunk_end = ifile->position + len;

	get_long(ifile);		//longword, seems to be equal to 4.  Don't know what it is

	for (y = 0; y < bmheader->h; y++) {
		uint8_t n_items;
		int cnt = bmheader->w;
		uint8_t code;

		n_items = get_byte(ifile);

		while (n_items--) {

			code = get_byte(ifile);

			if (code == 0) {				//repeat
				uint8_t rep, val;

				rep = get_byte(ifile);
				val = get_byte(ifile);

				cnt -= rep;
				if (cnt == -1)
					rep--;
				while (rep--)
					* p++ = val;
			}
			else if (code > 0x80) {	//skip
				cnt -= (code - 0x80);
				p += (code - 0x80);
				if (cnt == -1)
					p--;
			}
			else {						//literal
				cnt -= code;
				if (cnt == -1)
					code--;

				while (code--)
					* p++ = get_byte(ifile);

				if (cnt == -1)
					get_byte(ifile);
			}

		}

		if (cnt == -1) {
			if (!bmheader->w & 1)
				return IFF_CORRUPT;
		}
		else if (cnt)
			return IFF_CORRUPT;
	}

	if (ifile->position == chunk_end - 1)		//pad
		ifile->position++;

	if (ifile->position != chunk_end)
		return IFF_CORRUPT;
	else
		return IFF_NO_ERROR;
}

//  the buffer pointed to by raw_data is stuffed with a pointer to bitplane pixel data
void skip_chunk(FFILE* ifile, int32_t len)
{
	//int c,i;
	int ilen;
	ilen = len + 1 & ~1;

	//printf( "Skipping %d chunk\n", ilen );

	ifile->position += ilen;

	if (ifile->position >= ifile->length) {
		ifile->position = ifile->length;
	}


	//	for (i=0; i<ilen; i++ )
	//		c = cfgetc(ifile);
		//Assert(cfseek(ifile,ilen,SEEK_CUR)==0);
}

//read an ILBM or PBM file
// Pass pointer to opened file, and to empty bitmap_header structure, and form length
int iff_parse_ilbm_pbm(FFILE* ifile, int32_t form_type, iff_bitmap_header* bmheader, int form_len, grs_bitmap* prev_bm)
{
	int32_t sig, len;
	//char ignore=0;
	int32_t start_pos, end_pos;

	start_pos = ifile->position;
	end_pos = start_pos - 4 + form_len;

	//      printf(" %ld ",form_len);
	//      printsig(form_type);
	//      printf("\n");

	if (form_type == pbm_sig)
		bmheader->type = TYPE_PBM;
	else
		bmheader->type = TYPE_ILBM;

	while ((ifile->position < end_pos) && (sig = get_sig(ifile)) != EOF) {

		len = get_long(ifile);
		if (len == EOF) break;

		//              printf(" ");
		//              printsig(sig);
		//              printf(" %ld\n",len);

		switch (sig) {

		case bmhd_sig: {
			int ret;
			int save_w = bmheader->w, save_h = bmheader->h;

			//printf("Parsing header\n");

			ret = parse_bmhd(ifile, len, bmheader);

			if (ret != IFF_NO_ERROR)
				return ret;

			if (bmheader->raw_data) {

				if (save_w != bmheader->w || save_h != bmheader->h)
					return IFF_BM_MISMATCH;

			}
			else {

				MALLOC( bmheader->raw_data, uint8_t, bmheader->w * bmheader->h );
				if (!bmheader->raw_data)
					return IFF_NO_MEM;
			}

			break;

		}

		case anhd_sig:

			if (!prev_bm) return IFF_CORRUPT;

			bmheader->w = prev_bm->bm_w;
			bmheader->h = prev_bm->bm_h;
			bmheader->type = prev_bm->bm_type;

			MALLOC( bmheader->raw_data, uint8_t, bmheader->w * bmheader->h );

			memcpy(bmheader->raw_data, prev_bm->bm_data, bmheader->w * bmheader->h);
			skip_chunk(ifile, len);

			break;

		case cmap_sig:
		{
			int ncolors = (int)(len / 3), cnum;
			unsigned char r, g, b;

			//printf("Parsing RGB map\n");
			for (cnum = 0; cnum < ncolors; cnum++) {
				//							r=cfgetc(ifile);
				//							g=cfgetc(ifile);
				//							b=cfgetc(ifile);
				r = ifile->data[ifile->position++];
				g = ifile->data[ifile->position++];
				b = ifile->data[ifile->position++];
				r >>= 2; bmheader->palette[cnum].r = r;
				g >>= 2; bmheader->palette[cnum].g = g;
				b >>= 2; bmheader->palette[cnum].b = b;
			}
			//if (len & 1) ignore = cfgetc(ifile);
			if (len & 1) ifile->position++;

			break;
		}

		case body_sig:
		{
			int r;
			//printf("Parsing body\n");
			if ((r = parse_body(ifile, len, bmheader)) != IFF_NO_ERROR)
				return r;
			break;
		}
		case dlta_sig:
		{
			int r;
			if ((r = parse_delta(ifile, len, bmheader)) != IFF_NO_ERROR)
				return r;
			break;
		}
		default:
			skip_chunk(ifile, len);
			break;
		}
	}

	//if (ignore) ignore++;

	if (ifile->position != start_pos - 4 + form_len)
		return IFF_CORRUPT;

	return IFF_NO_ERROR;    /* ok! */
}

//convert an ILBM file to a PBM file
int convert_ilbm_to_pbm(iff_bitmap_header* bmheader)
{
	int x, y, p;
	int8_t* new_data, * destptr, * rowptr;
	int bytes_per_row, byteofs;
	uint8_t checkmask, newbyte, setbit;

	MALLOC( new_data, int8_t, bmheader->w * bmheader->h );
	if (new_data == NULL) return IFF_NO_MEM;

	destptr = new_data;

	bytes_per_row = 2 * ((bmheader->w + 15) / 16);

	for (y = 0; y < bmheader->h; y++) {

		rowptr = (int8_t*)&bmheader->raw_data[y * bytes_per_row * bmheader->nplanes];

		for (x = 0, checkmask = 0x80; x < bmheader->w; x++) {

			byteofs = x >> 3;

			for (p = newbyte = 0, setbit = 1; p < bmheader->nplanes; p++) {

				if (rowptr[bytes_per_row * p + byteofs] & checkmask)
					newbyte |= setbit;

				setbit <<= 1;
			}

			*destptr++ = newbyte;

			if ((checkmask >>= 1) == 0) checkmask = 0x80;
		}

	}

	free(bmheader->raw_data);
	bmheader->raw_data = (uint8_t*)new_data;

	bmheader->type = TYPE_PBM;

	return IFF_NO_ERROR;
}

#define INDEX_TO_15BPP(i) ((short)((((palptr[(i)].r/2)&31)<<10)+(((palptr[(i)].g/2)&31)<<5)+((palptr[(i)].b/2 )&31)))

int convert_rgb15(grs_bitmap* bm, iff_bitmap_header* bmheader)
{
	uint16_t* new_data;
	int x, y;
	int newptr = 0;
	pal_entry* palptr;

	palptr = bmheader->palette;

	//        if ((new_data = malloc(bm->bm_w * bm->bm_h * 2)) == NULL)
	//            {ret=IFF_NO_MEM; goto done;}
	MALLOC(new_data, uint16_t, bm->bm_w * bm->bm_h * 2);
	if (new_data == NULL)
		return IFF_NO_MEM;

	for (y = 0; y < bm->bm_h; y++) {

		for (x = 0; x < bmheader->w; x++)
			new_data[newptr++] = INDEX_TO_15BPP(bmheader->raw_data[y * bmheader->w + x]);

	}

	free(bm->bm_data);				//get rid of old-style data
	bm->bm_data = (uint8_t*)new_data;			//..and point to new data

	bm->bm_rowsize *= 2;				//two bytes per row

	return IFF_NO_ERROR;

}

//read in a entire file into a fake file structure
int open_fake_file(char* ifilename, FFILE* ffile)
{
	CFILE* ifile;
	int ret;

	//printf( "Reading %s\n", ifilename );

	ffile->data = NULL;

	if ((ifile = cfopen(ifilename, "rb")) == NULL) return IFF_NO_FILE;

	ffile->length = cfilelength(ifile);

	MALLOC(ffile->data,uint8_t,ffile->length);

	if ((int)cfread(ffile->data, 1, ffile->length, ifile) < ffile->length)
		ret = IFF_READ_ERROR;
	else
		ret = IFF_NO_ERROR;

	ffile->position = 0;

	if (ifile) cfclose(ifile);

	return ret;
}

void close_fake_file(FFILE* f)
{
	if (f->data)
		free(f->data);

	f->data = NULL;
}

//copy an iff header structure to a grs_bitmap structure
void copy_iff_to_grs(grs_bitmap* bm, iff_bitmap_header* bmheader)
{
	bm->bm_x = bm->bm_y = 0;
	bm->bm_w = bmheader->w;
	bm->bm_h = bmheader->h;
	bm->bm_type = (int8_t)bmheader->type;
	bm->bm_rowsize = bmheader->w;
	bm->bm_data = bmheader->raw_data;

	bm->bm_flags = 0;
	bm->bm_selector = 0;
}

//if bm->bm_data is set, use it (making sure w & h are correct), else
//allocate the memory
int iff_parse_bitmap(FFILE* ifile, grs_bitmap* bm, int bitmap_type, int8_t* palette, grs_bitmap* prev_bm)
{
	int ret;			//return code
	iff_bitmap_header bmheader;
	int32_t sig, form_len;
	int32_t form_type;

	bmheader.raw_data = bm->bm_data;

	if (bmheader.raw_data) {
		bmheader.w = bm->bm_w;
		bmheader.h = bm->bm_h;
	}

	sig = get_sig(ifile);

	if (sig != form_sig) {
		ret = IFF_NOT_IFF;
		goto done;
	}

	form_len = get_long(ifile);

	form_type = get_sig(ifile);

	if (form_type == anim_sig)
		ret = IFF_FORM_ANIM;
	else if ((form_type == pbm_sig) || (form_type == ilbm_sig))
		ret = iff_parse_ilbm_pbm(ifile, form_type, &bmheader, form_len, prev_bm);
	else
		ret = IFF_UNKNOWN_FORM;

	if (ret != IFF_NO_ERROR) {		//got an error parsing
		if (bmheader.raw_data) free(bmheader.raw_data);
		goto done;
	}

	//If IFF file is ILBM, convert to PPB
	if (bmheader.type == TYPE_ILBM) {

		ret = convert_ilbm_to_pbm(&bmheader);

		if (ret != IFF_NO_ERROR) goto done;
	}

	//Copy data from iff_bitmap_header structure into grs_bitmap structure

	copy_iff_to_grs(bm, &bmheader);

	if (palette) memcpy(palette, &bmheader.palette, sizeof(bmheader.palette));

	//Now do post-process if required

	if (bitmap_type == BM_RGB15) {
		ret = convert_rgb15(bm, &bmheader);
		if (ret != IFF_NO_ERROR) goto done;
	}

done:

	return ret;

}

//returns error codes - see IFF.H.  see GR.H for bitmap_type
int iff_read_bitmap(char* ifilename, grs_bitmap* bm, int bitmap_type, uint8_t* palette)
{
	int ret;			//return code
	FFILE ifile;

	ret = open_fake_file(ifilename, &ifile);		//read in entire file
	if (ret != IFF_NO_ERROR) goto done;

	bm->bm_data = NULL;

	ret = iff_parse_bitmap(&ifile, bm, bitmap_type, (int8_t*)palette, NULL);

done:

	if (ifile.data) free(ifile.data);

	close_fake_file(&ifile);

	return ret;


}

//like iff_read_bitmap(), but reads into a bitmap that already exists,
//without allocating memory for the bitmap. 
int iff_read_into_bitmap(char* ifilename, grs_bitmap* bm, uint8_t* palette)
{
	int ret;			//return code
	FFILE ifile;

	ret = open_fake_file(ifilename, &ifile);		//read in entire file
	if (ret != IFF_NO_ERROR) goto done;

	ret = iff_parse_bitmap(&ifile, bm, bm->bm_type, (int8_t*)palette, NULL);

done:

	if (ifile.data) free(ifile.data);

	close_fake_file(&ifile);

	return ret;


}

#define BMHD_SIZE 20

int write_bmhd(FILE * ofile, iff_bitmap_header * bitmap_header)
{
	put_sig(bmhd_sig, ofile);
	put_long((int32_t)BMHD_SIZE, ofile);

	put_word(bitmap_header->w, ofile);
	put_word(bitmap_header->h, ofile);
	put_word(bitmap_header->x, ofile);
	put_word(bitmap_header->y, ofile);

	put_byte(bitmap_header->nplanes, ofile);
	put_byte(bitmap_header->masking, ofile);
	put_byte(bitmap_header->compression, ofile);
	put_byte(0, ofile);	/* pad */

	put_word(bitmap_header->transparentcolor, ofile);
	put_byte(bitmap_header->xaspect, ofile);
	put_byte(bitmap_header->yaspect, ofile);

	put_word(bitmap_header->pagewidth, ofile);
	put_word(bitmap_header->pageheight, ofile);

	return IFF_NO_ERROR;

}

int write_pal(FILE* ofile, iff_bitmap_header* bitmap_header)
{
	int	i;

	int n_colors = 1 << bitmap_header->nplanes;

	put_sig(cmap_sig, ofile);
	//	put_long(sizeof(pal_entry) * n_colors,ofile);
	put_long(3 * n_colors, ofile);

	//printf("new write pal %d %d\n",3,n_colors);

	for (i = 0; i < 256; i++) {
		unsigned char r, g, b;
		r = bitmap_header->palette[i].r * 4 + (bitmap_header->palette[i].r ? 3 : 0);
		g = bitmap_header->palette[i].g * 4 + (bitmap_header->palette[i].g ? 3 : 0);
		b = bitmap_header->palette[i].b * 4 + (bitmap_header->palette[i].b ? 3 : 0);
		fputc(r, ofile);
		fputc(g, ofile);
		fputc(b, ofile);
	}

	//printf("write pal %d %d\n",sizeof(pal_entry),n_colors);
	//	fwrite(bitmap_header->palette,sizeof(pal_entry),n_colors,ofile);

	return IFF_NO_ERROR;
}

int rle_span(uint8_t* dest, uint8_t* src, int len)
{
	int n, lit_cnt, rep_cnt;
	uint8_t last, * cnt_ptr = NULL, * dptr;

	dptr = dest;

	last = src[0]; lit_cnt = 1;

	for (n = 1; n < len; n++) 
	{

		if (src[n] == last) 
		{

			rep_cnt = 2;

			n++;
			while (n < len && rep_cnt < 128 && src[n] == last) 
			{
				n++; rep_cnt++; 
			}

			if (rep_cnt > 2 || lit_cnt < 2) 
			{
				if (cnt_ptr == NULL)
				{
					Error("rle_span: cnt_ptr is NULL\n");
					return 0;
				}
				if (lit_cnt > 1) 
				{ 
					*cnt_ptr = lit_cnt - 2; --dptr; 
				}		//save old lit count
				*dptr++ = -(rep_cnt - 1);
				*dptr++ = last;
				last = src[n];
				lit_cnt = (n < len) ? 1 : 0;

				continue;		//go to next char
			}
			else n--;

		}

		{

			if (lit_cnt == 1) 
			{
				cnt_ptr = dptr++;		//save place for count
				*dptr++ = last;			//store first char
			}

			*dptr++ = last = src[n];

			if (lit_cnt == 127) 
			{
				*cnt_ptr = lit_cnt;
				//cnt_ptr = dptr++;
				lit_cnt = 1;
				last = src[++n];
			}
			else lit_cnt++;
		}


	}

	if (lit_cnt == 1)
	{
		*dptr++ = 0;
		*dptr++ = last;			//store first char
	}
	else if (lit_cnt > 1)
		* cnt_ptr = lit_cnt - 1;

	return dptr - dest;
}

#define EVEN(a) ((a+1)&0xfffffffel)

//returns length of chunk
int write_body(FILE* ofile, iff_bitmap_header* bitmap_header, int compression_on)
{
	int w = bitmap_header->w, h = bitmap_header->h;
	int y, odd = w & 1;
	int32_t len = EVEN(w) * h, newlen, total_len = 0;
	uint8_t* p = bitmap_header->raw_data, * new_span;
	int32_t save_pos;

	put_sig(body_sig, ofile);
	save_pos = ftell(ofile);
	put_long(len, ofile);

	//if (! (new_span = malloc(bitmap_header->w+(bitmap_header->w/128+2)*2))) return IFF_NO_MEM;
	MALLOC( new_span, uint8_t, bitmap_header->w + (bitmap_header->w/128+2)*2);
	if (new_span == NULL) return IFF_NO_MEM;

	for (y = bitmap_header->h; y--;) {

		if (compression_on) {
			total_len += newlen = rle_span(new_span, p, bitmap_header->w + odd);
			fwrite(new_span, newlen, 1, ofile);
		}
		else
			fwrite(p, bitmap_header->w + odd, 1, ofile);

		p += bitmap_header->row_size;	//bitmap_header->w;
	}

	if (compression_on) {		//write actual data length
		Assert(fseek(ofile, save_pos, SEEK_SET) == 0);
		put_long(total_len, ofile);
		Assert(fseek(ofile, total_len, SEEK_CUR) == 0);
		if (total_len & 1) fputc(0, ofile);		//pad to even
	}

	free(new_span);

	return ((compression_on) ? (EVEN(total_len) + 8) : (len + 8));

}

#if WRITE_TINY
//write a small representation of a bitmap. returns size
int write_tiny(CFILE* ofile, iff_bitmap_header* bitmap_header, int compression_on)
{
	int skip;
	int new_w, new_h;
	int len, total_len = 0, newlen;
	int x, y, xofs, odd;
	uint8_t* p = bitmap_header->raw_data;
	uint8_t tspan[80], new_span[80 * 2];
	int32_t save_pos;

	skip = max((bitmap_header->w + 79) / 80, (bitmap_header->h + 63) / 64);

	new_w = bitmap_header->w / skip;
	new_h = bitmap_header->h / skip;

	odd = new_w & 1;

	len = new_w * new_h + 4;

	put_sig(tiny_sig, ofile);
	save_pos = cftell(ofile);
	put_long(EVEN(len), ofile);

	put_word(new_w, ofile);
	put_word(new_h, ofile);

	for (y = 0; y < new_h; y++) {
		for (x = xofs = 0; x < new_w; x++, xofs += skip)
			tspan[x] = p[xofs];

		if (compression_on) {
			total_len += newlen = rle_span(new_span, tspan, new_w + odd);
			fwrite(new_span, newlen, 1, ofile);
		}
		else
			fwrite(p, new_w + odd, 1, ofile);

		p += skip * bitmap_header->row_size;		//bitmap_header->w;

	}

	if (compression_on) {
		Assert(cfseek(ofile, save_pos, SEEK_SET) == 0);
		put_long(4 + total_len, ofile);
		Assert(cfseek(ofile, 4 + total_len, SEEK_CUR) == 0);
		if (total_len & 1) cfputc(0, ofile);		//pad to even
	}

	return ((compression_on) ? (EVEN(total_len) + 8 + 4) : (len + 8));
}
#endif

int write_pbm(FILE* ofile, iff_bitmap_header* bitmap_header, int compression_on)			/* writes a pbm iff file */
{
	int ret;
	int32_t raw_size = EVEN(bitmap_header->w) * bitmap_header->h;
	int32_t body_size, tiny_size, pbm_size = 4 + BMHD_SIZE + 8 + EVEN(raw_size) + sizeof(pal_entry) * (1 << bitmap_header->nplanes) + 8;
	int32_t save_pos;

	//printf("write_pbm\n");

	put_sig(form_sig, ofile);
	save_pos = ftell(ofile);
	put_long(pbm_size + 8, ofile);
	put_sig(pbm_sig, ofile);

	ret = write_bmhd(ofile, bitmap_header);
	if (ret != IFF_NO_ERROR) return ret;

	ret = write_pal(ofile, bitmap_header);
	if (ret != IFF_NO_ERROR) return ret;

#if WRITE_TINY
	tiny_size = write_tiny(ofile, bitmap_header, compression_on);
#else
	tiny_size = 0;
#endif

	body_size = write_body(ofile, bitmap_header, compression_on);

	pbm_size = 4 + BMHD_SIZE + body_size + tiny_size + sizeof(pal_entry) * (1 << bitmap_header->nplanes) + 8;

	Assert(fseek(ofile, save_pos, SEEK_SET) == 0);
	put_long(pbm_size + 8, ofile);
	Assert(fseek(ofile, pbm_size + 8, SEEK_CUR) == 0);

	return ret;

}

//writes an IFF file from a grs_bitmap structure. writes palette if not null
//returns error codes - see IFF.H.
int iff_write_bitmap(char* ofilename, grs_bitmap* bm, int8_t* palette)
{
	FILE* ofile;
	iff_bitmap_header bmheader;
	int ret;
	int compression_on;

	if (bm->bm_type == BM_RGB15) return IFF_BAD_BM_TYPE;

#if COMPRESS
	compression_on = (bm->bm_w >= MIN_COMPRESS_WIDTH);
#else
	compression_on = 0;
#endif

	//fill in values in bmheader

	bmheader.x = bmheader.y = 0;
	bmheader.w = bm->bm_w;
	bmheader.h = bm->bm_h;
	bmheader.type = TYPE_PBM;
	bmheader.transparentcolor = iff_transparent_color;
	bmheader.pagewidth = bm->bm_w;	//I don't think it matters what I write
	bmheader.pageheight = bm->bm_h;
	bmheader.nplanes = 8;
	bmheader.masking = mskNone;
	if (iff_has_transparency) {
		bmheader.masking |= mskHasTransparentColor;
	}
	bmheader.compression = (compression_on ? cmpByteRun1 : cmpNone);

	bmheader.xaspect = bmheader.yaspect = 1;	//I don't think it matters what I write
	bmheader.raw_data = bm->bm_data;
	bmheader.row_size = bm->bm_rowsize;

	if (palette) memcpy(&bmheader.palette, palette, 256 * 3);

	//open file and write

	if ((ofile = fopen(ofilename, "wb")) == NULL) { ret = IFF_NO_FILE; goto done; }

	ret = write_pbm(ofile, &bmheader, compression_on);

done:
	if (ofile)
		fclose(ofile);

	return ret;
}

//read in many brushes.  fills in array of pointers, and n_bitmaps.
//returns iff error codes
int iff_read_animbrush(char* ifilename, grs_bitmap** bm_list, int max_bitmaps, int* n_bitmaps, uint8_t* palette)
{
	int ret;			//return code
	FFILE ifile;
	iff_bitmap_header bmheader;
	int32_t sig, form_len;
	int32_t form_type;

	*n_bitmaps = 0;

	ret = open_fake_file(ifilename, &ifile);		//read in entire file
	if (ret != IFF_NO_ERROR) goto done;

	bmheader.raw_data = NULL;

	sig = get_sig(&ifile);
	form_len = get_long(&ifile);

	if (sig != form_sig) {
		ret = IFF_NOT_IFF;
		goto done;
	}

	form_type = get_sig(&ifile);

	if ((form_type == pbm_sig) || (form_type == ilbm_sig))
		ret = IFF_FORM_BITMAP;
	else if (form_type == anim_sig) {
		int anim_end = ifile.position + form_len - 4;

		while (ifile.position < anim_end && *n_bitmaps < max_bitmaps) {

			grs_bitmap* prev_bm;

			prev_bm = *n_bitmaps > 0 ? bm_list[*n_bitmaps - 1] : NULL;

			MALLOC(bm_list[*n_bitmaps] , grs_bitmap, 1 );
			bm_list[*n_bitmaps]->bm_data = NULL;

			ret = iff_parse_bitmap(&ifile, bm_list[*n_bitmaps], form_type, (int8_t*)(*n_bitmaps > 0 ? NULL : palette), prev_bm);

			if (ret != IFF_NO_ERROR)
				goto done;

			(*n_bitmaps)++;
		}

		if (ifile.position < anim_end)	//ran out of room
			ret = IFF_TOO_MANY_BMS;

	}
	else
		ret = IFF_UNKNOWN_FORM;

done:

	close_fake_file(&ifile);

	return ret;

}

//text for error messges
char error_messages[] = {
	"No error.\0"
	"Not enough mem for loading or processing bitmap.\0"
	"IFF file has unknown FORM type.\0"
	"Not an IFF file.\0"
	"Cannot open file.\0"
	"Tried to save invalid type, like BM_RGB15.\0"
	"Bad data in file.\0"
	"ANIM file cannot be loaded with normal bitmap loader.\0"
	"Normal bitmap file cannot be loaded with anim loader.\0"
	"Array not big enough on anim brush read.\0"
	"Unknown mask type in bitmap header.\0"
	"Error reading file.\0"
};


//function to return pointer to error message
char* iff_errormsg(int error_number)
{
	char* p = error_messages;

	while (error_number--) {

		if (!p) return NULL;

		p += strlen(p) + 1;

	}

	return p;

}
