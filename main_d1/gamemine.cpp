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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "platform/mono.h"

#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "misc/error.h"
#include "gameseg.h"
#include "switch.h"

#include "game.h"
#include "newmenu.h"

#ifdef EDITOR
#include "editor\editor.h"
#endif

#include "cfile/cfile.h"		
#include "fuelcen.h"

#include "hash.h"
#include "platform/key.h"
#include "piggy.h"

#define REMOVE_EXT(s)  (*(strchr( (s), '.' ))='\0')

struct mtfi mine_top_fileinfo;    // Should be same as first two fields below...
struct mfi mine_fileinfo;
struct mh mine_header;
struct me mine_editor;

//int CreateDefaultNewSegment(); //[ISB] doesn't appear called?

#ifdef EDITOR

//[ISB] segment structure is unaligned so this is needed
void load_v16_side(side* sidep, CFILE* LoadFile)
{
	int i;
	sidep->type = CF_ReadByte(LoadFile);
	sidep->pad = CF_ReadByte(LoadFile);
	sidep->wall_num = CF_ReadShort(LoadFile);
	sidep->tmap_num = CF_ReadShort(LoadFile);
	sidep->tmap_num2 = CF_ReadShort(LoadFile);
	for (i = 0; i < 4; i++)
	{
		sidep->uvls[i].u = CF_ReadInt(LoadFile);
		sidep->uvls[i].v = CF_ReadInt(LoadFile);
		sidep->uvls[i].l = CF_ReadInt(LoadFile);
	}
	for (i = 0; i < 2; i++)
	{
		sidep->normals[i].x = CF_ReadInt(LoadFile);
		sidep->normals[i].y = CF_ReadInt(LoadFile);
		sidep->normals[i].z = CF_ReadInt(LoadFile);
	}
}

void load_v16_segment(segment* segp, CFILE* LoadFile)
{
	int i;
	segp->segnum = CF_ReadShort(LoadFile);
	for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
	{
		load_v16_side(&segp->sides[i], LoadFile);
	}
	for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++)
	{
		segp->children[i] = CF_ReadShort(LoadFile);
	}
	for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
	{
		segp->verts[i] = CF_ReadShort(LoadFile);
	}
	segp->group = CF_ReadShort(LoadFile);
	segp->objects = CF_ReadShort(LoadFile);
	segp->special = CF_ReadByte(LoadFile);
	segp->matcen_num = CF_ReadByte(LoadFile);
	segp->value = CF_ReadShort(LoadFile);
	segp->static_light = CF_ReadInt(LoadFile);
}

static char old_tmap_list[MAX_TEXTURES][13];
short tmap_xlate_table[MAX_TEXTURES];
static short tmap_times_used[MAX_TEXTURES];

// -----------------------------------------------------------------------------
//loads from an already-open file
// returns 0=everything ok, 1=old version, -1=error
int load_mine_data(CFILE* LoadFile)
{
	int   i, j;
	short tmap_xlate;
	int 	translate;
	char* temptr;
	int	mine_start = cftell(LoadFile);

	fuelcen_reset();

	for (i = 0; i < MAX_TEXTURES; i++)
		tmap_times_used[i] = 0;

#ifdef EDITOR
	// Create a new mine to initialize things.
	//texpage_goto_first();
	create_new_mine();
#endif

	//===================== READ FILE INFO ========================

	// These are the default values... version and fileinfo_sizeof
	// don't have defaults.
	mine_fileinfo.header_offset = -1;
	mine_fileinfo.header_size = sizeof(mine_header);
	mine_fileinfo.editor_offset = -1;
	mine_fileinfo.editor_size = sizeof(mine_editor);
	mine_fileinfo.vertex_offset = -1;
	mine_fileinfo.vertex_howmany = 0;
	mine_fileinfo.vertex_sizeof = sizeof(vms_vector);
	mine_fileinfo.segment_offset = -1;
	mine_fileinfo.segment_howmany = 0;
	mine_fileinfo.segment_sizeof = sizeof(segment);
	mine_fileinfo.newseg_verts_offset = -1;
	mine_fileinfo.newseg_verts_howmany = 0;
	mine_fileinfo.newseg_verts_sizeof = sizeof(vms_vector);
	mine_fileinfo.group_offset = -1;
	mine_fileinfo.group_howmany = 0;
	mine_fileinfo.group_sizeof = sizeof(group);
	mine_fileinfo.texture_offset = -1;
	mine_fileinfo.texture_howmany = 0;
	mine_fileinfo.texture_sizeof = 13;  // num characters in a name
	mine_fileinfo.walls_offset = -1;
	mine_fileinfo.walls_howmany = 0;
	mine_fileinfo.walls_sizeof = sizeof(wall);
	mine_fileinfo.triggers_offset = -1;
	mine_fileinfo.triggers_howmany = 0;
	mine_fileinfo.triggers_sizeof = sizeof(trigger);
	mine_fileinfo.object_offset = -1;
	mine_fileinfo.object_howmany = 1;
	mine_fileinfo.object_sizeof = sizeof(object);

	// Read in mine_top_fileinfo to get size of saved fileinfo.

	memset(&mine_top_fileinfo, 0, sizeof(mine_top_fileinfo));

	if (cfseek(LoadFile, mine_start, SEEK_SET))
		Error("Error moving to top of file in gamemine.c");

	if (cfread(&mine_top_fileinfo, sizeof(mine_top_fileinfo), 1, LoadFile) != 1)
		Error("Error reading mine_top_fileinfo in gamemine.c");

	if (mine_top_fileinfo.fileinfo_signature != 0x2884)
		return -1;

	// Check version number
	if (mine_top_fileinfo.fileinfo_version < COMPATIBLE_VERSION)
		return -1;

	// Now, Read in the fileinfo
	if (cfseek(LoadFile, mine_start, SEEK_SET))
		Error("Error seeking to top of file in gamemine.c");

	if (cfread(&mine_fileinfo, mine_top_fileinfo.fileinfo_sizeof, 1, LoadFile) != 1)
		Error("Error reading mine_fileinfo in gamemine.c");

	//===================== READ HEADER INFO ========================

	// Set default values.
	mine_header.num_vertices = 0;
	mine_header.num_segments = 0;

	if (mine_fileinfo.header_offset > -1)
	{
		if (cfseek(LoadFile, mine_fileinfo.header_offset, SEEK_SET))
			Error("Error seeking to header_offset in gamemine.c");

		if (cfread(&mine_header, mine_fileinfo.header_size, 1, LoadFile) != 1)
			Error("Error reading mine_header in gamemine.c");
	}

	//===================== READ EDITOR INFO ==========================

	// Set default values
	mine_editor.current_seg = 0;
	mine_editor.newsegment_offset = -1; // To be written
	mine_editor.newsegment_size = sizeof(segment);
	mine_editor.Curside = 0;
	mine_editor.Markedsegp = -1;
	mine_editor.Markedside = 0;

	if (mine_fileinfo.editor_offset > -1)
	{
		if (cfseek(LoadFile, mine_fileinfo.editor_offset, SEEK_SET))
			Error("Error seeking to editor_offset in gamemine.c");

		if (cfread(&mine_editor, mine_fileinfo.editor_size, 1, LoadFile) != 1)
			Error("Error reading mine_editor in gamemine.c");
	}

	//===================== READ TEXTURE INFO ==========================

	if ((mine_fileinfo.texture_offset > -1) && (mine_fileinfo.texture_howmany > 0))
	{
		if (cfseek(LoadFile, mine_fileinfo.texture_offset, SEEK_SET))
			Error("Error seeking to texture_offset in gamemine.c");

		for (i = 0; i < mine_fileinfo.texture_howmany; i++)
		{
			if (cfread(&old_tmap_list[i], mine_fileinfo.texture_sizeof, 1, LoadFile) != 1)
				Error("Error reading old_tmap_list[i] in gamemine.c");
		}
	}

	//=============== GENERATE TEXTURE TRANSLATION TABLE ===============

	translate = 0;

	Assert(NumTextures < MAX_TEXTURES);

	{
		hashtable ht;

		hashtable_init(&ht, NumTextures);

		// Remove all the file extensions in the textures list

		for (i = 0; i < NumTextures; i++) 
		{
			temptr = strchr(TmapInfo[i].filename, '.');
			if (temptr)* temptr = '\0';
			hashtable_insert(&ht, TmapInfo[i].filename, i);
		}

		// For every texture, search through the texture list
		// to find a matching name.
		for (j = 0; j < mine_fileinfo.texture_howmany; j++) 
		{
			// Remove this texture name's extension
			temptr = strchr(old_tmap_list[j], '.');
			if (temptr)* temptr = '\0';

			tmap_xlate_table[j] = hashtable_search(&ht, old_tmap_list[j]);
			if (tmap_xlate_table[j] < 0) 
			{
				//tmap_xlate_table[j] = 0;
				// mprintf( (0, "Couldn't find texture '%s'\n", old_tmap_list[j] ));
				;
			}
			if (tmap_xlate_table[j] != j) translate = 1;
			if (tmap_xlate_table[j] >= 0)
				tmap_times_used[tmap_xlate_table[j]]++;
		}

		{
			int count = 0;
			for (i = 0; i < MAX_TEXTURES; i++)
				if (tmap_times_used[i])
					count++;
			mprintf((0, "This mine has %d unique textures in it (~%d KB)\n", count, (count * 4096) / 1024));
		}

		mprintf((0, "Translate=%d\n", translate));

		hashtable_free(&ht);
	}

	//====================== READ VERTEX INFO ==========================

	// New check added to make sure we don't read in too many vertices.
	if (mine_fileinfo.vertex_howmany > MAX_VERTICES)
	{
		mprintf((0, "Num vertices exceeds maximum.  Loading MAX %d vertices\n", MAX_VERTICES));
		mine_fileinfo.vertex_howmany = MAX_VERTICES;
	}

	if ((mine_fileinfo.vertex_offset > -1) && (mine_fileinfo.vertex_howmany > 0))
	{
		if (cfseek(LoadFile, mine_fileinfo.vertex_offset, SEEK_SET))
			Error("Error seeking to vertex_offset in gamemine.c");

		for (i = 0; i < mine_fileinfo.vertex_howmany; i++)
		{
			// Set the default values for this vertex
			Vertices[i].x = 1;
			Vertices[i].y = 1;
			Vertices[i].z = 1;

			Vertices[i].x = CF_ReadInt(LoadFile);
			Vertices[i].y = CF_ReadInt(LoadFile);
			Vertices[i].z = CF_ReadInt(LoadFile);

			//if (cfread(&Vertices[i], mine_fileinfo.vertex_sizeof, 1, LoadFile) != 1)
			//	Error("Error reading Vertices[i] in gamemine.c");
		}
	}

	//==================== READ SEGMENT INFO ===========================

	// New check added to make sure we don't read in too many segments.
	if (mine_fileinfo.segment_howmany > MAX_SEGMENTS) 
	{
		mprintf((0, "Num segments exceeds maximum.  Loading MAX %d segments\n", MAX_SEGMENTS));
		mine_fileinfo.segment_howmany = MAX_SEGMENTS;
	}

	// [commented out by mk on 11/20/94 (weren't we supposed to hit final in October?) because it looks redundant.  I think I'll test it now...]  fuelcen_reset();

	if ((mine_fileinfo.segment_offset > -1) && (mine_fileinfo.segment_howmany > 0)) 
	{
		if (cfseek(LoadFile, mine_fileinfo.segment_offset, SEEK_SET))
			Error("Error seeking to segment_offset in gamemine.c");

		Highest_segment_index = mine_fileinfo.segment_howmany - 1;

		for (i = 0; i < mine_fileinfo.segment_howmany; i++) 
		{
			segment v16_seg;

			// Set the default values for this segment (clear to zero )
			//memset( &Segments[i], 0, sizeof(segment) );

			if (mine_top_fileinfo.fileinfo_version >= 16) 
			{
				Assert(mine_fileinfo.segment_sizeof == SEGMENT_SIZEOF);

				//if (cfread(&v16_seg, mine_fileinfo.segment_sizeof, 1, LoadFile) != 1)
				//	Error("Error reading segments in gamemine.c");
				load_v16_segment(&v16_seg, LoadFile);
			}
			else
				Error("Invalid mine version");

			Segments[i] = v16_seg;

			Segments[i].objects = -1;
#ifdef EDITOR
			Segments[i].group = -1;
#endif

			if (mine_top_fileinfo.fileinfo_version < 15) //used old uvl ranges
			{
				int sn, uvln;

				for (sn = 0; sn < MAX_SIDES_PER_SEGMENT; sn++)
					for (uvln = 0; uvln < 4; uvln++)
					{
						Segments[i].sides[sn].uvls[uvln].u /= 64;
						Segments[i].sides[sn].uvls[uvln].v /= 64;
						Segments[i].sides[sn].uvls[uvln].l /= 32;
					}
			}

			fuelcen_activate(&Segments[i], Segments[i].special);

			if (translate == 1)
				for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) 
				{
					unsigned short orient;
					tmap_xlate = Segments[i].sides[j].tmap_num;
					Segments[i].sides[j].tmap_num = tmap_xlate_table[tmap_xlate];
					if ((WALL_IS_DOORWAY(&Segments[i], j) & WID_RENDER_FLAG))
						if (Segments[i].sides[j].tmap_num < 0) 
						{
							mprintf((0, "Couldn't find texture '%s' for Segment %d, side %d\n", old_tmap_list[tmap_xlate], i, j));
							Int3();
							Segments[i].sides[j].tmap_num = 0;
						}
					tmap_xlate = Segments[i].sides[j].tmap_num2 & 0x3FFF;
					orient = Segments[i].sides[j].tmap_num2 & (~0x3FFF);
					if (tmap_xlate != 0)
					{
						int xlated_tmap = tmap_xlate_table[tmap_xlate];

						if ((WALL_IS_DOORWAY(&Segments[i], j) & WID_RENDER_FLAG))
							if (xlated_tmap <= 0) 
							{
								mprintf((0, "Couldn't find texture '%s' for Segment %d, side %d\n", old_tmap_list[tmap_xlate], i, j));
								Int3();
								Segments[i].sides[j].tmap_num2 = 0;
							}
						Segments[i].sides[j].tmap_num2 = xlated_tmap | orient;
					}
				}
		}
	}

	//===================== READ NEWSEGMENT INFO =====================

#ifdef EDITOR

	{		// Default segment created.
		vms_vector	sizevec;
		med_create_new_segment(vm_vec_make(&sizevec, DEFAULT_X_SIZE, DEFAULT_Y_SIZE, DEFAULT_Z_SIZE));		// New_segment = Segments[0];
		//memset( &New_segment, 0, sizeof(segment) );
	}

	if (mine_editor.newsegment_offset > -1)
	{
		if (cfseek(LoadFile, mine_editor.newsegment_offset, SEEK_SET))
			Error("Error seeking to newsegment_offset in gamemine.c");
		load_v16_segment(&New_segment, LoadFile);
		//if (cfread(&New_segment, mine_editor.newsegment_size, 1, LoadFile) != 1)
		//	Error("Error reading new_segment in gamemine.c");
	}

	if ((mine_fileinfo.newseg_verts_offset > -1) && (mine_fileinfo.newseg_verts_howmany > 0))
	{
		if (cfseek(LoadFile, mine_fileinfo.newseg_verts_offset, SEEK_SET))
			Error("Error seeking to newseg_verts_offset in gamemine.c");
		for (i = 0; i < mine_fileinfo.newseg_verts_howmany; i++)
		{
			// Set the default values for this vertex
			Vertices[NEW_SEGMENT_VERTICES + i].x = 1;
			Vertices[NEW_SEGMENT_VERTICES + i].y = 1;
			Vertices[NEW_SEGMENT_VERTICES + i].z = 1;

			Vertices[NEW_SEGMENT_VERTICES + 1].x = CF_ReadInt(LoadFile);
			Vertices[NEW_SEGMENT_VERTICES + 1].y = CF_ReadInt(LoadFile);
			Vertices[NEW_SEGMENT_VERTICES + 1].z = CF_ReadInt(LoadFile);

			//if (cfread(&Vertices[NEW_SEGMENT_VERTICES + i], mine_fileinfo.newseg_verts_sizeof, 1, LoadFile) != 1)
			//	Error("Error reading Vertices[NEW_SEGMENT_VERTICES+i] in gamemine.c");

			New_segment.verts[i] = NEW_SEGMENT_VERTICES + i;
		}
	}

#endif

	//========================= UPDATE VARIABLES ======================

#ifdef EDITOR

// Setting to Markedsegp to NULL ignores Curside and Markedside, which
// we want to do when reading in an old file.

	Markedside = mine_editor.Markedside;
	Curside = mine_editor.Curside;
	for (i = 0; i < 10; i++)
		Groupside[i] = mine_editor.Groupside[i];

	if (mine_editor.current_seg != -1)
		Cursegp = mine_editor.current_seg + Segments;
	else
		Cursegp = NULL;

	if (mine_editor.Markedsegp != -1)
		Markedsegp = mine_editor.Markedsegp + Segments;
	else
		Markedsegp = NULL;

	Num_groups = 0;
	Current_group = -1;

#endif

	Num_vertices = mine_fileinfo.vertex_howmany;
	Num_segments = mine_fileinfo.segment_howmany;
	Highest_vertex_index = Num_vertices - 1;
	Highest_segment_index = Num_segments - 1;

	reset_objects(1);		//one object, the player

#ifdef EDITOR
	Highest_vertex_index = MAX_SEGMENT_VERTICES - 1;
	Highest_segment_index = MAX_SEGMENTS - 1;
	set_vertex_counts();
	Highest_vertex_index = Num_vertices - 1;
	Highest_segment_index = Num_segments - 1;

	warn_if_concave_segments();
#endif

#ifdef EDITOR
	validate_segment_all();
#endif

	//create_local_segment_data();

	//gamemine_find_textures();

	if (mine_top_fileinfo.fileinfo_version < MINE_VERSION)
		return 1;		//old version
	else
		return 0;

}
#endif

#define COMPILED_MINE_VERSION 0

int	New_file_format_load = 1;

#ifndef SHAREWARE
int load_mine_data_compiled_new(CFILE* LoadFile);
#endif

int load_mine_data_compiled(CFILE* LoadFile)
{
	int i, segnum, sidenum;
	uint8_t version;
	short temp_short;
	uint16_t temp_ushort;

#ifndef SHAREWARE
	if (New_file_format_load)
		return load_mine_data_compiled_new(LoadFile);
#endif

	//	For compiled levels, textures map to themselves, prevent tmap_override always being gray,
	//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
	// 
	// Although in a cloud of arrogant glee, he forgot to ifdef it on EDITOR!
	// (Matt told me to write that!)
#ifdef EDITOR
	for (i = 0; i < MAX_TEXTURES; i++)
		tmap_xlate_table[i] = i;
#endif
	//	memset( Segments, 0, sizeof(segment)*MAX_SEGMENTS );
	fuelcen_reset();

	//=============================== Reading part ==============================
	cfread(&version, sizeof(uint8_t), 1, LoadFile);						// 1 byte = compiled version
	Assert(version == COMPILED_MINE_VERSION);
	cfread(&Num_vertices, sizeof(int), 1, LoadFile);					// 4 bytes = Num_vertices
	Assert(Num_vertices <= MAX_VERTICES);
	cfread(&Num_segments, sizeof(int), 1, LoadFile);					// 4 bytes = Num_segments
	Assert(Num_segments <= MAX_SEGMENTS);
	cfread(Vertices, sizeof(vms_vector), Num_vertices, LoadFile);

	for (segnum = 0; segnum < Num_segments; segnum++) {
#ifdef EDITOR
		Segments[segnum].segnum = segnum;
		Segments[segnum].group = 0;
#endif

		// Read short Segments[segnum].children[MAX_SIDES_PER_SEGMENT]
		cfread(Segments[segnum].children, sizeof(short), MAX_SIDES_PER_SEGMENT, LoadFile);
		// Read short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
		cfread(Segments[segnum].verts, sizeof(short), MAX_VERTICES_PER_SEGMENT, LoadFile);
		Segments[segnum].objects = -1;

		// Read uint8_t	Segments[segnum].special
		cfread(&Segments[segnum].special, sizeof(uint8_t), 1, LoadFile);
		// Read int8_t	Segments[segnum].matcen_num
		cfread(&Segments[segnum].matcen_num, sizeof(uint8_t), 1, LoadFile);
		// Read short	Segments[segnum].value
		cfread(&Segments[segnum].value, sizeof(short), 1, LoadFile);

		// Read fix	Segments[segnum].static_light (shift down 5 bits, write as short)
		cfread(&temp_ushort, sizeof(temp_ushort), 1, LoadFile);
		Segments[segnum].static_light = ((fix)temp_ushort) << 4;
		//cfread( &Segments[segnum].static_light, sizeof(fix), 1, LoadFile );

		// Read the walls as a 6 int8_t array
		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
			uint8_t byte_wallnum;
			Segments[segnum].sides[sidenum].pad = 0;
			cfread(&byte_wallnum, sizeof(uint8_t), 1, LoadFile);
			if (byte_wallnum == 255)
				Segments[segnum].sides[sidenum].wall_num = -1;
			else
				Segments[segnum].sides[sidenum].wall_num = byte_wallnum;
		}

		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {

			if ((Segments[segnum].children[sidenum] == -1) || (Segments[segnum].sides[sidenum].wall_num != -1)) {
				// Read short Segments[segnum].sides[sidenum].tmap_num;
				cfread(&Segments[segnum].sides[sidenum].tmap_num, sizeof(short), 1, LoadFile);
				// Read short Segments[segnum].sides[sidenum].tmap_num2;
				cfread(&Segments[segnum].sides[sidenum].tmap_num2, sizeof(short), 1, LoadFile);
				// Read uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i = 0; i < 4; i++) {
					cfread(&temp_short, sizeof(short), 1, LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].u = ((fix)temp_short) << 5;
					cfread(&temp_short, sizeof(short), 1, LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].v = ((fix)temp_short) << 5;
					cfread(&temp_ushort, sizeof(temp_ushort), 1, LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].l = ((fix)temp_ushort) << 1;
					//cfread( &Segments[segnum].sides[sidenum].uvls[i].l, sizeof(fix), 1, LoadFile );
				}
			}
			else {
				Segments[segnum].sides[sidenum].tmap_num = 0;
				Segments[segnum].sides[sidenum].tmap_num2 = 0;
				for (i = 0; i < 4; i++) {
					Segments[segnum].sides[sidenum].uvls[i].u = 0;
					Segments[segnum].sides[sidenum].uvls[i].v = 0;
					Segments[segnum].sides[sidenum].uvls[i].l = 0;
				}
			}
		}
	}

	Highest_vertex_index = Num_vertices - 1;
	Highest_segment_index = Num_segments - 1;

	validate_segment_all();			// Fill in side type and normals.

	// Activate fuelcentes
	for (i = 0; i < Num_segments; i++) {
		fuelcen_activate(&Segments[i], Segments[i].special);
	}

	reset_objects(1);		//one object, the player

	return 0;
}

#ifndef SHAREWARE
int load_mine_data_compiled_new(CFILE* LoadFile)
{
	int		i, segnum, sidenum;
	uint8_t		version;
	short		temp_short;
	uint16_t	temp_ushort;
	uint8_t		bit_mask;

	//	For compiled levels, textures map to themselves, prevent tmap_override always being gray,
	//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
#ifdef EDITOR
	for (i = 0; i < MAX_TEXTURES; i++)
		tmap_xlate_table[i] = i;
#endif

	//	memset( Segments, 0, sizeof(segment)*MAX_SEGMENTS );
	fuelcen_reset();

	//=============================== Reading part ==============================
	cfread(&version, sizeof(uint8_t), 1, LoadFile);						// 1 byte = compiled version
	Assert(version == COMPILED_MINE_VERSION);

	cfread(&temp_ushort, sizeof(uint16_t), 1, LoadFile);					// 2 bytes = Num_vertices
	Num_vertices = temp_ushort;
	Assert(Num_vertices <= MAX_VERTICES);

	cfread(&temp_ushort, sizeof(uint16_t), 1, LoadFile);					// 2 bytes = Num_segments
	Num_segments = temp_ushort;
	Assert(Num_segments <= MAX_SEGMENTS);

	cfread(Vertices, sizeof(vms_vector), Num_vertices, LoadFile);

	for (segnum = 0; segnum < Num_segments; segnum++) {
		int	bit;

#ifdef EDITOR
		Segments[segnum].segnum = segnum;
		Segments[segnum].group = 0;
#endif

		cfread(&bit_mask, sizeof(uint8_t), 1, LoadFile);

		for (bit = 0; bit < MAX_SIDES_PER_SEGMENT; bit++) {
			if (bit_mask & (1 << bit))
				cfread(&Segments[segnum].children[bit], sizeof(short), 1, LoadFile);
			else
				Segments[segnum].children[bit] = -1;
		}

		// Read short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
		cfread(Segments[segnum].verts, sizeof(short), MAX_VERTICES_PER_SEGMENT, LoadFile);
		Segments[segnum].objects = -1;

		if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
			// Read uint8_t	Segments[segnum].special
			cfread(&Segments[segnum].special, sizeof(uint8_t), 1, LoadFile);
			// Read int8_t	Segments[segnum].matcen_num
			cfread(&Segments[segnum].matcen_num, sizeof(uint8_t), 1, LoadFile);
			// Read short	Segments[segnum].value
			cfread(&Segments[segnum].value, sizeof(short), 1, LoadFile);
		}
		else {
			Segments[segnum].special = 0;
			Segments[segnum].matcen_num = -1;
			Segments[segnum].value = 0;
		}

		// Read fix	Segments[segnum].static_light (shift down 5 bits, write as short)
		cfread(&temp_ushort, sizeof(temp_ushort), 1, LoadFile);
		Segments[segnum].static_light = ((fix)temp_ushort) << 4;
		//cfread( &Segments[segnum].static_light, sizeof(fix), 1, LoadFile );

		// Read the walls as a 6 int8_t array
		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
			Segments[segnum].sides[sidenum].pad = 0;
		}

		cfread(&bit_mask, sizeof(uint8_t), 1, LoadFile);
		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
			uint8_t byte_wallnum;

			if (bit_mask & (1 << sidenum)) {
				cfread(&byte_wallnum, sizeof(uint8_t), 1, LoadFile);
				if (byte_wallnum == 255)
					Segments[segnum].sides[sidenum].wall_num = -1;
				else
					Segments[segnum].sides[sidenum].wall_num = byte_wallnum;
			}
			else
				Segments[segnum].sides[sidenum].wall_num = -1;
		}

		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {

			if ((Segments[segnum].children[sidenum] == -1) || (Segments[segnum].sides[sidenum].wall_num != -1)) {
				// Read short Segments[segnum].sides[sidenum].tmap_num;
				cfread(&temp_ushort, sizeof(uint16_t), 1, LoadFile);

				Segments[segnum].sides[sidenum].tmap_num = temp_ushort & 0x7fff;

				if (!(temp_ushort & 0x8000))
					Segments[segnum].sides[sidenum].tmap_num2 = 0;
				else {
					// Read short Segments[segnum].sides[sidenum].tmap_num2;
					cfread(&Segments[segnum].sides[sidenum].tmap_num2, sizeof(short), 1, LoadFile);
				}

				// Read uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i = 0; i < 4; i++) {
					cfread(&temp_short, sizeof(short), 1, LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].u = ((fix)temp_short) << 5;
					cfread(&temp_short, sizeof(short), 1, LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].v = ((fix)temp_short) << 5;
					cfread(&temp_ushort, sizeof(temp_ushort), 1, LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].l = ((fix)temp_ushort) << 1;
					//cfread( &Segments[segnum].sides[sidenum].uvls[i].l, sizeof(fix), 1, LoadFile );
				}
			}
			else {
				Segments[segnum].sides[sidenum].tmap_num = 0;
				Segments[segnum].sides[sidenum].tmap_num2 = 0;
				for (i = 0; i < 4; i++) {
					Segments[segnum].sides[sidenum].uvls[i].u = 0;
					Segments[segnum].sides[sidenum].uvls[i].v = 0;
					Segments[segnum].sides[sidenum].uvls[i].l = 0;
				}
			}
		}
	}

	Highest_vertex_index = Num_vertices - 1;
	Highest_segment_index = Num_segments - 1;

	validate_segment_all();			// Fill in side type and normals.

	// Activate fuelcentes
	for (i = 0; i < Num_segments; i++) {
		fuelcen_activate(&Segments[i], Segments[i].special);
	}

	reset_objects(1);		//one object, the player

	return 0;
}

#endif
