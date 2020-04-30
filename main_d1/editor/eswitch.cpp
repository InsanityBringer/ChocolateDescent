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

#include "main_d1/inferno.h"
#include "editor.h"
#include "eswitch.h"
#include "main_d1/segment.h"
#include "misc/error.h"
#include "main_d1/gameseg.h"
#include "platform/mono.h"
#include "main_d1/wall.h"
#include "medwall.h"

#include "main_d1/screens.h"

#include "main_d1/textures.h"
#include "main_d1/texmerge.h"
#include "medrobot.h"
#include "platform/timer.h"
#include "platform/key.h"
#include "ehostage.h"
#include "centers.h"
#include "main_d1/piggy.h"

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
#define NUM_TRIGGER_FLAGS 10

static UI_WINDOW 				*MainWindow = NULL;
static UI_GADGET_USERBOX	*WallViewBox;
static UI_GADGET_BUTTON 	*QuitButton;
static UI_GADGET_CHECKBOX	*TriggerFlag[NUM_TRIGGER_FLAGS];

static int old_trigger_num;

//-----------------------------------------------------------------
// Adds a trigger to wall, and returns the trigger number. 
// If there is a trigger already present, it returns the trigger number. (To be replaced)
int add_trigger(segment *seg, short side)
{
	int trigger_num = Num_triggers;
	int wall_num = seg->sides[side].wall_num;

	Assert(trigger_num < MAX_TRIGGERS);
	if (trigger_num>=MAX_TRIGGERS) return -1;

	if (wall_num == -1) {
		wall_add_to_markedside(WALL_OPEN);
		wall_num = seg->sides[side].wall_num;
		Walls[wall_num].trigger = trigger_num;
		
		// Set default values first time trigger is added
		Triggers[trigger_num].flags = 0;
		Triggers[trigger_num].value = F1_0*5;
		Triggers[trigger_num].num_links = 0;
		Triggers[trigger_num].flags &= TRIGGER_ON;		

		Num_triggers++;
		return trigger_num;
	} else {
		if (Walls[wall_num].trigger != -1)
			return Walls[wall_num].trigger;

		// Create new trigger.
		Walls[wall_num].trigger = trigger_num;

		// Set default values first time trigger is added
		Triggers[trigger_num].flags = 0;
		Triggers[trigger_num].value = F1_0*5;
		Triggers[trigger_num].num_links = 0;
		Triggers[trigger_num].flags &= TRIGGER_ON;

		Num_triggers++;
		return trigger_num;
	}
}		

//-----------------------------------------------------------------
// Adds a specific trigger flag to Markedsegp/Markedside if it is possible.
// Automatically adds flag to Connectside if possible unless it is a control trigger.
// Returns 1 if trigger flag added.
// Returns 0 if trigger flag cannot be added.
int trigger_add_to_Markedside(short flag) {
	int trigger_num; //, ctrigger_num;

	if (!Markedsegp) {
		editor_status("No Markedside.");
		return 0;
	}

	// If no child on Markedside return
	if (!IS_CHILD(Markedsegp->children[Markedside])) return 0;

	trigger_num = add_trigger(Markedsegp, Markedside);

	if (trigger_num == -1) {
		editor_status("Cannot add trigger at Markedside.");
		return 0;
	}

 	Triggers[trigger_num].flags |= flag;

	return 1;
}

int trigger_remove_flag_from_Markedside(short flag) {
	int trigger_num; //, ctrigger_num;
	int wall_num;
	
	if (!Markedsegp) {
		editor_status("No Markedside.");
		return 0;
	}

	// If no child on Markedside return
	if (!IS_CHILD(Markedsegp->children[Markedside])) return 0;

	// If no wall just return
	wall_num = Markedsegp->sides[Markedside].wall_num;
	if (wall_num == -1) return 0;

	trigger_num = Walls[wall_num].trigger;

	// If flag is already cleared, then don't change anything.
	if ( trigger_num == -1 ) {
		editor_status("No trigger at Markedside.");
		return 0;
	}

	if (!Triggers[trigger_num].flags & flag)
		return 1;

 	Triggers[trigger_num].flags &= ~flag;

	return 1;
}


int bind_matcen_to_trigger() {

	int wall_num, trigger_num, link_num;
	int i;

	if (!Markedsegp) {
		editor_status("No marked segment.");
		return 0;
	}

	wall_num = Markedsegp->sides[Markedside].wall_num;
	if (wall_num == -1) {
		editor_status("No wall at Markedside.");
		return 0;
	}

	trigger_num = Walls[wall_num].trigger;	

	if (trigger_num == -1) {
		editor_status("No trigger at Markedside.");
		return 0;
	}

	if (!(Cursegp->special & SEGMENT_IS_ROBOTMAKER)) {
		editor_status("No Matcen at Cursegp.");
		return 0;
	}

	link_num = Triggers[trigger_num].num_links;
	for (i=0;i<link_num;i++)
		if (Cursegp-Segments == Triggers[trigger_num].seg[i]) {
			editor_status("Matcen already bound to Markedside.");
			return 0;
		}

	// Error checking completed, actual binding begins
	Triggers[trigger_num].seg[link_num] = Cursegp - Segments;
	Triggers[trigger_num].num_links++;

	mprintf((0, "seg %d linked to link_num %d\n",
				Triggers[trigger_num].seg[link_num], link_num)); 

	editor_status("Matcen linked to trigger");

	return 1;
}


int bind_wall_to_trigger() {

	int wall_num, trigger_num, link_num;
	int i;

	if (!Markedsegp) {
		editor_status("No marked segment.");
		return 0;
	}

	wall_num = Markedsegp->sides[Markedside].wall_num;
	if (wall_num == -1) {
		editor_status("No wall at Markedside.");
		return 0;
	}

	trigger_num = Walls[wall_num].trigger;	

	if (trigger_num == -1) {
		editor_status("No trigger at Markedside.");
		return 0;
	}

	if (Cursegp->sides[Curside].wall_num == -1) {
		editor_status("No wall at Curside.");
		return 0;
	}

	if ((Cursegp==Markedsegp) && (Curside==Markedside)) {
		editor_status("Cannot bind wall to itself.");
		return 0;
	}

	link_num = Triggers[trigger_num].num_links;
	for (i=0;i<link_num;i++)
		if ((Cursegp-Segments == Triggers[trigger_num].seg[i]) && (Curside == Triggers[trigger_num].side[i])) {
			editor_status("Curside already bound to Markedside.");
			return 0;
		}

	// Error checking completed, actual binding begins
	Triggers[trigger_num].seg[link_num] = Cursegp - Segments;
	Triggers[trigger_num].side[link_num] = Curside;
	Triggers[trigger_num].num_links++;

	mprintf((0, "seg %d:side %d linked to link_num %d\n",
				Triggers[trigger_num].seg[link_num], Triggers[trigger_num].side[link_num], link_num)); 

	editor_status("Wall linked to trigger");

	return 1;
}

int remove_trigger(segment *seg, short side)
{    	
	int trigger_num, t, w;

	if (seg->sides[side].wall_num == -1) {
		mprintf((0, "Can't remove trigger from wall_num -1\n"));	
		return 0;
	}

	trigger_num = Walls[seg->sides[side].wall_num].trigger;

	if (trigger_num != -1) {
		Walls[seg->sides[side].wall_num].trigger = -1;
		for (t=trigger_num;t<Num_triggers-1;t++)
			Triggers[t] = Triggers[t+1];
	
		for (w=0; w<Num_walls; w++) {
			if (Walls[w].trigger > trigger_num) 
				Walls[w].trigger--;
		}

		Num_triggers--;
		for (t=0;t<Num_walls;t++)
			if (Walls[seg->sides[side].wall_num].trigger > trigger_num)
				Walls[seg->sides[side].wall_num].trigger--;
		
		return 1;
	}

	editor_status("No trigger to remove");
	return 0;
}


int add_trigger_control()
{
	trigger_add_to_Markedside(TRIGGER_CONTROL_DOORS);
	Update_flags = UF_WORLD_CHANGED;
	return 1;
}

int trigger_remove()
{
	remove_trigger(Markedsegp, Markedside);
	Update_flags = UF_WORLD_CHANGED;
	return 1;
}

int trigger_turn_all_ON()
{
	int t;

	for (t=0;t<Num_triggers;t++)
		Triggers[t].flags &= TRIGGER_ON;
	return 1;
}

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the trigger dialog box
//-------------------------------------------------------------------------
int do_trigger_dialog()
{
	int i;

	if (!Markedsegp) {
		editor_status("Trigger requires Marked Segment & Side.");
		return 0;
	}

	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;

	// Close other windows.	
	robot_close_window();
	close_wall_window();
	close_centers_window();
	hostage_close_window();

	// Open a window with a quit button
	MainWindow = ui_open_window( TMAPBOX_X+20, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, WIN_DIALOG );

	// These are the checkboxes for each door flag.
	i = 44;
	TriggerFlag[0] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Door Control" );  	i+=22;
	TriggerFlag[1] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Shield damage" ); 	i+=22;
	TriggerFlag[2] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Energy drain" );		i+=22;
	TriggerFlag[3] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Exit" );					i+=22;
	TriggerFlag[4] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "One-shot" );			i+=22;
	TriggerFlag[5] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Illusion ON" );		i+=22;
	TriggerFlag[6] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Illusion OFF" );		i+=22;
	TriggerFlag[7] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Trigger ON" );			i+=22;
	TriggerFlag[8] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Matcen Trigger" ); 	i+=22;
	TriggerFlag[9] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Secret Exit" ); 		i+=22;

	QuitButton = ui_add_gadget_button( MainWindow, 20, i, 48, 40, "Done", NULL );
																				 
	// The little box the wall will appear in.
	WallViewBox = ui_add_gadget_userbox( MainWindow, 155, 5, 64, 64 );

	// A bunch of buttons...
	i = 80;
//	ui_add_gadget_button( MainWindow,155,i,140, 26, "Add Door Control", add_trigger_control ); i += 29;
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Remove Trigger", trigger_remove ); i += 29;
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Bind Wall", bind_wall_to_trigger ); i += 29;
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Bind Matcen", bind_matcen_to_trigger ); i += 29;
	ui_add_gadget_button( MainWindow,155,i,140, 26, "All Triggers ON", trigger_turn_all_ON ); i += 29;

	old_trigger_num = -2;		// Set to some dummy value so everything works ok on the first frame.

	return 1;
}

void close_trigger_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_window( MainWindow );
		MainWindow = NULL;
	}
}

void do_trigger_window()
{
	int i;
	short Markedwall, trigger_num;

	if ( MainWindow == NULL ) return;
	if (!Markedsegp) {
		close_trigger_window();
		return;
	}

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;
	ui_window_do_gadgets(MainWindow);
	
	//------------------------------------------------------------
	// If we change walls, we need to reset the ui code for all
	// of the checkboxes that control the wall flags.  
	//------------------------------------------------------------
	Markedwall = Markedsegp->sides[Markedside].wall_num;
	if (Markedwall != -1)
		trigger_num = Walls[Markedwall].trigger;
	else trigger_num = -1;

	if (old_trigger_num != trigger_num ) {
		for (	i=0; i < NUM_TRIGGER_FLAGS; i++ )	{
			TriggerFlag[i]->flag = 0;				// Tells ui that this button isn't checked
			TriggerFlag[i]->status = 1;				// Tells ui to redraw button
		}

		if (trigger_num != -1) {
  			if (Triggers[trigger_num].flags & TRIGGER_CONTROL_DOORS)
				TriggerFlag[0]->flag = 1;
 			if (Triggers[trigger_num].flags & TRIGGER_SHIELD_DAMAGE)
				TriggerFlag[1]->flag = 1;
 			if (Triggers[trigger_num].flags & TRIGGER_ENERGY_DRAIN)
				TriggerFlag[2]->flag = 1;
 			if (Triggers[trigger_num].flags & TRIGGER_EXIT)
				TriggerFlag[3]->flag = 1;
 			if (Triggers[trigger_num].flags & TRIGGER_ONE_SHOT)
				TriggerFlag[4]->flag = 1;
 			if (Triggers[trigger_num].flags & TRIGGER_ILLUSION_ON)
				TriggerFlag[5]->flag = 1;
 			if (Triggers[trigger_num].flags & TRIGGER_ILLUSION_OFF)
				TriggerFlag[6]->flag = 1;
 			if (Triggers[trigger_num].flags & TRIGGER_ON)
				TriggerFlag[7]->flag = 1;
 			if (Triggers[trigger_num].flags & TRIGGER_MATCEN)
				TriggerFlag[8]->flag = 1;
 			if (Triggers[trigger_num].flags & TRIGGER_SECRET_EXIT)
				TriggerFlag[9]->flag = 1;
		}
	}
	
	//------------------------------------------------------------
	// If any of the checkboxes that control the wallflags are set, then
	// update the cooresponding wall flag.
	//------------------------------------------------------------
	if (IS_CHILD(Markedsegp->children[Markedside])) {
		if (TriggerFlag[0]->flag == 1) 
			trigger_add_to_Markedside(TRIGGER_CONTROL_DOORS); 
		else
			trigger_remove_flag_from_Markedside(TRIGGER_CONTROL_DOORS);
		if (TriggerFlag[1]->flag == 1)
			trigger_add_to_Markedside(TRIGGER_SHIELD_DAMAGE); 
		else
			trigger_remove_flag_from_Markedside(TRIGGER_SHIELD_DAMAGE);
		if (TriggerFlag[2]->flag == 1)
			trigger_add_to_Markedside(TRIGGER_ENERGY_DRAIN); 
		else
			trigger_remove_flag_from_Markedside(TRIGGER_ENERGY_DRAIN);
		if (TriggerFlag[3]->flag == 1)
			trigger_add_to_Markedside(TRIGGER_EXIT); 
		else
			trigger_remove_flag_from_Markedside(TRIGGER_EXIT);
		if (TriggerFlag[4]->flag == 1)
			trigger_add_to_Markedside(TRIGGER_ONE_SHOT); 
		else
			trigger_remove_flag_from_Markedside(TRIGGER_ONE_SHOT);
		if (TriggerFlag[5]->flag == 1)
			trigger_add_to_Markedside(TRIGGER_ILLUSION_ON); 
		else
			trigger_remove_flag_from_Markedside(TRIGGER_ILLUSION_ON);
		if (TriggerFlag[6]->flag == 1)
			trigger_add_to_Markedside(TRIGGER_ILLUSION_OFF);
		else
			trigger_remove_flag_from_Markedside(TRIGGER_ILLUSION_OFF);
		if (TriggerFlag[7]->flag == 1)
			trigger_add_to_Markedside(TRIGGER_ON);
		else
			trigger_remove_flag_from_Markedside(TRIGGER_ON);

		if (TriggerFlag[8]->flag == 1) 
			trigger_add_to_Markedside(TRIGGER_MATCEN);
		else
			trigger_remove_flag_from_Markedside(TRIGGER_MATCEN);

		if (TriggerFlag[9]->flag == 1) 
			trigger_add_to_Markedside(TRIGGER_SECRET_EXIT);
		else
			trigger_remove_flag_from_Markedside(TRIGGER_SECRET_EXIT);

	} else
		for (	i=0; i < NUM_TRIGGER_FLAGS; i++ )
			if (TriggerFlag[i]->flag == 1) { 
				TriggerFlag[i]->flag = 0;					// Tells ui that this button isn't checked
				TriggerFlag[i]->status = 1;				// Tells ui to redraw button
			}
	
	//------------------------------------------------------------
	// Draw the wall in the little 64x64 box
	//------------------------------------------------------------
  	gr_set_current_canvas( WallViewBox->canvas );

	if ((Markedsegp->sides[Markedside].wall_num == -1) || (Walls[Markedsegp->sides[Markedside].wall_num].trigger) == -1)
		gr_clear_canvas( CBLACK );
	else {
		if (Markedsegp->sides[Markedside].tmap_num2 > 0)  {
			gr_ubitmap(0,0, texmerge_get_cached_bitmap( Markedsegp->sides[Markedside].tmap_num, Markedsegp->sides[Markedside].tmap_num2));
		} else {
			if (Markedsegp->sides[Markedside].tmap_num > 0)	{
				PIGGY_PAGE_IN(Textures[Markedsegp->sides[Markedside].tmap_num]);
				gr_ubitmap(0,0, &GameBitmaps[Textures[Markedsegp->sides[Markedside].tmap_num].index]);
			} else
				gr_clear_canvas( CGREY );
		}
 	}

	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this robot.
	//------------------------------------------------------------
	if (ui_button_any_drawn || (old_trigger_num != trigger_num) ) {
		if ( Markedsegp->sides[Markedside].wall_num > -1 )	{
			ui_wprintf_at( MainWindow, 12, 6, "Trigger: %d    ", trigger_num);
		}	else {
			ui_wprintf_at( MainWindow, 12, 6, "Trigger: none ");
		}
		Update_flags |= UF_WORLD_CHANGED;
	}

	if ( QuitButton->pressed || (last_keypress==KEY_ESC))	{
		close_trigger_window();
		return;
	}		

	old_trigger_num = trigger_num;
}

#endif
