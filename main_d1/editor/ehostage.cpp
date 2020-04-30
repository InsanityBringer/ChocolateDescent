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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "main_d1/screens.h"
#include "main_d1/inferno.h"
#include "main_d1/segment.h"
#include "editor.h"

#include "platform/timer.h"
#include "objpage.h"
#include "fix/fix.h"
#include "platform/mono.h"
#include "misc/error.h"
#include "kdefs.h"
#include	"main_d1/object.h"
#include "main_d1/polyobj.h"
#include "main_d1/game.h"
#include "main_d1/powerup.h"
#include "main_d1/ai.h"
#include "main_d1/hostage.h"
#include "eobject.h"
#include "medwall.h"
#include "eswitch.h"
#include "medrobot.h"
#include "platform/key.h"
#include "main_d1/bm.h"
#include "main_d1/sounds.h"
#include "centers.h"
#include "main_d1/piggy.h"

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
static UI_WINDOW 				*MainWindow = NULL;
static UI_GADGET_USERBOX	*HostageViewBox;
static UI_GADGET_INPUTBOX	*HostageText;
static UI_GADGET_BUTTON 	*QuitButton;
static int						CurrentHostageIndex=-1;
static int						LastHostageIndex=-1;

static fix 			Vclip_animation_time=0;			// How long the rescue sequence has been playing
static fix 			Vclip_playback_speed=0;				// Calculated internally.  Frames/second of vclip.
static vclip 		*Vclip_ptr = NULL;				// Used for the vclip on monitor

void vclip_play( vclip * vc, fix frame_time )	
{
	int bitmapnum;

	if ( vc == NULL )
		return;

	if ( vc != Vclip_ptr )	{
		// Start new vclip
		Vclip_ptr = vc;
		Vclip_animation_time = 1;

		// Calculate the frame/second of the playback
		Vclip_playback_speed = fixdiv(i2f(Vclip_ptr->num_frames),Vclip_ptr->play_time);
	}

	if ( Vclip_animation_time <= 0 )
		return;

	// Find next bitmap in the vclip
	bitmapnum = f2i(Vclip_animation_time);

	// Check if vclip is done playing.
	if (bitmapnum >= Vclip_ptr->num_frames)		{
		Vclip_animation_time	= 1;											// Restart this vclip
		bitmapnum = 0;
	}

	PIGGY_PAGE_IN( Vclip_ptr->frames[bitmapnum] );
	gr_bitmap(0,0,&GameBitmaps[Vclip_ptr->frames[bitmapnum].index] );
	
	Vclip_animation_time += fixmul(frame_time, Vclip_playback_speed );
}



static char HostageMessage[]  = "  ";

static fix Time;

int SelectPrevHostage()	{
	int start=0;

	do	{
		CurrentHostageIndex--;
		if ( CurrentHostageIndex < 0 ) CurrentHostageIndex = MAX_HOSTAGES-1;
		start++;
		if ( start > MAX_HOSTAGES ) break;
	} while ( !hostage_is_valid( CurrentHostageIndex ) );

	if (hostage_is_valid( CurrentHostageIndex ) )	{
		Cur_object_index = Hostages[CurrentHostageIndex].objnum;
	} else {
		CurrentHostageIndex =-1;
	}
	
	return CurrentHostageIndex;
}


int SelectNextHostage()	{
	int start=0;

	do	{
		CurrentHostageIndex++;
		if ( CurrentHostageIndex >= MAX_HOSTAGES ) CurrentHostageIndex = 0;
		start++;
		if ( start > MAX_HOSTAGES ) break;
	} while ( !hostage_is_valid( CurrentHostageIndex ) );

	if (hostage_is_valid( CurrentHostageIndex ) )	{
		Cur_object_index = Hostages[CurrentHostageIndex].objnum;
	} else {
		CurrentHostageIndex =-1;
	}
	
	return CurrentHostageIndex;
}


int SelectClosestHostage()	{
	int start=0;

	while ( !hostage_is_valid( CurrentHostageIndex ) )	{
		CurrentHostageIndex++;
		if ( CurrentHostageIndex >= MAX_HOSTAGES ) CurrentHostageIndex = 0;
		start++;
		if ( start > MAX_HOSTAGES ) break;
	}

	if (hostage_is_valid( CurrentHostageIndex ) )	{
		Cur_object_index = Hostages[CurrentHostageIndex].objnum;
	} else {
		CurrentHostageIndex =-1;
	}
	
	return CurrentHostageIndex;
}


int PlaceHostage()	{
	int ctype,i;
	vms_vector	cur_object_loc;

	//update_due_to_new_segment();
	compute_segment_center(&cur_object_loc, Cursegp);

	ctype = -1;
	for (i=0; i<Num_total_object_types; i++ )	{
		if (ObjType[i] == OL_HOSTAGE )	{
			ctype = i;	
			break;
		}
	}

	Assert( ctype != -1 );

	if (place_object(Cursegp, &cur_object_loc, ctype )==0)	{
		Int3();		// Debug below
		i=place_object(Cursegp, &cur_object_loc, ctype );
		return 1;
	}

	if (hostage_object_is_valid( Cur_object_index ) )	{
		CurrentHostageIndex	= Objects[Cur_object_index].id;
	} else {
		Int3();		// Get John! (Object should be valid)
		i=hostage_object_is_valid( Cur_object_index );	// For debugging only
	}

	return 0;
}

int CompressHostages()
{
	hostage_compress_all();

	return 0;
}

//@@int SelectPrevVclip()	{
//@@	if (!hostage_is_valid( CurrentHostageIndex ) )	
//@@		return 0;
//@@
//@@	if ( Hostages[CurrentHostageIndex].type == 0 )
//@@		Hostages[CurrentHostageIndex].type = N_hostage_types-1;
//@@	else
//@@		Hostages[CurrentHostageIndex].type--;
//@@	
//@@	if ( Hostages[CurrentHostageIndex].type >= N_hostage_types )
//@@		Hostages[CurrentHostageIndex].type = 0;
//@@	
//@@	return 1;
//@@}
//@@
//@@int SelectNextVclip()	{
//@@	if (!hostage_is_valid( CurrentHostageIndex ) )	
//@@		return 0;
//@@
//@@	Hostages[CurrentHostageIndex].type++;
//@@	if ( Hostages[CurrentHostageIndex].type >= N_hostage_types )
//@@		Hostages[CurrentHostageIndex].type = 0;
//@@
//@@	return 1;
//@@}

int SelectNextFace()
{
	int start = Hostages[CurrentHostageIndex].vclip_num;
	
	if (!hostage_is_valid( CurrentHostageIndex ) )	
		return 0;

	do {
		Hostages[CurrentHostageIndex].vclip_num++;
		if ( Hostages[CurrentHostageIndex].vclip_num >= MAX_HOSTAGES)
			Hostages[CurrentHostageIndex].vclip_num = 0;

		if (Hostages[CurrentHostageIndex].vclip_num == start)
			return 0;

	} while (Hostage_face_clip[Hostages[CurrentHostageIndex].vclip_num].num_frames == 0);

	return 1;
}

int SelectPrevFace()
{
	int start = Hostages[CurrentHostageIndex].vclip_num;
	
	if (!hostage_is_valid( CurrentHostageIndex ) )	
		return 0;

	do {
		Hostages[CurrentHostageIndex].vclip_num--;
		if ( Hostages[CurrentHostageIndex].vclip_num < 0)
			Hostages[CurrentHostageIndex].vclip_num = MAX_HOSTAGES-1;

		if (Hostages[CurrentHostageIndex].vclip_num == start)
			return 0;

	} while (Hostage_face_clip[Hostages[CurrentHostageIndex].vclip_num].num_frames == 0);

	return 1;
}

int PlayHostageSound()	{
	int sound_num;

	if (!hostage_is_valid( CurrentHostageIndex ) )	
		return 0;

	sound_num = Hostage_face_clip[Hostages[CurrentHostageIndex].vclip_num].sound_num;

	if ( sound_num > -1 )	{
		digi_play_sample( sound_num, F1_0 );
	}

	return 1;	
}

//@@int find_next_hostage_sound()	{
//@@	int start=0,n;
//@@
//@@	n = Hostages[CurrentHostageIndex].sound_num;
//@@	do	{
//@@		n++;
//@@		if ( n < SOUND_HOSTAGE_VOICES ) n = SOUND_HOSTAGE_VOICES+MAX_HOSTAGE_SOUNDS-1;
//@@		if ( n >= SOUND_HOSTAGE_VOICES+MAX_HOSTAGE_SOUNDS ) n = SOUND_HOSTAGE_VOICES;
//@@		start++;
//@@		if ( start > MAX_HOSTAGE_SOUNDS ) break;
//@@	} while ( Sounds[n] == NULL );
//@@
//@@	if ( Sounds[n] == NULL )
//@@		Hostages[CurrentHostageIndex].sound_num = -1;
//@@	else	{
//@@		Hostages[CurrentHostageIndex].sound_num = n;
//@@		PlayHostageSound();
//@@	}
//@@	return 1;
//@@}
//@@
//@@int find_prev_hostage_sound()	{
//@@	int start=0,n;
//@@
//@@	n = Hostages[CurrentHostageIndex].sound_num;
//@@	do	{
//@@		n--;
//@@		if ( n < SOUND_HOSTAGE_VOICES ) n = SOUND_HOSTAGE_VOICES+MAX_HOSTAGE_SOUNDS-1;
//@@		if ( n >= SOUND_HOSTAGE_VOICES+MAX_HOSTAGE_SOUNDS ) n = SOUND_HOSTAGE_VOICES;
//@@		start++;
//@@		if ( start > MAX_HOSTAGE_SOUNDS ) break;
//@@	} while ( Sounds[n] == NULL );
//@@
//@@	if ( Sounds[n] == NULL )
//@@		Hostages[CurrentHostageIndex].sound_num = -1;
//@@	else	{
//@@		Hostages[CurrentHostageIndex].sound_num = n;
//@@		PlayHostageSound();
//@@	}
//@@	return 1;
//@@}


//-------------------------------------------------------------------------
// Called from the editor... does one instance of the hostage dialog box
//-------------------------------------------------------------------------
int do_hostage_dialog()
{
	int i;

	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;
	
	// Close other windows
	close_all_windows();

	CurrentHostageIndex = 0;
	SelectClosestHostage();

	// Open a window with a quit button
	MainWindow = ui_open_window( TMAPBOX_X+10, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, WIN_DIALOG );
	QuitButton = ui_add_gadget_button( MainWindow, 20, 222, 48, 40, "Done", NULL );

	ui_wprintf_at( MainWindow, 10, 32,"&Message:" );
	HostageText = ui_add_gadget_inputbox( MainWindow, 10, 50, HOSTAGE_MESSAGE_LEN, HOSTAGE_MESSAGE_LEN, HostageMessage );

	// The little box the hostage vclip will play in.
	HostageViewBox = ui_add_gadget_userbox( MainWindow,10, 90+10, 64, 64 );

	// A bunch of buttons...
	i = 90;
//@@	ui_add_gadget_button( MainWindow,155,i,70, 26, "<< Type", SelectPrevVclip );
//@@	ui_add_gadget_button( MainWindow,155+70,i,70, 26, "Type >>", SelectNextVclip );i += 29;		
//@@	ui_add_gadget_button( MainWindow,155,i,70, 26, "<< Sound",  find_prev_hostage_sound );
//@@	ui_add_gadget_button( MainWindow,155+70,i,70, 26, "Sound >>", find_next_hostage_sound );i += 29;		

	ui_add_gadget_button( MainWindow,155,i,70, 26, "<< Face", SelectPrevFace );
	ui_add_gadget_button( MainWindow,155+70,i,70, 26, "Face >>", SelectNextFace );i += 29;		
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Play sound", PlayHostageSound );i += 29;		
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Next Hostage", SelectNextHostage );	i += 29;		
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Prev Hostage", SelectPrevHostage ); i += 29;		
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Compress All", CompressHostages ); i += 29;		
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Delete", ObjectDelete );	i += 29;		
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Create New", PlaceHostage );	i += 29;		
	
	Time = timer_get_fixed_seconds();

	LastHostageIndex = -2;		// Set to some dummy value so everything works ok on the first frame.
	
//	if ( CurrentHostageIndex == -1 )
//		SelectNextHostage();

	return 1;

}

void hostage_close_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_window( MainWindow );
		MainWindow = NULL;
	}
}

void do_hostage_window()
{
	fix DeltaTime, Temp;

	if ( MainWindow == NULL ) return;

	SelectClosestHostage();

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;
	ui_window_do_gadgets(MainWindow);

	//------------------------------------------------------------
	// If we change objects, we need to reset the ui code for all
	// of the radio buttons that control the ai mode.  Also makes
	// the current AI mode button be flagged as pressed down.
	//------------------------------------------------------------
	if (LastHostageIndex != CurrentHostageIndex )	{

		if ( CurrentHostageIndex > -1 )	
			strcpy( HostageText->text, Hostages[CurrentHostageIndex].text );
		else
			strcpy(HostageText->text, " " );

		HostageText->position = strlen(HostageText->text);
		HostageText->oldposition = HostageText->position;
		HostageText->status=1;
		HostageText->first_time = 1;

	}

	//------------------------------------------------------------
	// If any of the radio buttons that control the mode are set, then
	// update the cooresponding AI state.
	//------------------------------------------------------------
	if ( CurrentHostageIndex > -1 )	
		strcpy( Hostages[CurrentHostageIndex].text, HostageText->text );

	//------------------------------------------------------------
	// A simple frame time counter for spinning the objects...
	//------------------------------------------------------------
	Temp = timer_get_fixed_seconds();
	DeltaTime = Temp - Time;
	Time = Temp;

	//------------------------------------------------------------
	// Redraw the object in the little 64x64 box
	//------------------------------------------------------------
	if (CurrentHostageIndex > -1 )	{
		int vclip_num;
		
		vclip_num = Hostages[CurrentHostageIndex].vclip_num;

		Assert(vclip_num != -1);

		gr_set_current_canvas( HostageViewBox->canvas );

		if ( vclip_num > -1 )	{
			vclip_play( &Hostage_face_clip[vclip_num], DeltaTime );	
		} else {
			gr_clear_canvas( CGREY );
		}
	} else {
		// no hostage, so just blank out
		gr_set_current_canvas( HostageViewBox->canvas );
		gr_clear_canvas( CGREY );
	}

	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this robot.
	//------------------------------------------------------------
	if (ui_button_any_drawn || (LastHostageIndex != CurrentHostageIndex) )	{
		if ( CurrentHostageIndex > -1 )	{
			ui_wprintf_at( MainWindow, 10, 15, "Hostage: %d   Object: %d", CurrentHostageIndex, Hostages[CurrentHostageIndex].objnum );
			//@@ui_wprintf_at( MainWindow, 10, 73, "Type: %d   Sound: %d   ", Hostages[CurrentHostageIndex].type, Hostages[CurrentHostageIndex].sound_num );
			ui_wprintf_at( MainWindow, 10, 73, "Face: %d   ", Hostages[CurrentHostageIndex].vclip_num);
		}	else {
			ui_wprintf_at( MainWindow, 10, 15, "Hostage: none " );
			//@@ui_wprintf_at( MainWindow, 10, 73, "Type:    Sound:       " );
			ui_wprintf_at( MainWindow, 10, 73, "Face:         " );
		}
		Update_flags |= UF_WORLD_CHANGED;
	}

	if ( QuitButton->pressed || (last_keypress==KEY_ESC))	{
		hostage_close_window();
		return;
	}		

	LastHostageIndex = CurrentHostageIndex;
}

#endif
