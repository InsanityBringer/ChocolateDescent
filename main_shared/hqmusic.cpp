/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <thread>

#include "misc/stb_vorbis.h"
#include "platform/mono.h"
#include "platform/i_sound.h"
#include "platform/platform_filesys.h"

bool PlayHQSong(const char* filename, bool loop)
{
	// Load ogg into memory:

	std::string name = filename;
	name = name.substr(0, name.size() - 4); // cut off extension

	FILE* file = fopen(("music/" + name + ".ogg").c_str(), "rb");
	if (!file) return false;
	fseek(file, 0, SEEK_END);
	auto size = ftell(file);
	fseek(file, 0, SEEK_SET);
	std::vector<uint8_t> filedata(size);
	fread(filedata.data(), filedata.size(), 1, file);
	fclose(file);

	// Decompress it:

	int error = 0;
	int stream_byte_offset = 0;
	stb_vorbis* handle = stb_vorbis_open_pushdata(filedata.data(), filedata.size(), &stream_byte_offset, &error, nullptr);
	if (handle == nullptr)
		return false;

	stb_vorbis_info stream_info = stb_vorbis_get_info(handle);
	int song_sample_rate = stream_info.sample_rate;
	int song_channels = stream_info.channels;
	std::vector<float> song_data;

	while (true)
	{
		float** pcm = nullptr;
		int pcm_samples = 0;
		int bytes_used = stb_vorbis_decode_frame_pushdata(handle, filedata.data() + stream_byte_offset, filedata.size() - stream_byte_offset, nullptr, &pcm, &pcm_samples);

		if (song_channels > 1)
		{
			for (int i = 0; i < pcm_samples; i++)
			{
				song_data.push_back(pcm[0][i]);
				song_data.push_back(pcm[1][i]);
			}
		}
		else
		{
			for (int i = 0; i < pcm_samples; i++)
			{
				song_data.push_back(pcm[0][i]);
				song_data.push_back(pcm[0][i]);
			}
		}

		stream_byte_offset += bytes_used;
		if (bytes_used == 0 || stream_byte_offset == filedata.size())
			break;
	}

	stb_vorbis_close(handle);

	plat_start_hq_song(song_sample_rate, std::move(song_data), loop);

	return true;
}

void StopHQSong()
{
	plat_stop_hq_song();
}

//Redbook music emulation functions
int RBA_Num_tracks;
bool RBA_Initialized;

int RBA_End_track;
volatile int RBA_Current_track = -1;

void RBAInit()
{
	int i;
	char filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char track_name[16];
	FILE* fp;

	//Simple hack to figure out how many CD tracks there are.
	for (i = 0; i < 99; i++)
	{
		snprintf(track_name, 15, "track%02d.ogg", i + 1);

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
		get_full_file_path(filename_full_path, track_name, CHOCOLATE_SAVE_DIR);
#else
		snprintf(filename_full_path, CHOCOLATE_MAX_FILE_PATH_SIZE, "cdmusic/%s", track_name);
		filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE - 1] = '\0';
#endif

		fp = fopen(filename_full_path, "rb");
		if (fp)
			fclose(fp);
		else
			break;
	}

	RBA_Num_tracks = 10;//i;
	if (RBA_Num_tracks >= 3) //Need sufficient tracks
		RBA_Initialized = true;
}

bool RBAEnabled()
{
	return RBA_Initialized;
}

void RBAStop()
{
}

int RBAGetNumberOfTracks()
{
	return RBA_Num_tracks;
}

int RBAGetTrackNum()
{
	return RBA_Current_track;
}

int RBAPlayTrack(int track)
{
	RBA_Current_track = RBA_End_track = track;
	mprintf((0, "Playing Track %d\n", track));
	return 1;
}

int RBAPlayTracks(int first, int last)
{
	RBA_Current_track = first;
	RBA_End_track = last;

	mprintf((0, "Playing tracks %d to %d\n", first, last));
	return 1;
}