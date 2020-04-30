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

// Warning( "MEM: Too many malloc's!" );
// Warning( "MEM: Malloc returnd an already alloced block!" );
// Warning( "MEM: Malloc Failed!" );
// Warning( "MEM: Freeing the NULL pointer!" );
// Warning( "MEM: Freeing a non-malloced pointer!" );
// Warning( "MEM: %d/%d check bytes were overwritten at the end of %8x", ec, CHECKSIZE, buffer  );
// Warning( "MEM: %d blocks were left allocated!", numleft );

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>

#include "platform/mono.h"
#include "misc/error.h"

//#define FULL_MEM_CHECKING

#ifdef FULL_MEM_CHECKING 

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

#define MAX_INDEX 10000

static void* MallocBase[MAX_INDEX];
static unsigned int MallocSize[MAX_INDEX];
static unsigned int MallocRealSize[MAX_INDEX];
static unsigned char Present[MAX_INDEX];
static char* Filename[MAX_INDEX];
static char* Varname[MAX_INDEX];
static int Line[MAX_INDEX];
static uint64_t BytesMalloced = 0;

int show_mem_info = 1;

static int free_list[MAX_INDEX];

static int num_blocks = 0;

static int Initialized = 0;

static int LargestIndex = 0;

int out_of_memory = 0;

void mem_display_blocks();

void mem_init()
{
	int i;

	Initialized = 1;

	for (i = 0; i < MAX_INDEX; i++)
	{
		free_list[i] = i;
		MallocBase[i] = NULL;
		MallocSize[i] = 0;
		MallocRealSize[i] = 0;
		Present[i] = 0;
		Filename[i] = NULL;
		Varname[i] = NULL;
		Line[i] = 0;
	}

	num_blocks = 0;
	LargestIndex = 0;

	atexit(mem_display_blocks);

}

void PrintInfo(int id)
{
	fprintf(stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], Line[id]);
}


void* mem_malloc(unsigned int size, const char* var, const char* filename, int line, int fill_zero)
{
	int i, j, id;
	void* ptr;
	unsigned char* pc;
	int* data;

	if (Initialized == 0)
		mem_init();

	if (num_blocks >= MAX_INDEX)
	{
		fprintf(stderr, "\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n");
		fprintf(stderr, "\tBlock '%s' created in %s, line %d.\n", var, filename, line);
		Error("MEM_OUT_OF_SLOTS");
		return nullptr;
	}

	id = free_list[num_blocks++];

	if (id > LargestIndex) LargestIndex = id;

	if (id == -1)
	{
		fprintf(stderr, "\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n");
		fprintf(stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], Line[id]);
		Error("MEM_OUT_OF_SLOTS");
		return nullptr;
	}

	ptr = malloc(size + (CHECKSIZE * 2));

	/*
	for (j=0; j<=LargestIndex; j++ )
	{
		if (Present[j] && MallocBase[j] == (unsigned int)ptr )
		{
			fprintf( stderr,"\nMEM_SPACE_USED: Malloc returned a block that is already marked as preset.\n" );
			fprintf( stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], Line[id] );
			Warning( "MEM_SPACE_USED" );
			Int3();
			}
	}
	*/

	if (ptr == NULL)
	{
		out_of_memory = 1;
		fprintf(stderr, "\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n");
		fprintf(stderr, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], Line[id]);
		Error("MEM_OUT_OF_MEMORY");
		Int3();
		return nullptr;
	}

	MallocBase[id] = (void*)ptr;
	//data = (int*)((int)MallocBase[id] - sizeof(int));
	//MallocRealSize[id] = *data; //[ISB] this has to be some sort of HORRIBLE fucking nonportable shit holy crap
	MallocSize[id] = size;
	Varname[id] = (char*)var;
	Filename[id] = (char*)filename;
	Line[id] = line;
	Present[id] = 1;

	pc = (unsigned char*)ptr;

	BytesMalloced += size;

	for (i = 0; i < CHECKSIZE; i++)
	{
		pc[i] = CHECKBYTE;
		pc[size + CHECKSIZE + i] = CHECKBYTE;
	}

	if (fill_zero)
		memset((uint8_t*)ptr+CHECKSIZE, 0, size);

	pc += CHECKSIZE;

	return (void*)pc;

}

int mem_find_id(void* buffer)
{
	int i;

	for (i = 0; i <= LargestIndex; i++)
		if (Present[i] == 1)
			if (MallocBase[i] == buffer)
				return i;

	// Didn't find id.
	return -1;
}

int mem_check_integrity(int block_number)
{
	int i, ErrorCount;
	unsigned char* CheckData; //[ISB] UNSIGNED IS NOT IMPLICIT GOD DAMN IT

	ErrorCount = 0;
	CheckData = (unsigned char*)((unsigned char*)MallocBase[block_number]);
	for (i = 0; i < CHECKSIZE; i++)
		if (CheckData[i] != CHECKBYTE)
		{
			ErrorCount++;
			fprintf(stderr, "OA: %p ", &CheckData[i]);
		}

	if (ErrorCount && (!out_of_memory))
	{
		fprintf(stderr, "MEM_UNDERWRITE: Memory before the allocated block overwritten.\n");
		PrintInfo(block_number);
		fprintf(stderr, "\t%d/%d check bytes were overwritten.\n", ErrorCount, CHECKSIZE);
	}

	CheckData = (unsigned char*)((unsigned char*)MallocBase[block_number] + MallocSize[block_number] + CHECKSIZE);

	ErrorCount = 0;

	for (i = 0; i < CHECKSIZE; i++)
		if (CheckData[i] != CHECKBYTE) 
		{
			ErrorCount++;
			fprintf(stderr, "OA: %p ", &CheckData[i]);
		}

	if (ErrorCount && (!out_of_memory)) 
	{
		fprintf(stderr, "\nMEM_OVERWRITE: Memory after the end of allocated block overwritten.\n");
		PrintInfo(block_number);
		fprintf(stderr, "\t%d/%d check bytes were overwritten.\n", ErrorCount, CHECKSIZE);
	}

	return ErrorCount;

}

void mem_free(void* buffer)
{
	int id;
	unsigned char* pc = (unsigned char*)buffer;
	pc -= CHECKSIZE;

	if (Initialized == 0)
		mem_init();

	if (buffer == NULL && (!out_of_memory))
	{
		fprintf(stderr, "\nMEM_FREE_NULL: An attempt was made to free the null pointer.\n");
		Warning("MEM: Freeing the NULL pointer!");
		Int3();
		return;
	}

	//id = mem_find_id(buffer);
	id = mem_find_id((void*)pc);

	if (id == -1 && (!out_of_memory))
	{
		fprintf(stderr, "\nMEM_FREE_NOMALLOC: An attempt was made to free a ptr that wasn't\nallocated with mem.h included.\n");
		Warning("MEM: Freeing a non-malloced pointer!");
		Int3();
		return;
	}

	mem_check_integrity(id);

	BytesMalloced -= MallocSize[id];

	//free(buffer);
	free(pc);

	Present[id] = 0;
	MallocBase[id] = 0;
	MallocSize[id] = 0;

	free_list[--num_blocks] = id;
}

void mem_display_blocks()
{
	int i, numleft;

	if (Initialized == 0) return;

	numleft = 0;
	for (i = 0; i <= LargestIndex; i++)
	{
		if (Present[i] == 1 && (!out_of_memory))
		{
			numleft++;
			if (show_mem_info) {
				fprintf(stderr, "\nMEM_LEAKAGE: Memory block has not been freed.\n");
				PrintInfo(i);
			}
			mem_free((void*)((unsigned char*)MallocBase[i] + CHECKSIZE));
		}
	}

	if (numleft && (!out_of_memory))
	{
		Warning("MEM: %d blocks were left allocated!", numleft);
	}
}

void mem_validate_heap()
{
	int i;

	for (i = 0; i < LargestIndex; i++)
		if (Present[i] == 1)
			mem_check_integrity(i);
}

void mem_print_all()
{
	FILE* ef;
	int i, size = 0;

	//ef = fopen("DESCENT.MEM", "wt");
	errno_t err = fopen_s(&ef, "DESCENT.MEM", "wt");
	if (err || ef == NULL)
	{
		fprintf(stderr, "mem_print_all: Can't open memory file\n");
		return;
	}

	for (i = 0; i < LargestIndex; i++)
		if (Present[i] == 1) 
		{
			size += MallocSize[i];
			//fprintf( ef, "Var:%s\t File:%s\t Line:%d\t Size:%d Base:%x\n", Varname[i], Filename[i], Line[i], MallocSize[i], MallocBase[i] );
			fprintf(ef, "%12d bytes in %s declared in %s, line %d\n", MallocSize[i], Varname[i], Filename[i], Line[i]);
		}
	fprintf(ef, "%d bytes (%d Kbytes) allocated.\n", size, size / 1024);
	fclose(ef);
}

#else

static int Initialized = 0;
static uint64_t BytesMalloced = 0;

void mem_display_blocks(void);

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

int show_mem_info = 0;

void mem_init()
{
	Initialized = 1;

	atexit(mem_display_blocks);
}

void* mem_malloc(unsigned int size, const char* var, const char* filename, int line, int fill_zero)
{
	unsigned int base;
	void* ptr;

	if (Initialized == 0)
		mem_init();

	//fprintf(stderr, "Allocation: Var %s, file %s, line %d. Amount %d\n", var, filename, line, size);

	if (size == 0) {
		fprintf(stderr, "\nMEM_MALLOC_ZERO: Attempting to malloc 0 bytes.\n");
		fprintf(stderr, "\tVar %s, file %s, line %d.\n", var, filename, line);
		Error("MEM_MALLOC_ZERO");
		Int3();

		return NULL;
	}

	ptr = malloc(size + CHECKSIZE);

	if (ptr == NULL) {
		fprintf(stderr, "\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n");
		fprintf(stderr, "\tVar %s, file %s, line %d.\n", var, filename, line);
		Error("MEM_OUT_OF_MEMORY");
		Int3();

		return NULL; //[ISB] solve C6011
	}

	if (fill_zero)
		memset(ptr, 0, size);

	return ptr;
}

void mem_free(void* buffer)
{
	int ErrorCount;

	if (Initialized == 0)
		mem_init();

	if (buffer == NULL)
	{
		fprintf(stderr, "\nMEM_FREE_NULL: An attempt was made to free the null pointer.\n");
		Warning("MEM: Freeing the NULL pointer!");
		Int3();
		return;
	}

	ErrorCount = 0;

	if (ErrorCount) 
	{
		fprintf(stderr, "\nMEM_OVERWRITE: Memory after the end of allocated block overwritten.\n");
		fprintf(stderr, "\tBlock at 0x%p\n", buffer); //[ISB] trying %p
		fprintf(stderr, "\t%d/%d check bytes were overwritten.\n", ErrorCount, CHECKSIZE);
	}

	free(buffer);
}

void mem_display_blocks(void)
{
	if (Initialized == 0) return;

	if (BytesMalloced != 0)
	{
		fprintf(stderr, "\nMEM_LEAKAGE: %lld bytes of memory have not been freed.\n", BytesMalloced);
	}
}

void mem_validate_heap()
{
}

void mem_print_all()
{
}

#endif


