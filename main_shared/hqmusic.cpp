/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "misc/stb_vorbis.h"
#include "platform/i_sound.h"

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