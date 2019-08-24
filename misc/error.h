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

int error_init(const char* fmt, ...);			//init error system, set default message, returns 0=ok
void set_exit_message(const char* fmt, ...);	//specify message to print at exit
void Warning(const char* fmt, ...);				//print out warning message to user
void set_warn_func(void (*f)(const char* s));//specifies the function to call with warning messages
void clear_warn_func(void (*f)(const char* s));//say this function no longer valid
void _Assert(int expr, const char* expr_text, const char* filename, int linenum);	//assert func
void Error(const char* fmt, ...);					//exit with error code=1, print message

#define Assert(expr) _Assert(expr,#expr,__FILE__,__LINE__)
void Int3();
