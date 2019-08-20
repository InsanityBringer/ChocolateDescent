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
/*
 * $Source: f:/miner/source/misc/rcs/error.h $
 * $Revision: 1.12 $
 * $Author: matt $
 * $Date: 1994/06/17 15:22:46 $
 *
 * Header for error handling/printing/exiting code
 *
 */

int error_init(char* fmt, ...);			//init error system, set default message, returns 0=ok
void set_exit_message(char* fmt, ...);	//specify message to print at exit
void Warning(char* fmt, ...);				//print out warning message to user
void set_warn_func(void (*f)(char* s));//specifies the function to call with warning messages
void clear_warn_func(void (*f)(char* s));//say this function no longer valid
void _Assert(int expr, char* expr_text, char* filename, int linenum);	//assert func
void Error(char* fmt, ...);					//exit with error code=1, print message

//void Assert(int expr);

#define Assert(expr) _Assert(expr,#expr,__FILE__,__LINE__)
void Int3();




