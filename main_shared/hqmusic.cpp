/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#define STB_VORBIS_HEADER_ONLY
#include "misc/stb_vorbis.c"


#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <thread>

#include "platform/mono.h"
#include "platform/i_sound.h"
#include "platform/platform_filesys.h"
#include "platform/timer.h"
#include "misc/error.h"

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

int RBA_Start_track, RBA_End_track;
volatile int RBA_Current_track = -1;

std::thread* RBA_thread;

//Data of the song currently decoding. ATM loaded all at once.
//TODO: Profile, may be much faster to let stb_vorbis do its own IO.
std::vector<uint8_t> RBA_data;

//Samples of the song currently decoding. Interleaved, stereo.
//TODO: Interface for allowing non-stereo data, since it's quicker
std::vector<uint16_t> RBA_samples;

//Set when the decoder reports no more data. When this is true, the thread
//will wait for the song to end playing, and then load another one. 
bool RBA_out_of_data = false;

//Set to true while the RBA thread should run
volatile bool RBA_active = false;

//Set to !0 if there's an error
volatile int RBA_error = 0;

stb_vorbis* RBA_Current_vorbis_handle = nullptr;
int RBA_Vorbis_stream_offset;
int RBA_Vorbis_error;

stb_vorbis_alloc RBA_Vorbis_alloc_buffer;
//Last frame data.
//TODO: Make this not constant size
constexpr int RBA_NUM_SAMPLES = 4096;
short RBA_Vorbis_frame_data[RBA_NUM_SAMPLES * 2];
int RBA_Vorbis_frame_size;

//Tracks are ones-based, heh.
bool RBAThreadStartTrack(int num, void** mysourceptr)
{
	char filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char track_name[16];

	snprintf(track_name, 15, "track%02d.ogg", num);

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(filename_full_path, track_name, CHOCOLATE_SAVE_DIR);
#else
	snprintf(filename_full_path, CHOCOLATE_MAX_FILE_PATH_SIZE, "cdmusic/%s", track_name);
	filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE - 1] = '\0';
#endif
	
	FILE* file = fopen(filename_full_path, "rb");
	if (!file) return false;

	fseek(file, 0, SEEK_END);
	auto size = ftell(file);
	fseek(file, 0, SEEK_SET);
	RBA_data.resize(size);
	fread(RBA_data.data(), RBA_data.size(), 1, file);
	fclose(file);

	//RBA_Current_vorbis_handle = stb_vorbis_open_pushdata(RBA_data.data(), RBA_data.size(), &RBA_Vorbis_stream_offset, &RBA_Vorbis_error, nullptr);
	RBA_Current_vorbis_handle = stb_vorbis_open_memory(RBA_data.data(), RBA_data.size(), &RBA_Vorbis_error, &RBA_Vorbis_alloc_buffer);
	if (!RBA_Current_vorbis_handle) return false;

	stb_vorbis_info info = stb_vorbis_get_info(RBA_Current_vorbis_handle);

	void* mysource = midi_start_source();
	midi_set_music_samplerate(mysource, info.sample_rate);
	*mysourceptr = mysource;
	RBA_out_of_data = false;

	return true;
}

//int stb_vorbis_get_samples_short_interleaved(stb_vorbis *f, int channels, short *buffer, int num_shorts);
void RBAThreadDecodeSamples()
{
	RBA_Vorbis_frame_size = stb_vorbis_get_samples_short_interleaved(RBA_Current_vorbis_handle, 2, RBA_Vorbis_frame_data, RBA_NUM_SAMPLES);
	if (RBA_Vorbis_frame_size == 0)
		RBA_out_of_data = true;
}

bool RBAPeekPlayStatus()
{
	return RBA_active;
}

//Before the thread starts, RBA_Current_track and RBA_End_track must be set.
//The thread will run until RBA_Current_track exceeds RBA_End_track.
void RBAThread()
{
	RBA_Current_track = RBA_Start_track;
	void* mysource;
	bool res = RBAThreadStartTrack(RBA_Current_track, &mysource);

	if (!res)
	{
		RBA_active = false;
		RBA_error = 1;
		return;
	}

	while (RBA_active)
	{
		if (!RBA_out_of_data)
		{
			midi_dequeue_midi_buffers(mysource);
			if (midi_queue_slots_available(mysource))
			{
				RBAThreadDecodeSamples();
				if (RBA_Vorbis_frame_size > 0)
				{
					midi_queue_buffer(mysource, RBA_Vorbis_frame_size, (uint16_t*)RBA_Vorbis_frame_data);
				}
				midi_check_status(mysource);
			}
		}
		else
		{
			midi_dequeue_midi_buffers(mysource);
			if (midi_check_finished(mysource))
			{
				RBA_Current_track++;
				if (RBA_Current_track > RBA_End_track)
				{
					RBA_Current_track = -1;
					RBA_active = false;
				}
				else
				{
					midi_stop_source(mysource);
					res = RBAThreadStartTrack(RBA_Current_track, &mysource);
					if (!res)
					{
						RBA_active = true;
						RBA_error = 1;
						return;
					}
				}
			}
		}

		I_DelayUS(4000);
	}

	//If we aborted, still check if the MIDI is actually done playing. 
	midi_stop_source(mysource);
	return;
}

void RBAInit()
{
	int i;
	char filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char track_name[16];
	FILE* fp;

	RBA_error = 0;

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

	RBA_Num_tracks = i;
	if (RBA_Num_tracks >= 3) //Need sufficient tracks
		RBA_Initialized = true;
}

bool RBAEnabled()
{
	return RBA_Initialized && !RBA_error;
}

void RBAStop()
{
	if (RBA_thread != nullptr)
	{
		RBA_active = false;
		RBA_thread->join();
		delete RBA_thread;
		RBA_thread = nullptr;
	}
}

int RBAGetNumberOfTracks()
{
	return RBA_Num_tracks;
}

int RBAGetTrackNum()
{
	return RBA_Current_track;
}

void RBAStartThread()
{
	RBA_active = true;
	Assert(RBA_thread == nullptr);
	RBA_thread = new std::thread(&RBAThread);
}

int RBAPlayTrack(int track)
{
	RBA_Start_track = RBA_End_track = track;
	mprintf((0, "Playing Track %d\n", track));
	RBAStartThread();
	return 1;
}

int RBAPlayTracks(int first, int last)
{
	RBA_Start_track = first;
	RBA_End_track = last;

	mprintf((0, "Playing tracks %d to %d\n", first, last));
	RBAStartThread();
	return 1;
}