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

#include "platform/disk.h"
#include "misc/types.h"

#ifdef WIN32
#include <windows.h>

unsigned int GetFreeDiskSpace()
{
	DWORD sec_per_cluster,
		bytes_per_sec,
		free_clusters,
		total_clusters;

	if (!GetDiskFreeSpace(
		NULL,
		&sec_per_cluster,
		&bytes_per_sec,
		&free_clusters,
		&total_clusters)) return 0x7fffffff;

	return (uint32_t)(free_clusters * sec_per_cluster * bytes_per_sec);
}

bool IsDrivePresent(int id)
{
	DWORD driveMask = GetLogicalDrives();
	return (driveMask & (1 << (id-1)));
}

#endif