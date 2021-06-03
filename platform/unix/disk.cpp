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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include "platform/disk.h"
#include "misc/types.h"
#include "platform/mono.h"

unsigned int GetFreeDiskSpace()
{
	char cwd[256];
	struct statvfs fsinfo;

	getcwd(cwd, 256);
	statvfs(cwd, &fsinfo);
	unsigned int sizeavail = (fsinfo.f_bavail * fsinfo.f_bsize);
	//mprintf((0, "size available: %d\n", sizeavail));
	return sizeavail;
}

bool IsDrivePresent(int id)
{
	return false;
}
