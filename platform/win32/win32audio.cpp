
#ifndef USE_OPENAL

#include "platform/i_sound.h"
#include "platform/s_sequencer.h"
#include "misc/error.h"
#include <Windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include <thread>
#include <mutex>
#include <algorithm>

#undef min
#undef max

#define NUMMVESNDBUFFERS 100
#define FORMAT_U8 1
#define FORMAT_S16 2

namespace
{
	IMMDevice *mmdevice;
	IAudioClient *audio_client;
	IAudioRenderClient *audio_render_client;
	MidiSequencer *sequencer;
	HANDLE audio_buffer_ready_event = INVALID_HANDLE_VALUE;
	bool is_playing;
	UINT32 fragment_size;
	int wait_timeout;
	float* next_fragment;

	int mixing_frequency = 48000;
	int mixing_latency = 50;

	std::thread mixer_thread;
	std::mutex mixer_mutex;
	bool mixer_stop_flag;

	float music_volume;
	unsigned short* midi_buffer;

	struct SoundSource
	{
		bool playing = false;
		int pos = 0;
		uint32_t frac = 0;
		const unsigned char* data = nullptr;
		int length = 0;
		int loop_start = 0;
		int sampleRate = 0;
		float angle_x = 0.0f;
		float angle_y = 0.0f;
		float volume = 0.0f;
		bool loop = false;
	} sources[_MAX_VOICES];

	struct MusicSource
	{
		bool playing = false;
		int pos = 0;
		uint32_t frac = 0;
		int sample_rate = 0;
		std::vector<float> song_data;
		bool loop = false;
	} music;

	typedef struct
	{
		bool live = false;
		int length = 0;
		int pos = 0;
		uint32_t frac = 0;
		uint8_t* data = nullptr;
	} moviesource_t;

	struct MovieRingBuffer
	{
		bool playing = false;
		bool stereo = false;
		int format = 0;
		int last_queued_buffer = 0;
		int current_buffer = 0;
		int sample_rate = 0;

		moviesource_t buffers[NUMMVESNDBUFFERS];
	} moviebuffer;

	void mix_fragment()
	{
		std::unique_lock<std::mutex> lock(mixer_mutex);

		int count = fragment_size;
		float* output = next_fragment;
		for (int i = 0; i < count; i++)
		{
			output[0] = 0.0f;
			output[1] = 0.0f;
			output += 2;
		}

		for (int handle = 0; handle < _MAX_VOICES; handle++)
		{
			SoundSource& src = sources[handle];
			if (src.playing)
			{
				output = next_fragment;
				uint32_t speed = static_cast<uint32_t>(src.sampleRate * static_cast<uint64_t>(1 << 16) / mixing_frequency);
				int pos = src.pos;
				uint32_t frac = src.frac;
				int length = src.length;
				int loop_start = src.loop_start;
				const unsigned char* data = src.data;
				float volume_left = src.volume * (1.0f + src.angle_x) * 0.5f;
				float volume_right = src.volume * (1.0f - src.angle_x) * 0.5f;
				for (int i = 0; i < count; i++)
				{
					float sample = (static_cast<int>(data[pos]) - 127) * (1.0f/127.0f);
					sample = std::max(sample, -1.0f);
					sample = std::min(sample, 1.0f);

					output[0] += sample * volume_left;
					output[1] += sample * volume_right;
					output += 2;

					frac += speed;
					pos += frac >> 16;
					frac &= 0xffff;
					if (pos >= length)
					{
						pos = pos % length;
						if (!src.loop)
						{
							pos = 0;
							frac = 0;
							src.playing = false;
							break;
						}
						else
						{
							pos += loop_start; //[ISB] hopefully this should do the trick...
						}
					}
				}
				src.pos = pos;
				src.frac = frac;
			}
		}

		if (music.playing)
		{
			output = next_fragment;
			uint32_t speed = static_cast<uint32_t>(music.sample_rate * static_cast<uint64_t>(1 << 16) / mixing_frequency);
			int pos = music.pos;
			uint32_t frac = music.frac;
			int length = music.song_data.size() / 2;
			const float* data = music.song_data.data();
			for (int i = 0; i < count; i++)
			{
				float sample = (static_cast<int>(data[pos]) - 127) * (1.0f / 127.0f);
				sample = std::max(sample, -1.0f);
				sample = std::min(sample, 1.0f);

				output[0] += data[pos << 1] * music_volume;
				output[1] += data[(pos << 1) + 1] * music_volume;
				output += 2;

				frac += speed;
				pos += frac >> 16;
				frac &= 0xffff;
				if (pos >= length)
				{
					pos = pos % length;
					if (!music.loop)
					{
						pos = 0;
						frac = 0;
						music.playing = false;
						break;
					}
				}
			}
			music.pos = pos;
			music.frac = frac;
		}
		else if (sequencer != nullptr) //[ISB] Play MIDI music if there's no HQ audio running right now
		{
			output = next_fragment;
			uint32_t speed = static_cast<uint32_t>(music.sample_rate * static_cast<uint64_t>(1 << 16) / mixing_frequency);
			uint32_t frac = music.frac;
			sequencer->Render(fragment_size, midi_buffer);
			const short* data = (short*)midi_buffer;
			for (int i = 0; i < count; i++)
			{
				float sample = (static_cast<int>(data[i << 1])) * (1.0f / 32767.0f);
				sample = std::max(sample, -1.0f);
				sample = std::min(sample, 1.0f);

				output[0] += sample * music_volume;

				sample = (static_cast<int>(data[(i << 1) + 1])) * (1.0f / 32767.0f);
				sample = std::max(sample, -1.0f);
				sample = std::min(sample, 1.0f);

				output[1] += sample * music_volume;
				output += 2;
			}
		}
		if (moviebuffer.playing)
		{
			output = next_fragment;
			uint32_t speed = static_cast<uint32_t>(moviebuffer.sample_rate * static_cast<uint64_t>(1 << 16) / mixing_frequency);
			int length = moviebuffer.buffers[moviebuffer.current_buffer].length;
			int pos = moviebuffer.buffers[moviebuffer.current_buffer].pos;
			uint32_t frac = moviebuffer.buffers[moviebuffer.current_buffer].frac;
			if (moviebuffer.stereo) length /= 2;

			if (moviebuffer.format == FORMAT_S16)
			{
				const short* data = (short*)moviebuffer.buffers[moviebuffer.current_buffer].data;

				for (int i = 0; i < count; i++)
				{
					if (data == nullptr) break;

					if (moviebuffer.stereo)
					{
						float sample = (static_cast<int>(data[pos<<1])) * (1.0f / 32767.0f);
						sample = std::max(sample, -1.0f);
						sample = std::min(sample, 1.0f);

						output[0] += sample * 0.5f;

						sample = (static_cast<int>(data[(pos<<1)+1])) * (1.0f / 32767.0f);
						sample = std::max(sample, -1.0f);
						sample = std::min(sample, 1.0f);
						output[1] += sample * 0.5f;
					}
					else
					{
						float sample = (static_cast<int>(data[pos]) - 32767) * (1.0f / 32767.0f);
						sample = std::max(sample, -1.0f);
						sample = std::min(sample, 1.0f);

						output[0] += sample * 0.5f;
						output[1] += sample * 0.5f;
					}
					output += 2;

					frac += speed;
					pos += frac >> 16;
					frac &= 0xffff;

					while (pos >= length) //[ISB] this is annoyingly complex as I need to be able to switch between multiple sources
					{
						moviebuffer.buffers[moviebuffer.current_buffer].live = false; //no longer a valid buffer
						delete[] moviebuffer.buffers[moviebuffer.current_buffer].data; //kill data
						moviebuffer.current_buffer = (moviebuffer.current_buffer + 1) % NUMMVESNDBUFFERS;
						pos = pos % length;
						length = moviebuffer.buffers[moviebuffer.current_buffer].length;
						if (moviebuffer.stereo) length /= 2;

						if (moviebuffer.buffers[moviebuffer.current_buffer].live) //more data
						{
							data = (short*)moviebuffer.buffers[moviebuffer.current_buffer].data;
						}
						else //uh, this is possibly bad, out of data
						{
							data = nullptr;
							break;
						}
					}
					moviebuffer.buffers[moviebuffer.current_buffer].pos = pos;
					moviebuffer.buffers[moviebuffer.current_buffer].frac = frac;
				}
			}
			else
			{
				Int3(); //I'll support it later...
			}
		}
	}

	void write_fragment()
	{
		UINT32 write_pos = 0;
		while (write_pos < fragment_size)
		{
			WaitForSingleObject(audio_buffer_ready_event, wait_timeout);

			UINT32 num_padding_frames = 0;
			audio_client->GetCurrentPadding(&num_padding_frames);

			UINT32 buffer_available = fragment_size - num_padding_frames;
			UINT32 buffer_needed = fragment_size - write_pos;

			if (buffer_available < buffer_needed)
				ResetEvent(audio_buffer_ready_event);

			UINT32 buffer_size = std::min(buffer_needed, buffer_available);
			if (buffer_size > 0)
			{
				BYTE* buffer = 0;
				HRESULT result = audio_render_client->GetBuffer(buffer_size, &buffer);
				if (SUCCEEDED(result))
				{
					memcpy(buffer, next_fragment + write_pos * 2, sizeof(float) * 2 * buffer_size);
					result = audio_render_client->ReleaseBuffer(buffer_size, 0);

					if (!is_playing)
					{
						result = audio_client->Start();
						if (SUCCEEDED(result))
							is_playing = true;
					}
				}

				write_pos += buffer_size;
			}
		}
	}

	void mixer_thread_main()
	{
		std::unique_lock<std::mutex> lock(mixer_mutex);
		while (!mixer_stop_flag)
		{
			lock.unlock();
			mix_fragment();
			write_fragment();
			lock.lock();
		}
	}

	void start_mixer_thread()
	{
		std::unique_lock<std::mutex> lock(mixer_mutex);
		mixer_stop_flag = false;
		lock.unlock();
		mixer_thread = std::thread(mixer_thread_main);
	}

	void stop_mixer_thread()
	{
		std::unique_lock<std::mutex> lock(mixer_mutex);
		mixer_stop_flag = true;
		lock.unlock();
		mixer_thread.join();
	}
}

int I_InitAudio()
{
	CoInitialize(0);
	static int majorhack = 1;

	IMMDeviceEnumerator* device_enumerator = nullptr;
	HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), 0, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&device_enumerator);
	if (FAILED(result))
	{
		Warning("I_InitAudio(): Unable to create IMMDeviceEnumerator instance\n"); //[ISB] downgrading to nonfatal warning for moment
		return 1;
	}

	result = device_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &mmdevice);
	device_enumerator->Release();
	if (FAILED(result))
	{
		Warning("I_InitAudio(): IDeviceEnumerator.GetDefaultAudioEndpoint failed\n");
		return 1;
	}

	result = mmdevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, 0, (void**)&audio_client);
	if (FAILED(result))
	{
		Warning("I_InitAudio(): IMMDevice.Activate failed\n");
		mmdevice->Release();
		return 1;
	}

	WAVEFORMATEXTENSIBLE wave_format;
	wave_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wave_format.Format.nChannels = 2;
	wave_format.Format.nBlockAlign = 2 * sizeof(float);
	wave_format.Format.wBitsPerSample = 8 * sizeof(float);
	wave_format.Format.cbSize = 22;
	wave_format.Samples.wValidBitsPerSample = wave_format.Format.wBitsPerSample;
	wave_format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
	wave_format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

	wave_format.Format.nSamplesPerSec = mixing_frequency;
	wave_format.Format.nAvgBytesPerSec = wave_format.Format.nSamplesPerSec * wave_format.Format.nBlockAlign;

	WAVEFORMATEX* closest_match = 0;
	result = audio_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX*)& wave_format, &closest_match);
	if (FAILED(result))
	{
		audio_client->Release();
		mmdevice->Release();
		audio_client = nullptr;
		mmdevice = nullptr;
		Warning("I_InitAudio(): IAudioClient.IsFormatSupported failed\n"); //[ISB] not fully sure if the cleanup is vital, but make sure its done before terminating through Error
		return 1;
	}

	// We could not get the exact format we wanted. Try to use the frequency that the closest matching format is using:
	if (result == S_FALSE)
	{
		mixing_frequency = closest_match->nSamplesPerSec;
		wait_timeout = mixing_latency * 2;
		wave_format.Format.nSamplesPerSec = mixing_frequency;
		wave_format.Format.nAvgBytesPerSec = wave_format.Format.nSamplesPerSec * wave_format.Format.nBlockAlign;

		CoTaskMemFree(closest_match);
		closest_match = 0;
	}

	result = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, mixing_latency * (REFERENCE_TIME)1000, 0, (WAVEFORMATEX*)& wave_format, 0);
	if (FAILED(result))
	{
		audio_client->Release();
		mmdevice->Release();
		audio_client = nullptr;
		mmdevice = nullptr;
		Warning("I_InitAudio(): IAudioClient.Initialize failed\n"); 
		return 1;
	}

	result = audio_client->GetService(__uuidof(IAudioRenderClient), (void**)&audio_render_client);
	if (FAILED(result))
	{
		audio_client->Release();
		mmdevice->Release();
		audio_client = nullptr;
		mmdevice = nullptr;
		Warning("I_InitAudio(): IAudioClient.GetService(IAudioRenderClient) failed\n");
		return 1;
	}

	audio_buffer_ready_event = CreateEvent(0, TRUE, TRUE, 0);
	if (audio_buffer_ready_event == INVALID_HANDLE_VALUE)
	{
		audio_render_client->Release();
		audio_client->Release();
		mmdevice->Release();
		audio_render_client = nullptr;
		audio_client = nullptr;
		mmdevice = nullptr;
		Warning("I_InitAudio(): CreateEvent failed\n");
		return 1;
	}

	result = audio_client->SetEventHandle(audio_buffer_ready_event);
	if (FAILED(result))
	{
		audio_render_client->Release();
		audio_client->Release();
		mmdevice->Release();
		audio_render_client = nullptr;
		audio_client = nullptr;
		mmdevice = nullptr;
		Warning("I_InitAudio(): IAudioClient.SetEventHandle failed\n");
		return 1;
	}

	result = audio_client->GetBufferSize(&fragment_size);
	if (FAILED(result))
	{
		audio_render_client->Release();
		audio_client->Release();
		mmdevice->Release();
		audio_render_client = nullptr;
		audio_client = nullptr;
		mmdevice = nullptr;
		Warning("I_InitAudio(): IAudioClient.GetBufferSize failed\n");
		return 1;
	}

	next_fragment = new float[2 * fragment_size];
	midi_buffer = new unsigned short[4 * fragment_size];
	start_mixer_thread();

	return 0;
}

void I_ShutdownAudio()
{
	if (audio_render_client)
	{
		stop_mixer_thread();
		if (is_playing)
		{
			audio_client->Stop();
			is_playing = false; //[ISB] oops, this was neglected.
		}
		audio_render_client->Release();
		audio_client->Release();
		mmdevice->Release();
		CloseHandle(audio_buffer_ready_event);
		delete[] next_fragment;
		delete[] midi_buffer;
		audio_render_client = nullptr;
		audio_client = nullptr;
		mmdevice = nullptr;
		audio_buffer_ready_event = INVALID_HANDLE_VALUE;
		next_fragment = nullptr;
	}
}

int I_GetSoundHandle()
{
	std::unique_lock<std::mutex> lock(mixer_mutex);
	for (int i = 0; i < _MAX_VOICES; i++)
	{
		if (!sources[i].playing)
			return i;
	}
	return _ERR_NO_SLOTS;
}

void I_SetSoundData(int handle, unsigned char* data, int length, int sampleRate)
{
	if (handle < 0 || handle >= _MAX_VOICES) return;

	std::unique_lock<std::mutex> lock(mixer_mutex);
	sources[handle].data = data;
	sources[handle].length = length;
	sources[handle].sampleRate = sampleRate;
	sources[handle].pos = 0;
	sources[handle].frac = 0;
	sources[handle].playing = false;
	sources[handle].loop = false;
	sources[handle].loop_start = 0;
}

void I_SetSoundInformation(int handle, int volume, int angle)
{
	if (handle < 0 || handle >= _MAX_VOICES) return;

	float flang = (angle / 65536.0f) * (3.1415927f);
	float x = (float)cos(flang);
	float y = (float)sin(flang);

	std::unique_lock<std::mutex> lock(mixer_mutex);
	sources[handle].angle_x = x;
	sources[handle].angle_y = y;
	sources[handle].volume = volume / 32768.0f;
}

void I_SetAngle(int handle, int angle)
{
	if (handle < 0 ||handle >= _MAX_VOICES) return;

	float flang = (angle / 65536.0f) * (3.1415927f);
	float x = (float)cos(flang);
	float y = (float)sin(flang);

	std::unique_lock<std::mutex> lock(mixer_mutex);
	sources[handle].angle_x = x;
	sources[handle].angle_y = y;
}

void I_SetVolume(int handle, int volume)
{
	if (handle < 0 || handle >= _MAX_VOICES) return;

	std::unique_lock<std::mutex> lock(mixer_mutex);
	sources[handle].volume = volume / 32768.0f;
}

void I_PlaySound(int handle, int loop)
{
	if (handle < 0 || handle >= _MAX_VOICES) return;

	std::unique_lock<std::mutex> lock(mixer_mutex);
	sources[handle].pos = 0;
	sources[handle].frac = 0;
	sources[handle].playing = true;
	sources[handle].loop = loop;
}

void I_StopSound(int handle)
{
	if (handle < 0 || handle >= _MAX_VOICES) return;

	std::unique_lock<std::mutex> lock(mixer_mutex);
	sources[handle].playing = false;
}

void I_SetLoopPoints(int handle, int start, int end)
{
	if (handle < 0 || handle >= _MAX_VOICES) return;

	std::unique_lock<std::mutex> lock(mixer_mutex);
	if (start > 0)
		sources[handle].loop_start = start;
	if (end > 0)
		sources[handle].length = end; //[ISB] kinda a hack tbh
}

int I_CheckSoundPlaying(int handle)
{
	if (handle < 0 || handle >= _MAX_VOICES) return 0;

	std::unique_lock<std::mutex> lock(mixer_mutex);
	return sources[handle].playing;
}

int I_CheckSoundDone(int handle)
{
	return !I_CheckSoundPlaying(handle);
}

void I_PlayHQSong(int sample_rate, std::vector<float>&& song_data, bool loop)
{
	std::unique_lock<std::mutex> lock(mixer_mutex);
	music.sample_rate = sample_rate;
	music.song_data = std::move(song_data);
	music.loop = loop;
	music.pos = 0;
	music.frac = 0;
	music.playing = true;
}

void I_StopHQSong()
{
	std::unique_lock<std::mutex> lock(mixer_mutex);
	music.sample_rate = 0;
	music.song_data.clear();
	music.loop = false;
	music.pos = 0;
	music.frac = 0;
	music.playing = false;
}

int I_StartMIDI(MidiSequencer* newSeq)
{
	std::unique_lock<std::mutex> lock(mixer_mutex);
	sequencer = newSeq;
	return 0;
}

void I_ShutdownMIDI()
{
	std::unique_lock<std::mutex> lock(mixer_mutex);
	sequencer = nullptr;
}

void I_StartMIDISong(hmpheader_t* song, bool loop)
{
	std::unique_lock<std::mutex> lock(mixer_mutex); //[ISB] OOPS
	if (sequencer)
		sequencer->StartSong(song, loop);
}

void I_StopMIDISong()
{
	std::unique_lock<std::mutex> lock(mixer_mutex);
	if (sequencer)
		sequencer->StopSong();
}

void I_SetMusicVolume(int volume)
{
	music_volume = volume / 127.0f;
}

uint32_t I_GetPreferredMIDISampleRate()
{
	return mixing_frequency;
}

void I_InitMovieAudio(int format, int samplerate, int stereo)
{
	std::unique_lock<std::mutex> lock(mixer_mutex);
	switch (format)
	{
	case MVESND_U8:
		moviebuffer.format = FORMAT_U8;
		break;
	case MVESND_S16LSB:
		moviebuffer.format = FORMAT_S16;
		break;
	}

	if (stereo)
		moviebuffer.stereo = true;
	else
		moviebuffer.stereo = false;
	moviebuffer.sample_rate = samplerate;
	moviebuffer.current_buffer = 0;
	moviebuffer.last_queued_buffer = 0;
}

void I_QueueMovieAudioBuffer(int len, short* data)
{
	std::unique_lock<std::mutex> lock(mixer_mutex);
	moviebuffer.buffers[moviebuffer.last_queued_buffer].data = new uint8_t[len];
	memcpy(moviebuffer.buffers[moviebuffer.last_queued_buffer].data, data, len);
	if (moviebuffer.format == FORMAT_S16)
		moviebuffer.buffers[moviebuffer.last_queued_buffer].length = len / 2;
	else
		moviebuffer.buffers[moviebuffer.last_queued_buffer].length = len;

	moviebuffer.buffers[moviebuffer.last_queued_buffer].live = true;
	moviebuffer.buffers[moviebuffer.last_queued_buffer].pos = 0;
	moviebuffer.buffers[moviebuffer.last_queued_buffer].frac = 0;

	moviebuffer.last_queued_buffer = (moviebuffer.last_queued_buffer + 1) % NUMMVESNDBUFFERS;
	if (moviebuffer.last_queued_buffer == moviebuffer.current_buffer) //ring buffer overrun. 
		Int3();

	moviebuffer.playing = true;
}

void I_DestroyMovieAudio()
{
	std::unique_lock<std::mutex> lock(mixer_mutex);
	moviebuffer.playing = false;
	for (int i = 0; i < NUMMVESNDBUFFERS; i++)
	{
		if (moviebuffer.buffers[i].live)
		{
			delete[] moviebuffer.buffers[i].data;
			moviebuffer.buffers[i].live = false;
		}
	}
}

void I_PauseMovieAudio()
{
	//[ISB] strictly speaking I don't think these need to be synced, but just in case. 
	std::unique_lock<std::mutex> lock(mixer_mutex);
	moviebuffer.playing = false;
}

void I_UnPauseMovieAudio()
{
	std::unique_lock<std::mutex> lock(mixer_mutex);
	moviebuffer.playing = true;
}

#endif
