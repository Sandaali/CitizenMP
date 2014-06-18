#include "StdInc.h"
#include "LAVStreamReader.h"

static void logCB(void*, int, const char* format, va_list ap)
{
	static char buffer[32768];
	vsprintf(buffer, format, ap);

	trace("[lav] %s", buffer);
}

LAVStreamReader::LAVStreamReader()
{
	av_log_set_level(AV_LOG_VERBOSE);
	av_log_set_callback(logCB);

	m_avf = avformat_alloc_context();
	m_avr = nullptr;
	m_avc = nullptr;

	m_frame = 0;

	m_decodedBytes = nullptr;

	trace("Initialized LAVStreamReader...\n  avformat: %s -- %s\n  avcodec: %s -- %s\n  avresample: %s -- %s\n", avformat_configuration(), avformat_license(), avcodec_configuration(), avcodec_license(), avresample_configuration(), avresample_license());
	
	memset(m_unkStructs, 0, sizeof(m_unkStructs));

	m_numRemainingBytes = 0;
}

LAVStreamReader::~LAVStreamReader()
{
	//avformat_free_context(m_avf);
	//avcodec_close(m_avc);
	av_free(m_avc);

	if (m_frame)
	{
		av_frame_free(&m_frame);
	}

	if (m_avr)
	{
		avresample_close(m_avr);
		av_free(m_avr);
	}
}

bool LAVStreamReader::Open(const wchar_t* wpath, int a2)
{
	char path[MAX_PATH];

	wcstombs(path, wpath, sizeof(path));

	if (avformat_open_input(&m_avf, path, nullptr, nullptr) < 0 ||
		avformat_find_stream_info(m_avf, nullptr) < 0)
	{
		trace("Could not open %s.\n", path);

		if (m_avf)
		{
			avformat_close_input(&m_avf);
		}

		return false;
	}

	// find the first audio stream
	AVStream* stream = nullptr;

	for (int i = 0; i < m_avf->nb_streams; i++)
	{
		stream = m_avf->streams[i];

		if (stream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			break;
		}
	}

	if (!stream)
	{
		trace("No audio stream in %s.\n", path);

		avformat_close_input(&m_avf);

		return false;
	}

	m_avc = stream->codec;

	m_codec = avcodec_find_decoder(m_avc->codec_id);

	if (avcodec_open2(m_avc, m_codec, nullptr) < 0)
	{
		trace("Could not open decoder for %s.\n", path);

		avformat_close_input(&m_avf);

		return false;
	}

	return true;
}

bool LAVStreamReader::Close()
{
	avcodec_close(m_avc);
	m_avc = nullptr;
	
	avformat_close_input(&m_avf);

	return true;
}

// based somewhat on Silent's SilentPatch code :)
size_t LAVStreamReader::FillBuffer(void* buffer, size_t size)
{
	AVPacket packet = { 0 };
	size_t bytesDecoded = 0;

	char* buf = (char*)buffer;

	while (bytesDecoded < size)
	{
		size_t thisSize;
		void* decodedBytes;

		// if nothing has been buffered from a previous decode
		if (m_numRemainingBytes == 0)
		{
			// read the packet from the transport
			if (av_read_frame(m_avf, &packet) < 0)
			{
				break;
			}

			// no frame? make one!
			if (!m_frame)
			{
				m_frame = av_frame_alloc();
			}

			int gotFrame;

			// and decode the packet
			if (avcodec_decode_audio4(m_avc, m_frame, &gotFrame, &packet) < 0)
			{
				// error state; do something about it?
				break;
			}

			if (!gotFrame)
			{
				break; // and hope this doesn't make the client assume we've stopped
			}

			if (!m_avr)
			{
				if (m_avc->channels != 2 || m_avc->sample_fmt != AV_SAMPLE_FMT_S16 || m_avc->sample_rate != 44100)
				{
					m_avr = avresample_alloc_context();

					av_opt_set_int(m_avr, "in_channel_layout", m_frame->channel_layout, 0);
					av_opt_set_int(m_avr, "in_sample_fmt", m_frame->format, 0);
					av_opt_set_int(m_avr, "in_sample_rate", m_frame->sample_rate, 0);
					av_opt_set_int(m_avr, "out_channel_layout", AV_CH_LAYOUT_STEREO_DOWNMIX, 0);
					av_opt_set_int(m_avr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
					av_opt_set_int(m_avr, "out_sample_rate", 44100, 0);

					avresample_open(m_avr);
				}
			}

			if (m_avr)
			{
				int32_t outLineSize;
				uint32_t outSize = av_samples_get_buffer_size(&outLineSize, 2, m_frame->nb_samples, AV_SAMPLE_FMT_S16, 0);

				uint32_t outSamples = avresample_get_delay(m_avr) + m_frame->nb_samples;

				outSamples = av_rescale_rnd(outSamples, 44100, m_frame->sample_rate, AV_ROUND_UP);

				outSamples += avresample_available(m_avr);

				m_decodedBytes = (uint8_t*)av_realloc_array(m_decodedBytes, 1, outSize);

				decodedBytes = m_decodedBytes;

				outSamples = avresample_convert(m_avr, &m_decodedBytes, outLineSize, outSamples, m_frame->data, m_frame->linesize[0], m_frame->nb_samples);

				thisSize = outSamples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 2;
			}
			else
			{
				decodedBytes = m_frame->data[0];

				thisSize = m_frame->nb_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * m_avc->channels;
			}

			m_numRemainingBytes = thisSize;
			m_numDecodedBytes = thisSize;

			thisSize = min(thisSize, size - bytesDecoded);
		}
		else
		{
			decodedBytes = m_decodedBytes;

			thisSize = min(m_numRemainingBytes, size);

			// move to the byte we last decoded and such
			decodedBytes = (char*)decodedBytes + (m_numDecodedBytes - thisSize);
		}

		// copy to game buffer
		memcpy(buf, decodedBytes, thisSize);
		m_numRemainingBytes -= thisSize;
		bytesDecoded += thisSize;
		buf += thisSize;
	}

	if (packet.data)
	{
		av_free_packet(&packet);
	}

	return bytesDecoded;
}

void LAVStreamReader::SetCursor(uint32_t msec)
{
	if (avformat_seek_file(m_avf, -1, 0, msec * (AV_TIME_BASE / 1000), INT32_MAX, AVSEEK_FLAG_ANY) < 0)
	{
		trace("Seek failed, don't ask me why.\n");
	}
}

uint32_t LAVStreamReader::GetDuration()
{
	return m_avf->duration / (AV_TIME_BASE / 1000);
}

uint32_t LAVStreamReader::GetStreamPlayTimeMs()
{
	AVRational baseDef = m_avf->streams[0]->time_base;

	return (m_avf->streams[0]->cur_dts / (baseDef.num / (double)baseDef.den)) * 1000;
}

static LAVStreamReader* MakeLAVReader()
{
	return new LAVStreamReader();
}

static void __declspec(naked) ConstructorJump()
{
	__asm
	{
		call MakeLAVReader

		push 0CD8AABh
		retn
	}
}

static HookFunction hookFunction([] ()
{
	av_register_all();

	hook::jump(0xCD8A8F, ConstructorJump);
});

CBaseStreamReader::~CBaseStreamReader() { }