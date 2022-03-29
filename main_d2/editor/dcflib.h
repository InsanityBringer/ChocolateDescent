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

#ifndef _DCFLIB_H
#define _DCFLIB_H

#include "stdio.h"
#include "misc/types.h"

#define DCFILE FILE

// Opens a file for reading only. Returns NULL if error.
#define dcfopen(filename) fopen((filename),"rb")

// Moves to a position in the file. Returns 0 if ok.
#define dcfseek( cfile, position ) fseek( cfile, position, SEEK_SET); 

#define dcftell( cfile ) ftell(cfile)
// Returns the current file position.

#define dcflength( cfile ) filelength(fileno(cfile))
// Returns the length of the file.

#define dcfread( cfile, data, size ) fread(data, sizeof(byte), size, cfile)
// Reads 'size' bytes of the file into 'data'. Returns 0 if ok.

#define dcfclose( cfile ) fclose( cfile )
// Closes 'cfile'. Frees all memory associated with file.

#endif
