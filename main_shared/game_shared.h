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

#pragma once

//This file contains defintions shared between Descent 1 and 2.

//from mglobal.c
extern fix FrameTime;					//time in seconds since last frame
extern fix RealFrameTime;					//time in seconds since last frame
extern fix GameTime;						//time in game (sum of FrameTime)
extern int FrameCount;					//how many frames rendered
extern fix	Next_laser_fire_time;	//	Time at which player can next fire his selected laser.
extern fix	Last_laser_fired_time;
extern fix	Next_missile_fire_time;	//	Time at which player can next fire his selected missile.
extern fix	Laser_delay_time;			//	Delay between laser fires.
extern int Cheats_enabled;

#define	NDL	5		//	Number of difficulty levels.
#define	NUM_DETAIL_LEVELS	6
