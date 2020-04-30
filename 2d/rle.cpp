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
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "mem/mem.h"
#include "platform/mono.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "misc/error.h"
#include "2d/rle.h"

#define RLE_CODE 			0xE0
#define NOT_RLE_CODE		31

uint8_t* gr_rle_decode(uint8_t* src, uint8_t* dest)
{
	//[ISB] from mac code
	int i;
	uint8_t data, count = 0;

	while (1) {
		data = *src++;
		if ((data & RLE_CODE) != RLE_CODE) 
		{
			*dest++ = data;
		}
		else 
		{
			count = data & NOT_RLE_CODE;
			if (count == 0) return dest;
			data = *src++;
			for (i = 0; i < count; i++)
				* dest++ = data;
		}
	}
	//k
}


void rle_stosb(uint8_t* dest, int len, int color)
{
	int i;
	for (i = 0; i < len; i++)
		* dest++ = color;
}

// Given pointer to start of one scanline of rle data, uncompress it to
// dest, from source pixels x1 to x2.
void gr_rle_expand_scanline_masked(uint8_t* dest, uint8_t* src, int x1, int x2)
{
	int i = 0;
	uint8_t count;
	uint8_t color = 0; //[ISB] shut up compiler warning

	if (x2 < x1) return;

	count = 0;
	while (i < x1) {
		color = *src++;
		if (color == RLE_CODE) return;
		if ((color & RLE_CODE) == RLE_CODE) {
			count = color & (~RLE_CODE);
			color = *src++;
		}
		else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.

	if (x1 + count > x2) {
		count = x2 - x1 + 1;
		if (color != 255)	rle_stosb(dest, count, color);
		return;
	}

	if (color != 255)	rle_stosb(dest, count, color);
	dest += count;
	i += count;

	while (i <= x2) {
		color = *src++;
		if (color == RLE_CODE) return;
		if ((color & RLE_CODE) == (RLE_CODE)) {
			count = color & (~RLE_CODE);
			color = *src++;
		}
		else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if (i + count <= x2) {
			if (color != 255)rle_stosb(dest, count, color);
			i += count;
			dest += count;
		}
		else {
			count = x2 - i + 1;
			if (color != 255)rle_stosb(dest, count, color);
			i += count;
			dest += count;
		}

	}
}

void gr_rle_expand_scanline(uint8_t* dest, uint8_t* src, int x1, int x2)
{
	int i = 0;
	uint8_t count;
	uint8_t color = 0; //[ISB] shut up compiler warning

	if (x2 < x1) return;

	count = 0;
	while (i < x1) {
		color = *src++;
		if (color == RLE_CODE) return;
		if ((color & RLE_CODE) == RLE_CODE) {
			count = color & (~RLE_CODE);
			color = *src++;
		}
		else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.

	if (x1 + count > x2) {
		count = x2 - x1 + 1;
		rle_stosb(dest, count, color);
		return;
	}

	rle_stosb(dest, count, color);
	dest += count;
	i += count;

	while (i <= x2) {
		color = *src++;
		if (color == RLE_CODE) return;
		if ((color & RLE_CODE) == RLE_CODE) {
			count = color & (~RLE_CODE);
			color = *src++;
		}
		else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if (i + count <= x2) {
			rle_stosb(dest, count, color);
			i += count;
			dest += count;
		}
		else {
			count = x2 - i + 1;
			rle_stosb(dest, count, color);
			i += count;
			dest += count;
		}
	}
}


int gr_rle_encode(int org_size, uint8_t* src, uint8_t* dest)
{
	int i;
	uint8_t c, oc;
	uint8_t count;
	uint8_t* dest_start;

	dest_start = dest;
	oc = *src++;
	count = 1;

	for (i = 1; i < org_size; i++) {
		c = *src++;
		if (c != oc) {
			if (count) {
				if ((count == 1) && ((oc & RLE_CODE) != RLE_CODE)) {
					*dest++ = oc;
					Assert(oc != RLE_CODE);
				}
				else {
					count |= RLE_CODE;
					*dest++ = count;
					*dest++ = oc;
				}
			}
			oc = c;
			count = 0;
		}
		count++;
		if (count == NOT_RLE_CODE) {
			count |= RLE_CODE;
			*dest++ = count;
			*dest++ = oc;
			count = 0;
		}
	}
	if (count) {
		if ((count == 1) && ((oc & RLE_CODE) != RLE_CODE)) {
			*dest++ = oc;
			Assert(oc != RLE_CODE);
		}
		else {
			count |= RLE_CODE;
			*dest++ = count;
			*dest++ = oc;
		}
	}
	*dest++ = RLE_CODE;

	return dest - dest_start;
}


int gr_rle_getsize(int org_size, uint8_t* src)
{
	int i;
	uint8_t c, oc;
	uint8_t count;
	int dest_size = 0;

	oc = *src++;
	count = 1;

	for (i = 1; i < org_size; i++) {
		c = *src++;
		if (c != oc) {
			if (count) {
				if ((count == 1) && ((oc & RLE_CODE) != RLE_CODE)) {
					dest_size++;
				}
				else {
					dest_size++;
					dest_size++;
				}
			}
			oc = c;
			count = 0;
		}
		count++;
		if (count == NOT_RLE_CODE) {
			dest_size++;
			dest_size++;
			count = 0;
		}
	}
	if (count) {
		if ((count == 1) && ((oc & RLE_CODE) != RLE_CODE)) {
			dest_size++;
		}
		else {
			dest_size++;
			dest_size++;
		}
	}
	dest_size++;

	return dest_size;
}

int gr_bitmap_rle_compress(grs_bitmap* bmp)
{
	int y, d1, d;
	int doffset;
	uint8_t* rle_data;

	rle_data = (uint8_t*)malloc((bmp->bm_w + 1) * bmp->bm_h);
	if (rle_data == NULL) return 0;
	doffset = 4 + bmp->bm_h;
	for (y = 0; y < bmp->bm_h; y++) {
		d1 = gr_rle_getsize(bmp->bm_w, &bmp->bm_data[bmp->bm_w * y]);
		if (((doffset + d1) > bmp->bm_w * bmp->bm_h) || (d1 > 255)) {
			free(rle_data);
			return 0;
		}
		d = gr_rle_encode(bmp->bm_w, &bmp->bm_data[bmp->bm_w * y], &rle_data[doffset]);
		Assert(d == d1);
		doffset += d;
		rle_data[y + 4] = d;
	}
	//mprintf( 0, "Bitmap of size %dx%d, (%d bytes) went down to %d bytes\n", bmp->bm_w, bmp->bm_h, bmp->bm_h*bmp->bm_w, doffset );
	memcpy(rle_data, &doffset, 4);
	memcpy(bmp->bm_data, rle_data, doffset);
	free(rle_data);
	bmp->bm_flags |= BM_FLAG_RLE;
	return 1;
}

#define MAX_CACHE_BITMAPS 32

typedef struct rle_cache_element {
	grs_bitmap* rle_bitmap;
	uint8_t* rle_data;
	grs_bitmap* expanded_bitmap;
	int last_used;
} rle_cache_element;

int rle_cache_initialized = 0;
int rle_counter = 0;
int rle_next = 0;
rle_cache_element rle_cache[MAX_CACHE_BITMAPS];

int rle_hits = 0;
int rle_misses = 0;

void rle_cache_close(void)
{
	if (rle_cache_initialized) {
		int i;
		rle_cache_initialized = 0;
		for (i = 0; i < MAX_CACHE_BITMAPS; i++) {
			gr_free_bitmap(rle_cache[i].expanded_bitmap);
		}
	}
}

void rle_cache_init()
{
	int i;
	for (i = 0; i < MAX_CACHE_BITMAPS; i++) {
		rle_cache[i].rle_bitmap = NULL;
		rle_cache[i].expanded_bitmap = gr_create_bitmap(64, 64);
		rle_cache[i].last_used = 0;
		Assert(rle_cache[i].expanded_bitmap != NULL);
	}
	rle_cache_initialized = 1;
	atexit(rle_cache_close);
}

void rle_cache_flush()
{
	int i;
	for (i = 0; i < MAX_CACHE_BITMAPS; i++) {
		rle_cache[i].rle_bitmap = NULL;
		rle_cache[i].last_used = 0;
	}
}

void rle_expand_texture_sub(grs_bitmap* bmp, grs_bitmap* rle_temp_bitmap_1);

grs_bitmap* rle_expand_texture(grs_bitmap* bmp)
{
	int i;
	int lowest_count, lc;
	int least_recently_used;

	if (!rle_cache_initialized) rle_cache_init();

	Assert(!(bmp->bm_flags & BM_FLAG_PAGED_OUT));

	lc = rle_counter;
	rle_counter++;
	if (rle_counter < lc) {
		for (i = 0; i < MAX_CACHE_BITMAPS; i++) {
			rle_cache[i].rle_bitmap = NULL;
			rle_cache[i].last_used = 0;
		}
	}

	//	if (((rle_counter % 100)==1) && (rle_hits+rle_misses > 0))
	//		mprintf(( 0, "RLE-CACHE %d%%, H:%d, M:%d\n", (rle_misses*100)/(rle_hits+rle_misses), rle_hits, rle_misses ));

	lowest_count = rle_cache[rle_next].last_used;
	least_recently_used = rle_next;
	rle_next++;
	if (rle_next >= MAX_CACHE_BITMAPS)
		rle_next = 0;

	for (i = 0; i < MAX_CACHE_BITMAPS; i++) {
		if (rle_cache[i].rle_bitmap == bmp) {
			rle_hits++;
			rle_cache[i].last_used = rle_counter;
			return rle_cache[i].expanded_bitmap;
		}
		if (rle_cache[i].last_used < lowest_count) {
			lowest_count = rle_cache[i].last_used;
			least_recently_used = i;
		}
	}
	rle_misses++;
	rle_expand_texture_sub(bmp, rle_cache[least_recently_used].expanded_bitmap);
	rle_cache[least_recently_used].rle_bitmap = bmp;
	rle_cache[least_recently_used].last_used = rle_counter;
	return rle_cache[least_recently_used].expanded_bitmap;
}

void rle_expand_texture_sub(grs_bitmap* bmp, grs_bitmap* rle_temp_bitmap_1)
{
	unsigned char* dbits;
	unsigned char* sbits;
	int i;
	unsigned char* dbits1;

	sbits = &bmp->bm_data[4 + 64];
	dbits = rle_temp_bitmap_1->bm_data;

	rle_temp_bitmap_1->bm_flags = bmp->bm_flags & (~BM_FLAG_RLE);

	for (i = 0; i < 64; i++) {
		dbits1 = (unsigned char*)gr_rle_decode(sbits, dbits);
		sbits += (int)bmp->bm_data[4 + i];
		dbits += 64;
		Assert(dbits == dbits1);		// Get John, bogus rle data!
	}
}


void gr_rle_expand_scanline_generic(grs_bitmap* dest, int dx, int dy, uint8_t* src, int x1, int x2)
{
	int i = 0, j;
	int count;
	uint8_t color = 0; //[ISB] shut up compiler warning

	if (x2 < x1) return;

	count = 0;
	while (i < x1) {
		color = *src++;
		if (color == RLE_CODE) return;
		if ((color & RLE_CODE) == RLE_CODE) {
			count = color & NOT_RLE_CODE;
			color = *src++;
		}
		else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.

	if (x1 + count > x2) {
		count = x2 - x1 + 1;
		for (j = 0; j < count; j++)
			gr_bm_pixel(dest, dx++, dy, color);
		return;
	}

	for (j = 0; j < count; j++)
		gr_bm_pixel(dest, dx++, dy, color);
	i += count;

	while (i <= x2) {
		color = *src++;
		if (color == RLE_CODE) return;
		if ((color & RLE_CODE) == RLE_CODE) {
			count = color & NOT_RLE_CODE;
			color = *src++;
		}
		else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if (i + count <= x2) {
			for (j = 0; j < count; j++)
				gr_bm_pixel(dest, dx++, dy, color);
			i += count;
		}
		else {
			count = x2 - i + 1;
			for (j = 0; j < count; j++)
				gr_bm_pixel(dest, dx++, dy, color);
			i += count;
		}
	}
}

