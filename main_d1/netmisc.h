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

 //Returns a checksum of a block of memory.
extern uint16_t netmisc_calc_checksum(void* vptr, int len);

//Functions for encoding values into a block of memory.
//All values are written little-endian indepenedent of alignment. 
void netmisc_encode_int8(uint8_t* ptr, int* offset, uint8_t v);
void netmisc_encode_int16(uint8_t* ptr, int* offset, short v);
void netmisc_encode_int32(uint8_t* ptr, int* offset, int v);
void netmisc_encode_shortpos(uint8_t* ptr, int* offset, shortpos* v);
void netmisc_encode_vector(uint8_t* ptr, int* offset, vms_vector* vec);

//Functions for extracting values from a block of memory, as encoded by the above functions.
void netmisc_decode_int8(uint8_t* ptr, int* offset, uint8_t* v);
void netmisc_decode_int16(uint8_t* ptr, int* offset, short* v);
void netmisc_decode_int32(uint8_t* ptr, int* offset, int* v);
void netmisc_decode_shortpos(uint8_t* ptr, int* offset, shortpos* v);
void netmisc_decode_vector(uint8_t* ptr, int* offset, vms_vector* vec);
