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

extern int show_mem_info;

#ifndef NDEBUG

//extern int show_mem_info;

void* mem_display_blocks(void);
extern void* mem_malloc_(unsigned int size, const char* var, const char* file, int line, int fill_zero);
extern void mem_free_(void* buffer);

#define mem_malloc(size)    mem_malloc_((size),"Unknown", __FILE__,__LINE__, 0 )
#define mem_calloc(n,size)  mem_malloc_((n*size),"Unknown", __FILE__,__LINE__, 1 )
#define mem_free(ptr)       do{ mem_free_(ptr); ptr=NULL; } while(0)

#define MALLOC( var, type, count )   (var=(type *)mem_malloc_((count)*sizeof(type),#var, __FILE__,__LINE__,0 ))

// Checks to see if any blocks are overwritten
void mem_validate_heap();

#else

#define mem_malloc(size)	malloc(size)
#define mem_calloc(n,size)	calloc(n,size)
#define mem_free(ptr)       do{ free(ptr); ptr=NULL; } while(0)

#define MALLOC( var, type, count )   (var=(type *)malloc((count)*sizeof(type)))

#endif
