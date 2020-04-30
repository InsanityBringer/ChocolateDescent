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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>

#include "2d/i_gr.h"
//#include "pa_enabl.h"                   //$$POLY_ACC
#include "misc/error.h"
#include "3d/3d.h"
#include "inferno.h"
#include "mem/mem.h"
#include "render.h"
#include "object.h"
#include "vclip.h"
#include "game.h"
#include "platform/mono.h"
#include "polyobj.h"
#include "sounds.h"
#include "player.h"
#include "bm.h"
#include "platform/key.h"
#include "newmenu.h"
#include "menu.h"
#include "screens.h"
#include "textures.h"
#include "platform/mouse.h"
#include "platform/timer.h"
#include "segpoint.h"
#include "platform/joy.h"
#include "iff/iff.h"
#include "2d/pcx.h"
#include "2d/palette.h"
#include "wall.h"
#include "gameseq.h"
#include "gamefont.h"
#include "network.h"
#include "kconfig.h"
#include "multi.h"
#include "endlevel.h"
#include "text.h"
#include "gauges.h"
#include "songs.h"
#include "powerup.h"
#include "network.h"
#include "switch.h"
#include "automap.h"
#include "cntrlcen.h"
#ifndef WINDOWS
//#include "vga.h"
#endif

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#define EF_USED			1		// This edge is used
#define EF_DEFINING		2		// A structure defining edge that should always draw.
#define EF_FRONTIER		4		// An edge between the known and the unknown.
#define EF_SECRET			8		// An edge that is part of a secret wall.
#define EF_GRATE			16		// A grate... draw it all the time.
#define EF_NO_FADE		32		// An edge that doesn't fade with distance
#define EF_TOO_FAR		64		// An edge that is too far away

typedef struct Edge_info {
	short verts[2];				// 4 bytes
	uint8_t sides[4];				// 4 bytes
	short segnum[4];			// 8 bytes	// This might not need to be stored... If you can access the normals of a side.
	uint8_t flags;				// 1 bytes 	// See the EF_??? defines above.
	uint8_t color;				// 1 bytes
	uint8_t num_faces;			// 1 bytes	// 19 bytes...
} Edge_info;

//OLD BUT GOOD -- #define MAX_EDGES_FROM_VERTS(v)   ((v*5)/2)
// THE following was determined by John by loading levels 1-14 and recording
// numbers on 10/26/94. 
//#define MAX_EDGES_FROM_VERTS(v)   (((v)*21)/10)
#define MAX_EDGES_FROM_VERTS(v)		((v)*4)
//#define MAX_EDGES (MAX_EDGES_FROM_VERTS(MAX_VERTICES))

#define MAX_EDGES 6000		// Determined by loading all the levels by John & Mike, Feb 9, 1995

#define	K_WALL_NORMAL_COLOR 			BM_XRGB( 29, 29, 29 )
#define	K_WALL_DOOR_COLOR				BM_XRGB( 5, 27, 5 )
#define	K_WALL_DOOR_BLUE				BM_XRGB( 0, 0, 31)
#define	K_WALL_DOOR_GOLD				BM_XRGB( 31, 31, 0)
#define	K_WALL_DOOR_RED				BM_XRGB( 31, 0, 0)
#define	K_WALL_REVEALED_COLOR		BM_XRGB( 0, 0, 25 )	//what you see when you have the full map powerup
#define	K_HOSTAGE_COLOR				BM_XRGB( 0, 31, 0 )
#define	K_FONT_COLOR_20				BM_XRGB( 20, 20, 20 )
#define	K_GREEN_31						BM_XRGB(0, 31, 0)

int	Wall_normal_color;
int	Wall_door_color;
int	Wall_door_blue;
int	Wall_door_gold;
int	Wall_door_red;
int	Wall_revealed_color;
int	Hostage_color;
int	Font_color_20;
int	Green_31;
int	White_63;
int	Blue_48;
int	Red_48;

void init_automap_colors(void)
{
	Wall_normal_color = K_WALL_NORMAL_COLOR;
	Wall_door_color = K_WALL_DOOR_COLOR;
	Wall_door_blue = K_WALL_DOOR_BLUE;
	Wall_door_gold = K_WALL_DOOR_GOLD;
	Wall_door_red = K_WALL_DOOR_RED;
	Wall_revealed_color = K_WALL_REVEALED_COLOR;
	Hostage_color = K_HOSTAGE_COLOR;
	Font_color_20 = K_FONT_COLOR_20;
	Green_31 = K_GREEN_31;

	White_63 = gr_find_closest_color_current(63, 63, 63);
	Blue_48 = gr_find_closest_color_current(0, 0, 48);
	Red_48 = gr_find_closest_color_current(48, 0, 0);
}

// Segment visited list
uint8_t Automap_visited[MAX_SEGMENTS];

// Edge list variables
static int Num_edges = 0;
static int Max_edges;		//set each frame
static int Highest_edge_index = -1;
static Edge_info Edges[MAX_EDGES];
static short DrawingListBright[MAX_EDGES];

//static short DrawingListBright[MAX_EDGES];
//static short Edge_used_list[MAX_EDGES];				//which entries in edge_list have been used

// Map movement defines
#define PITCH_DEFAULT 9000
#define ZOOM_DEFAULT i2f(20*10)
#define ZOOM_MIN_VALUE i2f(20*5)
#define ZOOM_MAX_VALUE i2f(20*100)

#define SLIDE_SPEED 				(350)
#define ZOOM_SPEED_FACTOR		500	//(1500)
#define ROT_SPEED_DIVISOR		(115000)

// Screen anvas variables
static int current_page = 0;
/*
#ifdef WINDOWS
static dd_grs_canvas ddPages[2];
static dd_grs_canvas ddDrawingPages[2];

#define ddPage ddPages[0]
#define ddDrawingPage ddDrawingPages[0]

#endif*/

#if defined(MACINTOSH) && defined(POLY_ACC)
grs_canvas Pages[2];			// non static under rave so the backbuffer callback function can get at them
grs_canvas DrawingPages[2];		// non static under rave so the backbuffer callback function can get at them
#else
static grs_canvas Pages[2];
static grs_canvas DrawingPages[2];
#endif

#define Page Pages[0]
#define DrawingPage DrawingPages[0]

// Flags
static int Automap_cheat = 0;		// If set, show everything

// Rendering variables
static fix Automap_zoom = 0x9000;
static vms_vector view_target;
static fix Automap_farthest_dist = (F1_0 * 20 * 50);		// 50 segments away
static vms_matrix	ViewMatrix;
static fix ViewDist = 0;

//	Function Prototypes
void adjust_segment_limit(int SegmentLimit);
void draw_all_edges(void);
void automap_build_edge_list(void);

#define	MAX_DROP_MULTI		2
#define	MAX_DROP_SINGLE	9

vms_vector MarkerPoint[NUM_MARKERS];		//these are only used in multi.c, and I'd get rid of them there, but when I tried to do that once, I caused some horrible bug. -MT
int HighlightMarker = -1;
char MarkerMessage[NUM_MARKERS][MARKER_MESSAGE_LEN];
char MarkerOwner[NUM_MARKERS][CALLSIGN_LEN + 1];
float MarkerScale = 2.0;
int	MarkerObject[NUM_MARKERS];

extern vms_vector Matrix_scale;		//how the matrix is currently scaled

// -------------------------------------------------------------

void DrawMarkerNumber(int num)
{
	int i;
	g3s_point BasePoint, FromPoint, ToPoint;

	float ArrayX[10][20] = { {-.25, 0.0, 0.0, 0.0, -1.0, 1.0},
						   {-1.0, 1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0, 1.0},
						   {-1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 0.0, 1.0},
						   {-1.0, -1.0, -1.0, 1.0, 1.0, 1.0},
						   {-1.0, 1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0, 1.0},
						   {-1.0, 1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0, 1.0},
						   {-1.0, 1.0, 1.0, 1.0},
						   {-1.0, 1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0, 1.0},
						   {-1.0, 1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0}

	};
	float ArrayY[10][20] = { {.75, 1.0, 1.0, -1.0, -1.0, -1.0},
						   {1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, -1.0, -1.0},
						   {1.0, 1.0, 1.0, -1.0, -1.0, -1.0, 0.0, 0.0},
						   {1.0, 0.0, 0.0, 0.0, 1.0, -1.0},
						   {1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, -1.0, -1.0},
						   {1.0, 1.0, 1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 0.0},
						   {1.0, 1.0, 1.0, -1.0},
						   {1.0, 1.0, 1.0, -1.0, -1.0, -1.0, -1.0, 1.0, 0.0, 0.0},
						   {1.0, 1.0, 1.0, -1.0, 0.0, 0.0, 0.0, 1.0}
	};
	int NumOfPoints[] = { 6,10,8,6,10,10,4,10,8 };

	for (i = 0; i < NumOfPoints[num]; i++)
	{
		ArrayX[num][i] *= MarkerScale;
		ArrayY[num][i] *= MarkerScale;
	}

	if (num == HighlightMarker)
		gr_setcolor(White_63);
	else
		gr_setcolor(Blue_48);


	g3_rotate_point(&BasePoint, &Objects[MarkerObject[(Player_num * 2) + num]].pos);

	for (i = 0; i < NumOfPoints[num]; i += 2)
	{

		FromPoint = BasePoint;
		ToPoint = BasePoint;

		FromPoint.p3_x += fixmul((fl2f(ArrayX[num][i])), Matrix_scale.x);
		FromPoint.p3_y += fixmul((fl2f(ArrayY[num][i])), Matrix_scale.y);
		g3_code_point(&FromPoint);
		g3_project_point(&FromPoint);

		ToPoint.p3_x += fixmul((fl2f(ArrayX[num][i + 1])), Matrix_scale.x);
		ToPoint.p3_y += fixmul((fl2f(ArrayY[num][i + 1])), Matrix_scale.y);
		g3_code_point(&ToPoint);
		g3_project_point(&ToPoint);

#if	defined(MACINTOSH) && defined(POLY_ACC)
		{
			int savePAEnabledState = PAEnabled;	// icky hack.  automap draw context is no longer valid when this is called.
												// so we can not use the pa_draw_line function for rave

			PAEnabled = 0;
			g3_draw_line(&FromPoint, &ToPoint);
			PAEnabled = savePAEnabledState;
		}
#else
		g3_draw_line(&FromPoint, &ToPoint);
#endif

	}
}

void DropMarker(int player_marker_num)
{
	int marker_num = (Player_num * 2) + player_marker_num;
	object* playerp = &Objects[Players[Player_num].objnum];

	MarkerPoint[marker_num] = playerp->pos;

	if (MarkerObject[marker_num] != -1)
		obj_delete(MarkerObject[marker_num]);

	MarkerObject[marker_num] = drop_marker_object(&playerp->pos, playerp->segnum, &playerp->orient, marker_num);

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
		multi_send_drop_marker(Player_num, playerp->pos, player_marker_num, MarkerMessage[marker_num]);
#endif

}

extern char guidebot_name[];

void DropBuddyMarker(object* objp)
{
	int	marker_num;

	//	Find spare marker slot.  "if" code below should be an assert, but what if someone changes NUM_MARKERS or MAX_CROP_SINGLE and it never gets hit?
	marker_num = MAX_DROP_SINGLE + 1;
	if (marker_num > NUM_MARKERS - 1)
		marker_num = NUM_MARKERS - 1;

	sprintf(MarkerMessage[marker_num], "RIP: %s", guidebot_name);

	MarkerPoint[marker_num] = objp->pos;

	if (MarkerObject[marker_num] != -1 && MarkerObject[marker_num] != 0)
		obj_delete(MarkerObject[marker_num]);

	MarkerObject[marker_num] = drop_marker_object(&objp->pos, objp->segnum, &objp->orient, marker_num);

}

#define MARKER_SPHERE_SIZE 0x58000

void DrawMarkers()
{
	int i, maxdrop;
	static int cyc = 10, cycdir = 1;
	g3s_point sphere_point;

	if (Game_mode & GM_MULTI)
		maxdrop = 2;
	else
		maxdrop = 9;

	for (i = 0; i < maxdrop; i++)
		if (MarkerObject[(Player_num * 2) + i] != -1) {

			g3_rotate_point(&sphere_point, &Objects[MarkerObject[(Player_num * 2) + i]].pos);

			gr_setcolor(gr_find_closest_color_current(cyc, 0, 0));
			g3_draw_sphere(&sphere_point, MARKER_SPHERE_SIZE);
			gr_setcolor(gr_find_closest_color_current(cyc + 10, 0, 0));
			g3_draw_sphere(&sphere_point, MARKER_SPHERE_SIZE / 2);
			gr_setcolor(gr_find_closest_color_current(cyc + 20, 0, 0));
			g3_draw_sphere(&sphere_point, MARKER_SPHERE_SIZE / 4);

			DrawMarkerNumber(i);
		}

	if (cycdir)
		cyc += 2;
	else
		cyc -= 2;

	if (cyc > 43)
	{
		cyc = 43;
		cycdir = 0;
	}
	else if (cyc < 10)
	{
		cyc = 10;
		cycdir = 1;
	}

}

void ClearMarkers()
{
	int i;

	for (i = 0; i < NUM_MARKERS; i++) {
		MarkerMessage[i][0] = 0;
		MarkerObject[i] = -1;
	}
}

void automap_clear_visited()
{
	int i;
	for (i = 0; i < MAX_SEGMENTS; i++)
		Automap_visited[i] = 0;
	ClearMarkers();
}

grs_canvas* name_canv_left, * name_canv_right;

void draw_player(object* obj)
{
	vms_vector arrow_pos, head_pos;
	g3s_point sphere_point, arrow_point, head_point;

	// Draw Console player -- shaped like a ellipse with an arrow.
	g3_rotate_point(&sphere_point, &obj->pos);
	g3_draw_sphere(&sphere_point, obj->size);

	// Draw shaft of arrow
	vm_vec_scale_add(&arrow_pos, &obj->pos, &obj->orient.fvec, obj->size * 3);
	g3_rotate_point(&arrow_point, &arrow_pos);
	g3_draw_line(&sphere_point, &arrow_point);

	// Draw right head of arrow
	vm_vec_scale_add(&head_pos, &obj->pos, &obj->orient.fvec, obj->size * 2);
	vm_vec_scale_add2(&head_pos, &obj->orient.rvec, obj->size * 1);
	g3_rotate_point(&head_point, &head_pos);
	g3_draw_line(&arrow_point, &head_point);

	// Draw left head of arrow
	vm_vec_scale_add(&head_pos, &obj->pos, &obj->orient.fvec, obj->size * 2);
	vm_vec_scale_add2(&head_pos, &obj->orient.rvec, obj->size * (-1));
	g3_rotate_point(&head_point, &head_pos);
	g3_draw_line(&arrow_point, &head_point);

	// Draw player's up vector
	vm_vec_scale_add(&arrow_pos, &obj->pos, &obj->orient.uvec, obj->size * 2);
	g3_rotate_point(&arrow_point, &arrow_pos);
	g3_draw_line(&sphere_point, &arrow_point);
}

int AutomapHires;

void draw_automap()
{
	int i;
	int color;
	object* objp;
	vms_vector viewer_position;
	g3s_point sphere_point;

	if (!AutomapHires) 
	{
		current_page ^= 1;
		gr_set_current_canvas(&DrawingPages[current_page]);
	}
	else 
	{
		gr_set_current_canvas(&DrawingPage);
	}

#if defined(POLY_ACC)
	pa_flush();
#endif

	gr_clear_canvas(BM_XRGB(0, 0, 0));

	{
		g3_start_frame();
		render_start_frame();

		vm_vec_scale_add(&viewer_position, &view_target, &ViewMatrix.fvec, -ViewDist);

		g3_set_view_matrix(&viewer_position, &ViewMatrix, Automap_zoom);

		//	mprintf((0, "dd_grd_curcanv->canvas.cv_bitmap.bm_data= %x\n", dd_grd_curcanv->canvas.cv_bitmap.bm_data));
		//	mprintf((0, "grd_curcanv->cv_bitmap.bm_data= %x\n", grd_curcanv->cv_bitmap.bm_data));

		draw_all_edges();

		// Draw player...
#ifdef NETWORK
		if (Game_mode & GM_TEAM)
			color = get_team(Player_num);
		else
#endif	
			color = Player_num;	// Note link to above if!

		gr_setcolor(gr_getcolor(player_rgb[color].r, player_rgb[color].g, player_rgb[color].b));
		draw_player(&Objects[Players[Player_num].objnum]);

		DrawMarkers();

		if (HighlightMarker > -1 && MarkerMessage[HighlightMarker][0] != 0)
		{
			char msg[10 + MARKER_MESSAGE_LEN + 1];

			sprintf(msg, "Marker %d: %s", HighlightMarker + 1, MarkerMessage[(Player_num * 2) + HighlightMarker]);

			gr_setcolor(Red_48);

			//modex_printf(5,20,msg,SMALL_FONT,Font_color_20); [ISB] TODO ughhh
		}

		// Draw player(s)...
#ifdef NETWORK
		if ((Game_mode & (GM_TEAM | GM_MULTI_COOP)) || (Netgame.game_flags & NETGAME_FLAG_SHOW_MAP)) {
			for (i = 0; i < N_players; i++) {
				if ((i != Player_num) && ((Game_mode & GM_MULTI_COOP) || (get_team(Player_num) == get_team(i)) || (Netgame.game_flags & NETGAME_FLAG_SHOW_MAP))) {
					if (Objects[Players[i].objnum].type == OBJ_PLAYER) {
						if (Game_mode & GM_TEAM)
							color = get_team(i);
						else
							color = i;
						gr_setcolor(gr_getcolor(player_rgb[color].r, player_rgb[color].g, player_rgb[color].b));
						draw_player(&Objects[Players[i].objnum]);
					}
				}
			}
		}
#endif

		objp = &Objects[0];
		for (i = 0; i <= Highest_object_index; i++, objp++) {
			switch (objp->type) {
			case OBJ_HOSTAGE:
				gr_setcolor(Hostage_color);
				g3_rotate_point(&sphere_point, &objp->pos);
				g3_draw_sphere(&sphere_point, objp->size);
				break;
			case OBJ_POWERUP:
				if (Automap_visited[objp->segnum]) {
					if ((objp->id == POW_KEY_RED) || (objp->id == POW_KEY_BLUE) || (objp->id == POW_KEY_GOLD)) {
						switch (objp->id) {
						case POW_KEY_RED:		gr_setcolor(gr_getcolor(63, 5, 5));	break;
						case POW_KEY_BLUE:	gr_setcolor(gr_getcolor(5, 5, 63)); break;
						case POW_KEY_GOLD:	gr_setcolor(gr_getcolor(63, 63, 10)); break;
						default:
							Error("Illegal key type: %i", objp->id);
						}
						g3_rotate_point(&sphere_point, &objp->pos);
						g3_draw_sphere(&sphere_point, objp->size * 4);
					}
				}
				break;
			}
		}

		g3_end_frame();

		gr_bitmapm(AutomapHires ? 10 : AutomapHires ? 10 : 5, 5, &name_canv_left->cv_bitmap);
		gr_bitmapm(grd_curcanv->cv_bitmap.bm_w - (AutomapHires ? 10 : 5) - name_canv_right->cv_bitmap.bm_w, AutomapHires ? 10 : 5, &name_canv_right->cv_bitmap);
	}

	if (!AutomapHires)
		gr_show_canvas(&Pages[current_page]);
	else 
	{
		gr_bm_ubitblt(Page.cv_bitmap.bm_w, Page.cv_bitmap.bm_h, Page.cv_bitmap.bm_x, Page.cv_bitmap.bm_y, 0, 0, &Page.cv_bitmap, &grd_curscreen->sc_canvas.cv_bitmap);
	}

}

#ifdef WINDOWS
#define LEAVE_TIME 0x00010000
#else
#define LEAVE_TIME 0x4000
#endif

#define WINDOW_WIDTH		288


//print to canvas & double height
grs_canvas* print_to_canvas(char* s, grs_font* font, int fc, int bc, int double_flag)
{
	int y;
	uint8_t* data;
	int rs;
	grs_canvas* temp_canv;
	grs_font* save_font;
	int w, h, aw;

	WINDOS(
		dd_grs_canvas * save_canv,
		grs_canvas * save_canv
	);

	WINDOS(
		save_canv = dd_grd_curcanv,
		save_canv = grd_curcanv
	);

	save_font = grd_curcanv->cv_font;
	gr_set_curfont(font);					//set the font we're going to use
	gr_get_string_size(s, &w, &h, &aw);		//now get the string size
	gr_set_curfont(save_font);				//restore real font

	//temp_canv = gr_create_canvas(font->ft_w*strlen(s),font->ft_h*2);
	temp_canv = gr_create_canvas(w, font->ft_h * 2);

	gr_set_current_canvas(temp_canv);
	gr_set_curfont(font);
	gr_clear_canvas(TRANSPARENCY_COLOR);						//trans color
	gr_set_fontcolor(fc, bc);
	gr_printf(0, 0, s);

	//now double it, since we're drawing to 400-line modex screen

	if (double_flag) 
	{
		data = temp_canv->cv_bitmap.bm_data;
		rs = temp_canv->cv_bitmap.bm_rowsize;

		for (y = temp_canv->cv_bitmap.bm_h / 2; y--;) {
			memcpy(data + (rs * y * 2), data + (rs * y), temp_canv->cv_bitmap.bm_w);
			memcpy(data + (rs * (y * 2 + 1)), data + (rs * y), temp_canv->cv_bitmap.bm_w);
		}
	}

	WINDOS(
		dd_gr_set_current_canvas(save_canv),
		gr_set_current_canvas(save_canv)
	);

	return temp_canv;
}

//print to buffer, double heights, and blit bitmap to screen
void modex_printf(int x, int y, char* s, grs_font* font, int color)
{
	grs_canvas* temp_canv;

	temp_canv = print_to_canvas(s, font, color, -1, !AutomapHires);

	gr_bitmapm(x, y, &temp_canv->cv_bitmap);

	gr_free_canvas(temp_canv);
}

//name for each group.  maybe move somewhere else
const char* system_name[] = {
			"Zeta Aquilae",
			"Quartzon System",
			"Brimspark System",
			"Limefrost Spiral",
			"Baloris Prime",
			"Omega System" };

void create_name_canv()
{
	char	name_level_left[128], name_level_right[128];

	if (Current_level_num > 0)
		sprintf(name_level_left, "%s %i", TXT_LEVEL, Current_level_num);
	else
		sprintf(name_level_left, "Secret Level %i", -Current_level_num);

	if (Current_mission_num == 0 && Current_level_num > 0)		//built-in mission
		sprintf(name_level_right, "%s %d: ", system_name[(Current_level_num - 1) / 4], ((Current_level_num - 1) % 4) + 1);
	else
		strcpy(name_level_right, " ");

	strcat(name_level_right, Current_level_name);

	gr_set_fontcolor(Green_31, -1);
	name_canv_left = print_to_canvas(name_level_left, SMALL_FONT, Green_31, -1, !AutomapHires);
	name_canv_right = print_to_canvas(name_level_right, SMALL_FONT, Green_31, -1, !AutomapHires);

}

void modex_print_message(int x, int y, char* str)
{
	if (!AutomapHires) {
		int	i;

		for (i = 0; i < 2; i++) {
			gr_set_current_canvas(&Pages[i]);
			modex_printf(x, y, str, MEDIUM1_FONT, Font_color_20);
		}
		gr_set_current_canvas(&DrawingPages[current_page]);
	}
	else {
		gr_set_current_canvas(&Page);
		modex_printf(x, y, str, MEDIUM1_FONT, Font_color_20);
		gr_set_current_canvas(&DrawingPage);
	}
}

extern void GameLoop(int, int);
extern int set_segment_depths(int start_seg, uint8_t* segbuf);

int Automap_active = 0;

#ifdef RELEASE
#define MAP_BACKGROUND_FILENAME (AutomapHires?"\x01MAPB.PCX":"\x01MAP.PCX")	//load only from hog file
#else
#define MAP_BACKGROUND_FILENAME (AutomapHires?"MAPB.PCX":"MAP.PCX")
#endif

int Automap_always_hires = 0;
extern int MenuHiresAvailable;

extern int Current_display_mode;

void do_automap(int key_code) 
{
	int done = 0;
	vms_matrix	tempm;
	vms_angvec	tangles;
	int leave_mode = 0;
	int first_time = 1;
	int pcx_error;
	int i;
	int c, marker_num;
	fix entry_time;
	int pause_game = 1;		// Set to 1 if everything is paused during automap...No pause during net.
	fix t1, t2;
	control_info saved_control_info;
	grs_bitmap Automap_background;
	int Max_segments_away = 0;
	int SegmentLimit = 1;
	uint8_t pal[256 * 3];
	char maxdrop;
	int must_free_canvas = 0;

	Automap_active = 1;

	init_automap_colors();

	key_code = key_code;	// disable warning...

	if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
		pause_game = 0;

	if (pause_game) {
		stop_time();
		digi_pause_digi_sounds();
	}

	Max_edges = std::min(MAX_EDGES_FROM_VERTS(Num_vertices), MAX_EDGES);			//make maybe smaller than max

	mprintf((0, "Num_vertices=%d, Max_edges=%d, (MAX:%d)\n", Num_vertices, Max_edges, MAX_EDGES));
	mprintf((0, "Allocated %d K for automap edge list\n", (sizeof(Edge_info) + sizeof(short)) * Max_edges / 1024));

	if ((Current_display_mode != 0 && Current_display_mode != 2) || (Automap_always_hires && MenuHiresAvailable)) 
	{
#if !defined(POLY_ACC)
		gr_set_mode(SM_640x480V);
#endif
		//PA_DFX (pa_set_frontbuffer_current());
		AutomapHires = 1;
	}
	else
	{
		gr_set_mode(SM_320x400U);
		AutomapHires = 0;
	}

	FontHires = AutomapHires;

	create_name_canv();

	gr_palette_clear();

		if (!AutomapHires)
		{
			gr_init_sub_canvas(&Pages[0], grd_curcanv, 0, 0, 320, 400);
			gr_init_sub_canvas(&Pages[1], grd_curcanv, 0, 401, 320, 400);
			gr_init_sub_canvas(&DrawingPages[0], &Pages[0], 16, 69, WINDOW_WIDTH, 272);
			gr_init_sub_canvas(&DrawingPages[1], &Pages[1], 16, 69, WINDOW_WIDTH, 272);

			Automap_background.bm_data = NULL;
			pcx_error = pcx_read_bitmap(MAP_BACKGROUND_FILENAME, &Automap_background, BM_LINEAR, pal);
			if (pcx_error != PCX_ERROR_NONE)
				Error("File %s - PCX error: %s", MAP_BACKGROUND_FILENAME, pcx_errormsg(pcx_error));
			gr_remap_bitmap_good(&Automap_background, pal, -1, -1);

			for (i = 0; i < 2; i++)
			{
				gr_set_current_canvas(&Pages[i]);
				gr_bitmap(0, 0, &Automap_background);
				modex_printf(40, 22, TXT_AUTOMAP, HUGE_FONT, Font_color_20);
				modex_printf(30, 353, TXT_TURN_SHIP, SMALL_FONT, Font_color_20);
				modex_printf(30, 369, TXT_SLIDE_UPDOWN, SMALL_FONT, Font_color_20);
				modex_printf(30, 385, TXT_VIEWING_DISTANCE, SMALL_FONT, Font_color_20);
			}
			if (Automap_background.bm_data)
				free(Automap_background.bm_data);
			Automap_background.bm_data = NULL;

			gr_set_current_canvas(&DrawingPages[current_page]);
		}
		else 
		{
			if (VR_render_buffer[0].cv_w >= 640 && VR_render_buffer[0].cv_h >= 480)
			{
				gr_init_sub_canvas(&Page, &VR_render_buffer[0], 0, 0, 640, 480);
			}
			else 
			{
				void* raw_data;
				MALLOC(raw_data, uint8_t, 640 * 480);
				gr_init_canvas(&Page, (unsigned char*)raw_data, BM_LINEAR, 640, 480);
				must_free_canvas = 1;
			}

			gr_init_sub_canvas(&DrawingPage, &Page, 27, 80, 582, 334);

			gr_set_current_canvas(&Page);

#if defined(POLY_ACC)
			pcx_error = pcx_read_bitmap(MAP_BACKGROUND_FILENAME, &(grd_curcanv->cv_bitmap), BM_LINEAR15, pal);
#else

			pcx_error = pcx_read_bitmap(MAP_BACKGROUND_FILENAME, &(grd_curcanv->cv_bitmap), BM_LINEAR, pal);
			if (pcx_error != PCX_ERROR_NONE)
			{
				//printf("File %s - PCX error: %s",MAP_BACKGROUND_FILENAME,pcx_errormsg(pcx_error));
				Error("File %s - PCX error: %s", MAP_BACKGROUND_FILENAME, pcx_errormsg(pcx_error));
				return;
			}

			gr_remap_bitmap_good(&(grd_curcanv->cv_bitmap), pal, -1, -1);
#endif

			gr_set_curfont(HUGE_FONT);
			gr_set_fontcolor(BM_XRGB(20, 20, 20), -1);
			gr_printf(80, 36, TXT_AUTOMAP, HUGE_FONT);
			gr_set_curfont(SMALL_FONT);
			gr_set_fontcolor(BM_XRGB(20, 20, 20), -1);
			gr_printf(60, 426, TXT_TURN_SHIP);
			gr_printf(60, 443, TXT_SLIDE_UPDOWN);
			gr_printf(60, 460, TXT_VIEWING_DISTANCE);

			gr_set_current_canvas(&DrawingPage);
		}

		automap_build_edge_list();

		if (ViewDist == 0)
			ViewDist = ZOOM_DEFAULT;
		ViewMatrix = Objects[Players[Player_num].objnum].orient;

		tangles.p = PITCH_DEFAULT;
		tangles.h = 0;
		tangles.b = 0;

		done = 0;

		view_target = Objects[Players[Player_num].objnum].pos;

		t1 = entry_time = timer_get_fixed_seconds();
		t2 = t1;

		//Fill in Automap_visited from Objects[Players[Player_num].objnum].segnum
		Max_segments_away = set_segment_depths(Objects[Players[Player_num].objnum].segnum, Automap_visited);
		SegmentLimit = Max_segments_away;

		adjust_segment_limit(SegmentLimit);

		uint64_t startTime;

		while (!done)
		{
			startTime = I_GetUS();
			if (leave_mode == 0 && Controls.automap_state && (timer_get_fixed_seconds() - entry_time) > LEAVE_TIME)
				leave_mode = 1;

			if (!Controls.automap_state && (leave_mode == 1))
				done = 1;

			if (!pause_game)
			{
				uint16_t old_wiggle;
				saved_control_info = Controls;				// Save controls so we can zero them
				memset(&Controls, 0, sizeof(control_info));	// Clear everything...
				old_wiggle = ConsoleObject->mtype.phys_info.flags & PF_WIGGLE;	// Save old wiggle
				ConsoleObject->mtype.phys_info.flags &= ~PF_WIGGLE;		// Turn off wiggle
#ifdef NETWORK
				if (multi_menu_poll())
					done = 1;
#endif
				//			GameLoop( 0, 0 );		// Do game loop with no rendering and no reading controls.
				ConsoleObject->mtype.phys_info.flags |= old_wiggle;	// Restore wiggle
				Controls = saved_control_info;
			}

			controls_read_all();

			if (Controls.automap_down_count)
			{
				if (leave_mode == 0)
					done = 1;
				c = 0;
			}

			//see if redbook song needs to be restarted
			songs_check_redbook_repeat();

			while ((c = key_inkey()))
			{
				switch (c) 
				{
#ifndef NDEBUG
				case KEY_BACKSP: Int3(); break;
#endif

				case KEY_PRINT_SCREEN:
				{
					if (AutomapHires)
					{
							gr_set_current_canvas(NULL);
					}
					else
						gr_set_current_canvas(&Pages[current_page]);
					save_screen_shot(1);
					break;
				}

				case KEY_ESC:
					if (leave_mode == 0)
						done = 1;
					break;

#ifndef NDEBUG
				case KEY_DEBUGGED + KEY_F: 
				{
					for (i = 0; i <= Highest_segment_index; i++)
						Automap_visited[i] = 1;
					automap_build_edge_list();
					Max_segments_away = set_segment_depths(Objects[Players[Player_num].objnum].segnum, Automap_visited);
					SegmentLimit = Max_segments_away;
					adjust_segment_limit(SegmentLimit);
				}
										 break;
#endif

				case KEY_MINUS:
					if (SegmentLimit > 1) 
					{
						SegmentLimit--;
						adjust_segment_limit(SegmentLimit);
					}
					break;
				case KEY_EQUAL:
					if (SegmentLimit < Max_segments_away) 
					{
						SegmentLimit++;
						adjust_segment_limit(SegmentLimit);
					}
					break;
				case KEY_1:
				case KEY_2:
				case KEY_3:
				case KEY_4:
				case KEY_5:
				case KEY_6:
				case KEY_7:
				case KEY_8:
				case KEY_9:
				case KEY_0:
					if (Game_mode & GM_MULTI)
						maxdrop = 2;
					else
						maxdrop = 9;

					marker_num = c - KEY_1;
					if (marker_num <= maxdrop)
					{
						if (MarkerObject[marker_num] != -1)
							HighlightMarker = marker_num;
					}
					break;

				case KEY_D + KEY_CTRLED:
					if (current_page)		//menu will only work on page 0
						draw_automap();	//..so switch from 1 to 0

					if (HighlightMarker > -1 && MarkerObject[HighlightMarker] != -1) 
					{
						gr_set_current_canvas(&Pages[current_page]);

						if (nm_messagebox(NULL, 2, TXT_YES, TXT_NO, "Delete Marker?") == 0)
						{
							obj_delete(MarkerObject[HighlightMarker]);
							MarkerObject[HighlightMarker] = -1;
							MarkerMessage[HighlightMarker][0] = 0;
							HighlightMarker = -1;
						}
					}
					break;

#ifndef RELEASE
				case KEY_COMMA:
					if (MarkerScale > .5)
						MarkerScale -= .5;
					break;
				case KEY_PERIOD:
					if (MarkerScale < 30.0)
						MarkerScale += .5;
#endif

				}
			}

			if (Controls.fire_primary_down_count) 
			{
				// Reset orientation
				ViewDist = ZOOM_DEFAULT;
				tangles.p = PITCH_DEFAULT;
				tangles.h = 0;
				tangles.b = 0;
				view_target = Objects[Players[Player_num].objnum].pos;
			}

			ViewDist -= Controls.forward_thrust_time * ZOOM_SPEED_FACTOR;

			tangles.p += fixdiv(Controls.pitch_time, ROT_SPEED_DIVISOR);
			tangles.h += fixdiv(Controls.heading_time, ROT_SPEED_DIVISOR);
			tangles.b += fixdiv(Controls.bank_time, ROT_SPEED_DIVISOR * 2);

			if (Controls.vertical_thrust_time || Controls.sideways_thrust_time)
			{
				vms_angvec	tangles1;
				vms_vector	old_vt;
				old_vt = view_target;
				tangles1 = tangles;
				vm_angles_2_matrix(&tempm, &tangles1);
				vm_matrix_x_matrix(&ViewMatrix, &Objects[Players[Player_num].objnum].orient, &tempm);
				vm_vec_scale_add2(&view_target, &ViewMatrix.uvec, Controls.vertical_thrust_time * SLIDE_SPEED);
				vm_vec_scale_add2(&view_target, &ViewMatrix.rvec, Controls.sideways_thrust_time * SLIDE_SPEED);
				if (vm_vec_dist_quick(&view_target, &Objects[Players[Player_num].objnum].pos) > i2f(1000))
				{
					view_target = old_vt;
				}
			}

			vm_angles_2_matrix(&tempm, &tangles);
			vm_matrix_x_matrix(&ViewMatrix, &Objects[Players[Player_num].objnum].orient, &tempm);

			if (ViewDist < ZOOM_MIN_VALUE) ViewDist = ZOOM_MIN_VALUE;
			if (ViewDist > ZOOM_MAX_VALUE) ViewDist = ZOOM_MAX_VALUE;

			draw_automap();

			if (first_time)
			{
				first_time = 0;
				gr_palette_load(gr_palette);
			}

			I_DrawCurrentCanvas(0);
			I_DoEvents();
			//[ISB] framerate limiter 
			//waiting loop for polled fps mode
			//With suggestions from dpjudas.
			uint64_t numUS = 1000000 / FPSLimit;
			//[ISB] Combine a sleep with the polling loop to try to spare CPU cycles
			uint64_t diff = (startTime + numUS) - I_GetUS();
			if (diff > 2000) //[ISB] Sleep only if there's sufficient time to do so, since the scheduler isn't precise enough
				I_DelayUS(diff - 2000);
			while (I_GetUS() < startTime + numUS);

			t2 = timer_get_fixed_seconds();
			if (pause_game)
				FrameTime = t2 - t1;
			t1 = t2;
		}

		//free(Edges);
		//free(DrawingListBright);

		gr_free_canvas(name_canv_left);  name_canv_left = NULL;
		gr_free_canvas(name_canv_right);  name_canv_right = NULL;

		if (must_free_canvas)
		{
			free(Page.cv_bitmap.bm_data);
		}

		mprintf((0, "Automap memory freed\n"));

		game_flush_inputs();

		if (pause_game)
		{
			start_time();
			digi_resume_digi_sounds();
		}

		Automap_active = 0;
}

void adjust_segment_limit(int SegmentLimit)
{
	int i, e1;
	Edge_info* e;

	mprintf((0, "Seglimit: %d\n", SegmentLimit));

	for (i = 0; i <= Highest_edge_index; i++) {
		e = &Edges[i];
		e->flags |= EF_TOO_FAR;
		for (e1 = 0; e1 < e->num_faces; e1++) {
			if (Automap_visited[e->segnum[e1]] <= SegmentLimit) {
				e->flags &= (~EF_TOO_FAR);
				break;
			}
		}
	}

}

void draw_all_edges()
{
	g3s_codes cc;
	int i, j, nbright;
	uint8_t nfacing, nnfacing;
	Edge_info* e;
	vms_vector* tv1;
	fix distance;
	fix min_distance = 0x7fffffff;
	g3s_point* p1, * p2;


	nbright = 0;

	for (i = 0; i <= Highest_edge_index; i++) 
	{
		//e = &Edges[Edge_used_list[i]];
		e = &Edges[i];
		if (!(e->flags & EF_USED)) continue;

		if (e->flags & EF_TOO_FAR) continue;

		if (e->flags & EF_FRONTIER) {						// A line that is between what we have seen and what we haven't
			if ((!(e->flags & EF_SECRET)) && (e->color == Wall_normal_color))
				continue;		// If a line isn't secret and is normal color, then don't draw it
		}

		cc = rotate_list(2, e->verts);
		distance = Segment_points[e->verts[1]].p3_z;

		if (min_distance > distance)
			min_distance = distance;

		if (!cc.high) //all off screen?
		{
			nfacing = nnfacing = 0;
			tv1 = &Vertices[e->verts[0]];
			j = 0;
			while (j < e->num_faces && (nfacing == 0 || nnfacing == 0)) {
#ifdef COMPACT_SEGS
				vms_vector temp_v;
				get_side_normal(&Segments[e->segnum[j]], e->sides[j], 0, &temp_v);
				if (!g3_check_normal_facing(tv1, &temp_v))
#else
				if (!g3_check_normal_facing(tv1, &Segments[e->segnum[j]].sides[e->sides[j]].normals[0]))
#endif
					nfacing++;
				else
					nnfacing++;
				j++;
			}

			if (nfacing && nnfacing) {
				// a contour line
				DrawingListBright[nbright++] = e - Edges;
			}
			else if (e->flags & (EF_DEFINING | EF_GRATE)) {
				if (nfacing == 0) {
					if (e->flags & EF_NO_FADE)
						gr_setcolor(e->color);
					else
						gr_setcolor(gr_fade_table[e->color + 256 * 8]);
					g3_draw_line(&Segment_points[e->verts[0]], &Segment_points[e->verts[1]]);
				}
				else {
					DrawingListBright[nbright++] = e - Edges;
				}
			}
		}
	}

	///	mprintf( (0, "Min distance=%.2f, ViewDist=%.2f, Delta=%.2f\n", f2fl(min_distance), f2fl(ViewDist), f2fl(min_distance)- f2fl(ViewDist) ));

	if (min_distance < 0) min_distance = 0;

	// Sort the bright ones using a shell sort
	{
		int t;
		int i, j, incr, v1, v2;

		incr = nbright / 2;
		while (incr > 0) {
			for (i = incr; i < nbright; i++) {
				j = i - incr;
				while (j >= 0) {
					// compare element j and j+incr
					v1 = Edges[DrawingListBright[j]].verts[0];
					v2 = Edges[DrawingListBright[j + incr]].verts[0];

					if (Segment_points[v1].p3_z < Segment_points[v2].p3_z) {
						// If not in correct order, them swap 'em
						t = DrawingListBright[j + incr];
						DrawingListBright[j + incr] = DrawingListBright[j];
						DrawingListBright[j] = t;
						j -= incr;
					}
					else
						break;
				}
			}
			incr = incr / 2;
		}
	}

	// Draw the bright ones
	for (i = 0; i < nbright; i++) 
	{
		int color;
		fix dist;
		e = &Edges[DrawingListBright[i]];
		p1 = &Segment_points[e->verts[0]];
		p2 = &Segment_points[e->verts[1]];
		dist = p1->p3_z - min_distance;
		// Make distance be 1.0 to 0.0, where 0.0 is 10 segments away;
		if (dist < 0) dist = 0;
		if (dist >= Automap_farthest_dist) continue;

		if (e->flags & EF_NO_FADE) 
		{
			gr_setcolor(e->color);
		}
		else 
		{
			dist = F1_0 - fixdiv(dist, Automap_farthest_dist);
			color = f2i(dist * 31);
			gr_setcolor(gr_fade_table[e->color + color * 256]);
		}
		g3_draw_line(p1, p2);
	}

}


//==================================================================
//
// All routines below here are used to build the Edge list
//
//==================================================================


//finds edge, filling in edge_ptr. if found old edge, returns index, else return -1
static int automap_find_edge(int v0, int v1, Edge_info** edge_ptr)
{
	int32_t vv, evv;
	short hash, oldhash;
	int ret, ev0, ev1;

	vv = (v1 << 16) + v0;

	oldhash = hash = ((v0 * 5 + v1) % Max_edges);

	ret = -1;

	while (ret == -1) {
		ev0 = (int)(Edges[hash].verts[0]);
		ev1 = (int)(Edges[hash].verts[1]);
		evv = (ev1 << 16) + ev0;
		if (Edges[hash].num_faces == 0) ret = 0;
		else if (evv == vv) ret = 1;
		else {
			if (++hash == Max_edges) hash = 0;
			if (hash == oldhash) Error("Edge list full!");
		}
	}

	*edge_ptr = &Edges[hash];

	if (ret == 0)
		return -1;
	else
		return hash;

}


void add_one_edge(short va, short vb, uint8_t color, uint8_t side, short segnum, int hidden, int grate, int no_fade) {
	int found;
	Edge_info* e;
	short tmp;

	if (Num_edges >= Max_edges) 
	{
		// GET JOHN! (And tell him that his 
		// MAX_EDGES_FROM_VERTS formula is hosed.)
		// If he's not around, save the mine, 
		// and send him  mail so he can look 
		// at the mine later. Don't modify it.
		// This is important if this happens.
		Int3();		// LOOK ABOVE!!!!!!
		return;
	}

	if (va > vb) 
	{
		tmp = va;
		va = vb;
		vb = tmp;
	}

	found = automap_find_edge(va, vb, &e);

	if (found == -1) 
	{
		e->verts[0] = va;
		e->verts[1] = vb;
		e->color = color;
		e->num_faces = 1;
		e->flags = EF_USED | EF_DEFINING;			// Assume a normal line
		e->sides[0] = side;
		e->segnum[0] = segnum;
		//Edge_used_list[Num_edges] = e-Edges;
		if ((e - Edges) > Highest_edge_index)
			Highest_edge_index = e - Edges;
		Num_edges++;
	}
	else 
	{
		//Assert(e->num_faces < 8 );

		if (color != Wall_normal_color)
			if (color != Wall_revealed_color)
				e->color = color;

		if (e->num_faces < 4) 
		{
			e->sides[e->num_faces] = side;
			e->segnum[e->num_faces] = segnum;
			e->num_faces++;
		}
	}

	if (grate)
		e->flags |= EF_GRATE;

	if (hidden)
		e->flags |= EF_SECRET;		// Mark this as a hidden edge
	if (no_fade)
		e->flags |= EF_NO_FADE;
}

void add_one_unknown_edge(short va, short vb) 
{
	int found;
	Edge_info* e;
	short tmp;

	if (va > vb) {
		tmp = va;
		va = vb;
		vb = tmp;
	}

	found = automap_find_edge(va, vb, &e);
	if (found != -1)
		e->flags |= EF_FRONTIER;		// Mark as a border edge
}

#ifndef _GAMESEQ_H
extern obj_position Player_init[];
#endif

void add_segment_edges(segment* seg)
{
	int 	is_grate, no_fade;
	uint8_t	color;
	int	sn;
	int	segnum = seg - Segments;
	int	hidden_flag;
	int ttype, trigger_num;

	for (sn = 0; sn < MAX_SIDES_PER_SEGMENT; sn++) 
	{
		short	vertex_list[4];

		hidden_flag = 0;

		is_grate = 0;
		no_fade = 0;

		color = 255;
		if (seg->children[sn] == -1) 
		{
			color = Wall_normal_color;
		}

		switch (Segment2s[segnum].special) 
		{
		case SEGMENT_IS_FUELCEN:
			color = BM_XRGB(29, 27, 13);
			break;
		case SEGMENT_IS_CONTROLCEN:
			if (Control_center_present)
				color = BM_XRGB(29, 0, 0);
			break;
		case SEGMENT_IS_ROBOTMAKER:
			color = BM_XRGB(29, 0, 31);
			break;
		}

		if (seg->sides[sn].wall_num > -1)
		{

			trigger_num = Walls[seg->sides[sn].wall_num].trigger;
			ttype = Triggers[trigger_num].type;
			if (ttype == TT_SECRET_EXIT)
			{
				color = BM_XRGB(29, 0, 31);
				no_fade = 1;
				goto Here;
			}

			switch (Walls[seg->sides[sn].wall_num].type) 
			{
			case WALL_DOOR:
				if (Walls[seg->sides[sn].wall_num].keys == KEY_BLUE)
				{
					no_fade = 1;
					color = Wall_door_blue;
					//mprintf((0, "Seg %i, side %i has BLUE wall\n", segnum, sn));
				}
				else if (Walls[seg->sides[sn].wall_num].keys == KEY_GOLD) 
				{
					no_fade = 1;
					color = Wall_door_gold;
					//mprintf((0, "Seg %i, side %i has GOLD wall\n", segnum, sn));
				}
				else if (Walls[seg->sides[sn].wall_num].keys == KEY_RED) 
				{
					no_fade = 1;
					color = Wall_door_red;
					//mprintf((0, "Seg %i, side %i has RED wall\n", segnum, sn));
				}
				else if (!(WallAnims[Walls[seg->sides[sn].wall_num].clip_num].flags & WCF_HIDDEN))
				{
					int	connected_seg = seg->children[sn];
					if (connected_seg != -1) {
						int connected_side = find_connect_side(seg, &Segments[connected_seg]);
						int	keytype = Walls[Segments[connected_seg].sides[connected_side].wall_num].keys;
						if ((keytype != KEY_BLUE) && (keytype != KEY_GOLD) && (keytype != KEY_RED))
							color = Wall_door_color;
						else {
							switch (Walls[Segments[connected_seg].sides[connected_side].wall_num].keys) 
							{
							case KEY_BLUE:	color = Wall_door_blue;	no_fade = 1; break;
							case KEY_GOLD:	color = Wall_door_gold;	no_fade = 1; break;
							case KEY_RED:	color = Wall_door_red;	no_fade = 1; break;
							default:	Error("Inconsistent data.  Supposed to be a colored wall, but not blue, gold or red.\n");
							}
							//mprintf((0, "Seg %i, side %i has a colored door on the other side.\n", segnum, sn));
						}

					}
				}
				else 
				{
					color = Wall_normal_color;
					hidden_flag = 1;
					//mprintf((0, "Wall at seg:side %i:%i is hidden.\n", seg-Segments, sn));
				}
				break;
			case WALL_CLOSED:
				// Make grates draw properly
				if (WALL_IS_DOORWAY(seg, sn) & WID_RENDPAST_FLAG)
					is_grate = 1;
				else
					hidden_flag = 1;
				color = Wall_normal_color;
				break;
			case WALL_BLASTABLE:
				// Hostage doors
				color = Wall_door_color;
				break;
			}
		}

		if (segnum == Player_init[Player_num].segnum)
			color = BM_XRGB(31, 0, 31);

		if (color != 255) 
		{
			// If they have a map powerup, draw unvisited areas in dark blue.
			if (Players[Player_num].flags & PLAYER_FLAGS_MAP_ALL && (!Automap_visited[segnum]))
				color = Wall_revealed_color;
		Here:

			get_side_verts(vertex_list, segnum, sn);
			add_one_edge(vertex_list[0], vertex_list[1], color, sn, segnum, hidden_flag, 0, no_fade);
			add_one_edge(vertex_list[1], vertex_list[2], color, sn, segnum, hidden_flag, 0, no_fade);
			add_one_edge(vertex_list[2], vertex_list[3], color, sn, segnum, hidden_flag, 0, no_fade);
			add_one_edge(vertex_list[3], vertex_list[0], color, sn, segnum, hidden_flag, 0, no_fade);

			if (is_grate) 
			{
				add_one_edge(vertex_list[0], vertex_list[2], color, sn, segnum, hidden_flag, 1, no_fade);
				add_one_edge(vertex_list[1], vertex_list[3], color, sn, segnum, hidden_flag, 1, no_fade);
			}
		}
	}

}


// Adds all the edges from a segment we haven't visited yet.

void add_unknown_segment_edges(segment* seg)
{
	int sn;
	int segnum = seg - Segments;

	for (sn = 0; sn < MAX_SIDES_PER_SEGMENT; sn++)
	{
		short	vertex_list[4];

		// Only add edges that have no children
		if (seg->children[sn] == -1) 
		{
			get_side_verts(vertex_list, segnum, sn);

			add_one_unknown_edge(vertex_list[0], vertex_list[1]);
			add_one_unknown_edge(vertex_list[1], vertex_list[2]);
			add_one_unknown_edge(vertex_list[2], vertex_list[3]);
			add_one_unknown_edge(vertex_list[3], vertex_list[0]);
		}
	}
}

void automap_build_edge_list()
{
	int	i, e1, e2, s;
	Edge_info* e;

	Automap_cheat = 0;

	if (Players[Player_num].flags & PLAYER_FLAGS_MAP_ALL_CHEAT)
		Automap_cheat = 1;		// Damn cheaters...

	// clear edge list
	for (i = 0; i < Max_edges; i++) 
	{
		Edges[i].num_faces = 0;
		Edges[i].flags = 0;
	}
	Num_edges = 0;
	Highest_edge_index = -1;

	if (Automap_cheat || (Players[Player_num].flags & PLAYER_FLAGS_MAP_ALL)) 
	{
		// Cheating, add all edges as visited
		for (s = 0; s <= Highest_segment_index; s++)
#ifdef EDITOR
			if (Segments[s].segnum != -1)
#endif
			{
				add_segment_edges(&Segments[s]);
			}
	}
	else 
	{
		// Not cheating, add visited edges, and then unvisited edges
		for (s = 0; s <= Highest_segment_index; s++)
#ifdef EDITOR
			if (Segments[s].segnum != -1)
#endif
				if (Automap_visited[s]) 
				{
					add_segment_edges(&Segments[s]);
				}

		for (s = 0; s <= Highest_segment_index; s++)
#ifdef EDITOR
			if (Segments[s].segnum != -1)
#endif
				if (!Automap_visited[s]) 
				{
					add_unknown_segment_edges(&Segments[s]);
				}
	}

	// Find unnecessary lines (These are lines that don't have to be drawn because they have small curvature)
	for (i = 0; i <= Highest_edge_index; i++) 
	{
		e = &Edges[i];
		if (!(e->flags & EF_USED)) continue;

		for (e1 = 0; e1 < e->num_faces; e1++) 
		{
			for (e2 = 1; e2 < e->num_faces; e2++) 
			{
				if ((e1 != e2) && (e->segnum[e1] != e->segnum[e2]))
				{
					if (vm_vec_dot(&Segments[e->segnum[e1]].sides[e->sides[e1]].normals[0], &Segments[e->segnum[e2]].sides[e->sides[e2]].normals[0]) > (F1_0 - (F1_0 / 10))) 
					{
						e->flags &= (~EF_DEFINING);
						break;
					}
				}
			}
			if (!(e->flags & EF_DEFINING))
				break;
		}
	}

	mprintf((0, "Automap used %d / %d edges\n", Num_edges, Max_edges));

}

char Marker_input[40];
int Marker_index = 0;
uint8_t DefiningMarkerMessage = 0;
uint8_t MarkerBeingDefined;
uint8_t LastMarkerDropped;

void InitMarkerInput()
{
	int maxdrop, i;

	//find free marker slot

	if (Game_mode & GM_MULTI)
		maxdrop = MAX_DROP_MULTI;
	else
		maxdrop = MAX_DROP_SINGLE;

	for (i = 0; i < maxdrop; i++)
		if (MarkerObject[(Player_num * 2) + i] == -1)		//found free slot!
			break;

	if (i == maxdrop)		//no free slot
		if (Game_mode & GM_MULTI)
			i = !LastMarkerDropped;		//in multi, replace older of two
		else 
		{
			HUD_init_message("No free marker slots");
			return;
		}

	//got a free slot.  start inputting marker message

	Marker_input[0] = 0;
	Marker_index = 0;
	DefiningMarkerMessage = 1;
	MarkerBeingDefined = i;
}

void MarkerInputMessage(int key)
{
	switch (key)
	{
	case KEY_F8:
	case KEY_ESC:
		DefiningMarkerMessage = 0;
		game_flush_inputs();
		break;
	case KEY_LEFT:
	case KEY_BACKSP:
	case KEY_PAD4:
		if (Marker_index > 0)
			Marker_index--;
		Marker_input[Marker_index] = 0;
		break;
	case KEY_ENTER:
		strcpy(MarkerMessage[(Player_num * 2) + MarkerBeingDefined], Marker_input);
		if (Game_mode & GM_MULTI)
			strcpy(MarkerOwner[(Player_num * 2) + MarkerBeingDefined], Players[Player_num].callsign);
		DropMarker(MarkerBeingDefined);
		LastMarkerDropped = MarkerBeingDefined;
		game_flush_inputs();
		DefiningMarkerMessage = 0;
		break;
	default:
		if (key > 0)
		{
			int ascii = key_to_ascii(key);
			if ((ascii < 255))
				if (Marker_index < 38)
				{
					Marker_input[Marker_index++] = ascii;
					Marker_input[Marker_index] = 0;
				}
		}
		break;

	}
}




