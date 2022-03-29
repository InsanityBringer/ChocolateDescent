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

#ifdef EDITOR

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "cfile/cfile.h"
#include "platform/mono.h"
#include "platform/key.h"
#include "2d/gr.h"

#include "main_d2/bm.h"			// for MAX_TEXTURES

#include "main_d2/inferno.h"
#include "main_d2/segment.h"
#include "editor.h"
#include "misc/error.h"
#include "main_d2/textures.h"
#include "main_d2/object.h"

#include "main_d2/gamemine.h"
#include "main_d2/gameseg.h"

#include "ui/ui.h"			// Because texpage.h need UI_WINDOW type
#include "texpage.h"		// For texpage_goto_first
			 
#include "medwall.h"
#include "main_d2/switch.h"

#include "main_d2/fuelcen.h"



#define REMOVE_EXT(s)  (*(strchr( (s), '.' ))='\0')

int CreateDefaultNewSegment();
int save_mine_data_compiled_new(FILE* SaveFile);
int save_mine_data(FILE* SaveFile);

static char	 current_tmap_list[MAX_TEXTURES][13];

void M_WriteByte(FILE* fp, uint8_t b)
{
	fwrite(&b, sizeof(uint8_t), 1, fp);
}

void M_WriteShort(FILE* fp, short s)
{
	uint8_t b1 = s & 255;
	uint8_t b2 = (s >> 8) & 255;
	fwrite(&b1, sizeof(uint8_t), 1, fp);
	fwrite(&b2, sizeof(uint8_t), 1, fp);
}

void M_WriteInt(FILE* fp, int i)
{
	uint8_t b1 = i & 255;
	uint8_t b2 = (i >> 8) & 255;
	uint8_t b3 = (i >> 16) & 255;
	uint8_t b4 = (i >> 24) & 255;
	fwrite(&b1, sizeof(uint8_t), 1, fp);
	fwrite(&b2, sizeof(uint8_t), 1, fp);
	fwrite(&b3, sizeof(uint8_t), 1, fp);
	fwrite(&b4, sizeof(uint8_t), 1, fp);
}

static void m_write_vector(vms_vector* v, FILE* file)
{
	file_write_int(file, v->x);
	file_write_int(file, v->y);
	file_write_int(file, v->z);
}

static void m_write_matrix(vms_matrix* m, FILE* file)
{
	m_write_vector(&m->rvec, file);
	m_write_vector(&m->uvec, file);
	m_write_vector(&m->fvec, file);
}

//[ISB] segment structure is unaligned so this is needed
void save_v16_side(side* sidep, FILE* LoadFile)
{
	int i;
	M_WriteByte(LoadFile, sidep->type);
	M_WriteByte(LoadFile, sidep->pad);
	M_WriteShort(LoadFile, sidep->wall_num);
	M_WriteShort(LoadFile, sidep->tmap_num);
	M_WriteShort(LoadFile, sidep->tmap_num2);
	for (i = 0; i < 4; i++)
	{
		M_WriteInt(LoadFile, sidep->uvls[i].u);
		M_WriteInt(LoadFile, sidep->uvls[i].v);
		M_WriteInt(LoadFile, sidep->uvls[i].l);
	}
	for (i = 0; i < 2; i++)
	{
		M_WriteInt(LoadFile, sidep->normals[i].x);
		M_WriteInt(LoadFile, sidep->normals[i].y);
		M_WriteInt(LoadFile, sidep->normals[i].z);
	}
}

void save_v16_segment(segment* segp, FILE* LoadFile)
{
	int i;
	M_WriteShort(LoadFile, segp->segnum);
	for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
	{
		save_v16_side(&segp->sides[i], LoadFile);
	}
	for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
	{
		M_WriteShort(LoadFile, segp->children[i]);
	}
	for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
	{
		M_WriteShort(LoadFile, segp->verts[i]);
	}
	M_WriteShort(LoadFile, segp->group);
	M_WriteShort(LoadFile, segp->objects);
	M_WriteByte(LoadFile, Segment2s[segp->segnum].special);
	M_WriteByte(LoadFile, Segment2s[segp->segnum].matcen_num);
	M_WriteShort(LoadFile, Segment2s[segp->segnum].value);
	M_WriteInt(LoadFile, Segment2s[segp->segnum].static_light);
}


// -----------------------------------------------------------------------------
// Save mine will:
// 1. Write file info, header info, editor info, vertex data, segment data,
//    and new_segment in that order, marking their file offset.
// 2. Go through all the fields and fill in the offset, size, and sizeof
//    values in the headers.

int med_save_mine(char * filename)
{
	FILE * SaveFile;
	char ErrorMessage[256];

	SaveFile = fopen( filename, "wb" );
	if (!SaveFile)
	{
		char fname[256];
		_splitpath( filename, NULL, NULL, fname, NULL );

		sprintf( ErrorMessage, \
			"ERROR: Cannot write to '%s'.\nYou probably need to check out a locked\nversion of the file. You should save\nthis under a different filename, and then\ncheck out a locked copy by typing\n\'co -l %s.lvl'\nat the DOS prompt.\n" 
			, filename, fname, fname );
		sprintf( ErrorMessage, "ERROR: Unable to open %s\n", filename );
		MessageBox( -2, -2, 1, ErrorMessage, "Ok" );
		return 1;
	}

	save_mine_data(SaveFile);
	
	//==================== CLOSE THE FILE =============================
	fclose(SaveFile);

	return 0;

}

#define SEGMENT_SIZEOF sizeof(segment)

//I don't actually need to do this, since it's aligned, but I'm a masochist I guess :D
void write_mine_fileinfo(FILE* fp)
{
	file_write_short(fp, mine_fileinfo.fileinfo_signature);
	file_write_short(fp, mine_fileinfo.fileinfo_version);
	file_write_int(fp, mine_fileinfo.fileinfo_sizeof);
	file_write_int(fp, mine_fileinfo.header_offset);          // Stuff common to game & editor
	file_write_int(fp, mine_fileinfo.header_size);
	file_write_int(fp, mine_fileinfo.editor_offset);   // Editor specific stuff
	file_write_int(fp, mine_fileinfo.editor_size);
	file_write_int(fp, mine_fileinfo.segment_offset);
	file_write_int(fp, mine_fileinfo.segment_howmany);
	file_write_int(fp, mine_fileinfo.segment_sizeof);
	file_write_int(fp, mine_fileinfo.newseg_verts_offset);
	file_write_int(fp, mine_fileinfo.newseg_verts_howmany);
	file_write_int(fp, mine_fileinfo.newseg_verts_sizeof);
	file_write_int(fp, mine_fileinfo.group_offset);
	file_write_int(fp, mine_fileinfo.group_howmany);
	file_write_int(fp, mine_fileinfo.group_sizeof);
	file_write_int(fp, mine_fileinfo.vertex_offset);
	file_write_int(fp, mine_fileinfo.vertex_howmany);
	file_write_int(fp, mine_fileinfo.vertex_sizeof);
	file_write_int(fp, mine_fileinfo.texture_offset);
	file_write_int(fp, mine_fileinfo.texture_howmany);
	file_write_int(fp, mine_fileinfo.texture_sizeof);
	file_write_int(fp, mine_fileinfo.walls_offset);
	file_write_int(fp, mine_fileinfo.walls_howmany);
	file_write_int(fp, mine_fileinfo.walls_sizeof);
	file_write_int(fp, mine_fileinfo.triggers_offset);
	file_write_int(fp, mine_fileinfo.triggers_howmany);
	file_write_int(fp, mine_fileinfo.triggers_sizeof);
	file_write_int(fp, mine_fileinfo.links_offset);
	file_write_int(fp, mine_fileinfo.links_howmany);
	file_write_int(fp, mine_fileinfo.links_sizeof);
	file_write_int(fp, mine_fileinfo.object_offset);				// Object info
	file_write_int(fp, mine_fileinfo.object_howmany);
	file_write_int(fp, mine_fileinfo.object_sizeof);
	file_write_int(fp, mine_fileinfo.unused_offset);			//was: doors_offset
	file_write_int(fp, mine_fileinfo.unused_howmamy);		//was: doors_howmany
	file_write_int(fp, mine_fileinfo.unused_sizeof);			//was: doors_sizeof
	file_write_short(fp, mine_fileinfo.level_shake_frequency);
	file_write_short(fp, mine_fileinfo.level_shake_duration);

	file_write_int(fp, Secret_return_segment);
	m_write_matrix(&mine_fileinfo.secret_return_orient, fp);

	file_write_int(fp, mine_fileinfo.dl_indices_offset);
	file_write_int(fp, mine_fileinfo.dl_indices_howmany);
	file_write_int(fp, mine_fileinfo.dl_indices_sizeof);

	file_write_int(fp, mine_fileinfo.delta_light_offset);
	file_write_int(fp, mine_fileinfo.delta_light_howmany);
	file_write_int(fp, mine_fileinfo.delta_light_sizeof);

	file_write_int(fp, mine_fileinfo.segment2_offset);
	file_write_int(fp, mine_fileinfo.segment2_howmany);
	file_write_int(fp, mine_fileinfo.segment2_sizeof);
}

// -----------------------------------------------------------------------------
// saves to an already-open file
int save_mine_data(FILE * SaveFile)
{
	int  header_offset, editor_offset, vertex_offset, segment_offset, doors_offset, texture_offset, walls_offset, triggers_offset, dl_offset, dlindex_offset, segment2s_offset; //, links_offset;
	int  newseg_verts_offset;
	int  newsegment_offset;
	int  i;

	med_compress_mine();
	warn_if_concave_segments();
	
	for (i=0;i<NumTextures;i++)
		strncpy(current_tmap_list[i], TmapInfo[i].filename, 13);

	//=================== Calculate offsets into file ==================

	header_offset = ftell(SaveFile) + MFI_SIZEOF;
	editor_offset = header_offset + MH_SIZEOF;
	texture_offset = editor_offset + ME_SIZEOF;
	vertex_offset  = texture_offset + (13*NumTextures);
	segment_offset = vertex_offset + (sizeof(vms_vector)*Num_vertices);
	segment2s_offset = segment_offset + (SEGMENT_SIZEOF * Num_segments);
	newsegment_offset = segment2s_offset + (sizeof(segment2)*Num_segments);
	newseg_verts_offset = newsegment_offset + SEGMENT_SIZEOF;
	walls_offset = newseg_verts_offset + (sizeof(vms_vector)*8);
	triggers_offset =	walls_offset + (sizeof(wall)*Num_walls);
	doors_offset = triggers_offset + (sizeof(trigger)*Num_triggers);
	//dlindex_offset = doors_offset + (sizeof(dl_index)*Num_dl
	

	//===================== SAVE FILE INFO ========================

	mine_fileinfo.fileinfo_signature=	0x2884;
	mine_fileinfo.fileinfo_version  =   MINE_VERSION;
	mine_fileinfo.fileinfo_sizeof   =   sizeof(mine_fileinfo);
	mine_fileinfo.header_offset     =   header_offset;
	mine_fileinfo.header_size       =   sizeof(mine_header);
	mine_fileinfo.editor_offset     =   editor_offset;
	mine_fileinfo.editor_size       =   sizeof(mine_editor);
	mine_fileinfo.vertex_offset     =   vertex_offset;
	mine_fileinfo.vertex_howmany    =   Num_vertices;
	mine_fileinfo.vertex_sizeof     =   12;
	mine_fileinfo.segment_offset    =   segment_offset;
	mine_fileinfo.segment_howmany   =   Num_segments;
	mine_fileinfo.segment_sizeof    =   SEGMENT_SIZEOF;
	mine_fileinfo.newseg_verts_offset     =   newseg_verts_offset;
	mine_fileinfo.newseg_verts_howmany    =   8;
	mine_fileinfo.newseg_verts_sizeof     =   sizeof(vms_vector);
	mine_fileinfo.texture_offset    =   texture_offset;
	mine_fileinfo.texture_howmany   =   NumTextures;
	mine_fileinfo.texture_sizeof    =   13;  // num characters in a name
	mine_fileinfo.walls_offset		  =	walls_offset;
	mine_fileinfo.walls_howmany	  =	Num_walls;
	mine_fileinfo.walls_sizeof		  =	sizeof(wall);  
	mine_fileinfo.triggers_offset	  =	triggers_offset;
	mine_fileinfo.triggers_howmany  =	Num_triggers;
	mine_fileinfo.triggers_sizeof	  =	sizeof(trigger);  
	//[ISB] Don't actually need to write this, it's read by the mine reader but the gamesave reader is what actually cares about it. 
	//mine_fileinfo.dl_indices_offset = dlindex_offset;

	//This stuff is used, though (rip seismic disturbances)
	mine_fileinfo.level_shake_frequency = Level_shake_frequency >> 12;
	mine_fileinfo.level_shake_duration = Level_shake_duration >> 12;
	mine_fileinfo.secret_return_segment = Secret_return_segment;
	mine_fileinfo.secret_return_orient = Secret_return_orient;
	mine_fileinfo.segment2_offset = segment2s_offset;
	mine_fileinfo.segment2_sizeof = sizeof(segment2);
	mine_fileinfo.segment2_howmany = Num_segments;

	// Write the fileinfo
	write_mine_fileinfo(SaveFile);

	//===================== SAVE HEADER INFO ========================

	mine_header.num_vertices        =   Num_vertices;
	mine_header.num_segments        =   Num_segments;

	// Write the editor info
	if (header_offset != ftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );

	file_write_int(SaveFile, mine_header.num_vertices);
	file_write_int(SaveFile, mine_header.num_segments);

	//===================== SAVE EDITOR INFO ==========================
	mine_editor.current_seg         =   Cursegp - Segments;
	mine_editor.newsegment_offset   =   newsegment_offset; 
	mine_editor.newsegment_size     =   SEGMENT_SIZEOF;

	// Next 3 vars added 10/07 by JAS
	mine_editor.Curside             =   Curside;
	if (Markedsegp)
		mine_editor.Markedsegp       =   Markedsegp - Segments;
	else									  
		mine_editor.Markedsegp       =   -1;
	mine_editor.Markedside          =   Markedside;
	for (i=0;i<10;i++)
		mine_editor.Groupsegp[i]	  =	Groupsegp[i] - Segments;
	for (i=0;i<10;i++)
		mine_editor.Groupside[i]     =	Groupside[i];

	if (editor_offset != ftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	fwrite( &mine_editor, sizeof(mine_editor), 1, SaveFile );

	//===================== SAVE TEXTURE INFO ==========================

	if (texture_offset != ftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	fwrite( current_tmap_list, 13, NumTextures, SaveFile );
	
	//===================== SAVE VERTEX INFO ==========================

	if (vertex_offset != ftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	for (i = 0; i < Num_vertices; i++)
	{
		M_WriteInt(SaveFile, Vertices[i].x);
		M_WriteInt(SaveFile, Vertices[i].y);
		M_WriteInt(SaveFile, Vertices[i].z);
	}
	//cfwrite( Vertices, sizeof(vms_vector), Num_vertices, SaveFile );

	//===================== SAVE SEGMENT INFO =========================

	if (segment_offset != ftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	//for (i = 0; i < Num_segments; i++)
	//	save_v16_segment(&Segments[i], SaveFile);

	//V20 saves raw segments
	fwrite( Segments, sizeof(segment), Num_segments, SaveFile );

	//===================== SAVE SEGMENT2 INFO =========================
	if (segment2s_offset != ftell(SaveFile))
		Error("OFFSETS WRONG IN MINE.C!");

	fwrite(Segment2s, sizeof(segment2), Num_segments, SaveFile);

	//===================== SAVE NEWSEGMENT INFO ======================

	if (newsegment_offset != ftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	//save_v16_segment(&New_segment, SaveFile);
	fwrite( &New_segment, sizeof(segment), 1, SaveFile );

	if (newseg_verts_offset != ftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	for (i = 0; i < 8; i++)
	{
		M_WriteInt(SaveFile, Vertices[New_segment.verts[i]].x);
		M_WriteInt(SaveFile, Vertices[New_segment.verts[i]].y);
		M_WriteInt(SaveFile, Vertices[New_segment.verts[i]].z);
	}
	//cfwrite( &Vertices[New_segment.verts[0]], sizeof(vms_vector), 8, SaveFile );

	//==================== CLOSE THE FILE =============================

	return 0;

}



#define COMPILED_MINE_VERSION 0

void dump_fix_as_short( fix value, int nbits, FILE * SaveFile )
{
	int int_value = (int)(value>>nbits);
	short short_value;

	if( int_value > 0x7fff )
	{
		short_value = 0x7fff;
		mprintf((1, "Warning: Fix (%8x) won't fit in short.  Saturating to %8x.\n", int_value, short_value<<nbits));
	}
	else if( int_value < -0x7fff ) {
		short_value = -0x7fff;
		mprintf((1, "Warning: Fix (%8x) won't fit in short.  Saturating to %8x.\n", int_value, short_value<<nbits));
	}
	else
		short_value = (short)int_value;

	file_write_short(SaveFile, short_value);
}

//version of dump for unsigned values
void dump_fix_as_ushort( fix value, int nbits, FILE * SaveFile )
{
	uint32_t int_value;
	uint16_t short_value;

	if (value < 0) 
	{
		mprintf((1, "Warning: fix (%8x) is signed...setting to zero.\n", value));
		Int3();		//hey---show this to Matt
		value = 0;
	}
	else
		int_value = value >> nbits;

	if( int_value > 0xffff ) 
	{
		short_value = 0xffff;
		mprintf((1, "Warning: Fix (%8x) won't fit in unsigned short.  Saturating to %8x.\n", int_value, short_value<<nbits));
	}
	else
		short_value = int_value;

	file_write_short(SaveFile, short_value);
}

int	New_file_format_save = 1;

// -----------------------------------------------------------------------------
// saves compiled mine data to an already-open file...
int save_mine_data_compiled(FILE * SaveFile)
{
	short i,segnum,sidenum;
	uint8_t version = COMPILED_MINE_VERSION;

#ifndef SHAREWARE
	if (New_file_format_save)
		return save_mine_data_compiled_new(SaveFile);
#endif

	med_compress_mine();
	warn_if_concave_segments();

	if (Highest_segment_index >= MAX_SEGMENTS)
	{
		char	message[128];
		sprintf(message, "Error: Too many segments (%i > %i) for game (not editor)", Highest_segment_index+1, MAX_SEGMENTS);
		MessageBox( -2, -2, 1, message, "Ok" );
	}

	if (Highest_vertex_index >= MAX_SEGMENT_VERTICES)
	{
		char	message[128];
		sprintf(message, "Error: Too many vertices (%i > %i) for game (not editor)", Highest_vertex_index+1, MAX_SEGMENT_VERTICES);
		MessageBox( -2, -2, 1, message, "Ok" );
	}

	//=============================== Writing part ==============================
	fwrite( &version, sizeof(uint8_t), 1, SaveFile );						// 1 byte = compiled version
	fwrite( &Num_vertices, sizeof(int), 1, SaveFile );					// 4 bytes = Num_vertices
	fwrite( &Num_segments, sizeof(int), 1, SaveFile );						// 4 bytes = Num_segments
	fwrite( Vertices, sizeof(vms_vector), Num_vertices, SaveFile );

	for (segnum=0; segnum<Num_segments; segnum++ )	
	{
		// Write short Segments[segnum].children[MAX_SIDES_PER_SEGMENT]
 		fwrite( &Segments[segnum].children, sizeof(short), MAX_SIDES_PER_SEGMENT, SaveFile );
		// Write short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
		fwrite( &Segments[segnum].verts, sizeof(short), MAX_VERTICES_PER_SEGMENT, SaveFile );
		// Write ubyte	Segments[segnum].special
		fwrite( &Segment2s[segnum].special, sizeof(uint8_t), 1, SaveFile );
		// Write byte	Segments[segnum].matcen_num
		fwrite( &Segment2s[segnum].matcen_num, sizeof(uint8_t), 1, SaveFile );
		// Write short	Segments[segnum].value
		fwrite( &Segment2s[segnum].value, sizeof(short), 1, SaveFile );
		// Write fix	Segments[segnum].static_light (shift down 5 bits, write as short)
		dump_fix_as_ushort( Segment2s[segnum].static_light, 4, SaveFile );
		//cfwrite( &Segments[segnum].static_light , sizeof(fix), 1, SaveFile );
	
		// Write the walls as a 6 byte array
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	
		{
			uint32_t wallnum;
			uint8_t byte_wallnum;
			if (Segments[segnum].sides[sidenum].wall_num<0)
				wallnum = 255;		// Use 255 to mark no walls
			else 
			{
				wallnum = Segments[segnum].sides[sidenum].wall_num;
				Assert( wallnum < 255 );		// Get John or Mike.. can only store up to 255 walls!!! 
			}
			byte_wallnum = (uint8_t)wallnum;
			fwrite( &byte_wallnum, sizeof(uint8_t), 1, SaveFile );
		}

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	
		{
			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	
			{
				// Write short Segments[segnum].sides[sidenum].tmap_num;
				fwrite( &Segments[segnum].sides[sidenum].tmap_num, sizeof(short), 1, SaveFile );
				// Write short Segments[segnum].sides[sidenum].tmap_num2;
				fwrite( &Segments[segnum].sides[sidenum].tmap_num2, sizeof(short), 1, SaveFile );
				// Write uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i=0; i<4; i++ )	
				{
					dump_fix_as_short( Segments[segnum].sides[sidenum].uvls[i].u, 5, SaveFile );
					dump_fix_as_short( Segments[segnum].sides[sidenum].uvls[i].v, 5, SaveFile );
					dump_fix_as_ushort( Segments[segnum].sides[sidenum].uvls[i].l, 1, SaveFile );
					//cfwrite( &Segments[segnum].sides[sidenum].uvls[i].l, sizeof(fix), 1, SaveFile );
				}	
			}
		}

	}

	return 0;
}

// -----------------------------------------------------------------------------
// saves compiled mine data to an already-open file...
int save_mine_data_compiled_new(FILE * SaveFile)
{
	short		i, segnum, sidenum, temp_short;
	uint8_t 	version = COMPILED_MINE_VERSION;
	uint8_t		bit_mask = 0;

	med_compress_mine();
	warn_if_concave_segments();

	if (Highest_segment_index >= MAX_SEGMENTS) 
	{
		char	message[128];
		sprintf(message, "Error: Too many segments (%i > %i) for game (not editor)", Highest_segment_index+1, MAX_SEGMENTS);
		MessageBox( -2, -2, 1, message, "Ok" );
	}

	if (Highest_vertex_index >= MAX_SEGMENT_VERTICES)
	{
		char	message[128];
		sprintf(message, "Error: Too many vertices (%i > %i) for game (not editor)", Highest_vertex_index+1, MAX_SEGMENT_VERTICES);
		MessageBox( -2, -2, 1, message, "Ok" );
	}

	//=============================== Writing part ==============================
	file_write_byte(SaveFile, version);						// 1 byte = compiled version
	temp_short = Num_vertices;
	file_write_short(SaveFile, temp_short);					// 2 bytes = Num_vertices
	temp_short = Num_segments;
	file_write_short(SaveFile, temp_short);					// 2 bytes = Num_segments

	for (i = 0; i < Num_vertices; i++)
	{
		file_write_int(SaveFile, Vertices[i].x);
		file_write_int(SaveFile, Vertices[i].y);
		file_write_int(SaveFile, Vertices[i].z);
	}

	for (segnum=0; segnum<Num_segments; segnum++ )	
	{
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++)
		{
 			if (Segments[segnum].children[sidenum] != -1)
				bit_mask |= (1 << sidenum);
		}

		if ((Segment2s[segnum].special != 0) || (Segment2s[segnum].matcen_num != 0) || (Segment2s[segnum].value != 0))
			bit_mask |= (1 << MAX_SIDES_PER_SEGMENT);

		file_write_byte(SaveFile, bit_mask);

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++)
		{
			if (bit_mask & (1 << sidenum))
				file_write_short(SaveFile, Segments[segnum].children[sidenum]);
		}

		for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
		{
			file_write_short(SaveFile, Segments[segnum].verts[i]);
		}
	
		// Write the walls as a 6 byte array
		bit_mask = 0;
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	
		{
			uint32_t wallnum;
			if (Segments[segnum].sides[sidenum].wall_num >= 0) 
			{
				bit_mask |= (1 << sidenum);
				wallnum = Segments[segnum].sides[sidenum].wall_num;
				Assert( wallnum < 255 );		// Get John or Mike.. can only store up to 255 walls!!! 
			}
		}
		file_write_byte(SaveFile, bit_mask);

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	
		{
			if (bit_mask & (1 << sidenum))
				file_write_byte(SaveFile, Segments[segnum].sides[sidenum].wall_num);
		}

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	
		{
			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	
			{
				uint16_t	tmap_num, tmap_num2;

				tmap_num = Segments[segnum].sides[sidenum].tmap_num;
				tmap_num2 = Segments[segnum].sides[sidenum].tmap_num2;
				if (tmap_num2 != 0)
					tmap_num |= 0x8000;

				file_write_short(SaveFile, tmap_num);
				if (tmap_num2 != 0)
					file_write_short(SaveFile, tmap_num2);

				for (i=0; i<4; i++ )	
				{
					dump_fix_as_short( Segments[segnum].sides[sidenum].uvls[i].u, 5, SaveFile );
					dump_fix_as_short( Segments[segnum].sides[sidenum].uvls[i].v, 5, SaveFile );
					dump_fix_as_ushort( Segments[segnum].sides[sidenum].uvls[i].l, 1, SaveFile );
				}	
			}
		}

	}

	//Write segments2
	for (i = 0; i < Num_segments; i++)
	{
		file_write_byte(SaveFile, Segment2s[i].special);
		file_write_byte(SaveFile, Segment2s[i].matcen_num);
		file_write_byte(SaveFile, Segment2s[i].value);
		file_write_byte(SaveFile, Segment2s[i].s2_flags);
		file_write_int(SaveFile, Segment2s[i].static_light);
	}

	return 0;
}

#endif
