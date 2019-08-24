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

#include "misc/error.h"
#include "inferno.h"
#include "vclip.h"
#include "weapon.h"

//----------------- Variables for video clips -------------------
int 					Num_vclips = 0;
vclip 				Vclip[VCLIP_MAXNUM];		// General purpose vclips.

//draw an object which renders as a vclip
void draw_vclip_object(object* obj, fix timeleft, int lighted, int vclip_num)
{
	int nf, bitmapnum;

	nf = Vclip[vclip_num].num_frames;

	bitmapnum = (nf - f2i(fixdiv((nf - 1) * timeleft, Vclip[vclip_num].play_time))) - 1;

	if (bitmapnum >= Vclip[vclip_num].num_frames)
		bitmapnum = Vclip[vclip_num].num_frames - 1;

	if (bitmapnum >= 0) {

		if (Vclip[vclip_num].flags & VF_ROD)
			draw_object_tmap_rod(obj, Vclip[vclip_num].frames[bitmapnum], lighted);
		else {
			Assert(lighted == 0);		//blob cannot now be lighted

			draw_object_blob(obj, Vclip[vclip_num].frames[bitmapnum]);
		}
	}
}

void draw_weapon_vclip(object* obj)
{
	int	vclip_num;
	fix	modtime;

	//mprintf( 0, "[Drawing obj %d type %d fireball size %x]\n", obj-Objects, Weapon_info[obj->id].weapon_vclip, obj->size );

	vclip_num = Weapon_info[obj->id].weapon_vclip;
	modtime = obj->lifeleft;
	while (modtime > Vclip[vclip_num].play_time)
		modtime -= Vclip[vclip_num].play_time;

	draw_vclip_object(obj, modtime, 0, vclip_num);
}
