#include "bios/disk.h"
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

	return (uint)(free_clusters * sec_per_cluster * bytes_per_sec);
}

#endif