#pragma once

#include "StreamReader.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
}

class LAVStreamReader : public CBaseStreamReader
{
private:
	AVFormatContext* m_avf;

	AVCodecContext* m_avc;

	AVCodec* m_codec;

	AVAudioResampleContext* m_avr;

	AVFrame* m_frame;

	UnkStruct m_unkStructs[2];

	uint32_t m_numRemainingBytes;

	uint32_t m_numDecodedBytes;

	uint8_t* m_decodedBytes;

public:
	LAVStreamReader();

	virtual ~LAVStreamReader();

	virtual bool Open(const wchar_t* path, int a2);

	virtual bool Close();

	virtual size_t FillBuffer(void* buffer, size_t size);

	virtual void SetCursor(uint32_t msec);

	virtual uint32_t GetSampleRate() { return 44100; }

	virtual uint32_t GetDuration();

	virtual uint32_t GetStreamPlayTimeMs();

	virtual UnkStruct* GetUnkStruct1() { return &m_unkStructs[0]; }

	virtual UnkStruct* GetUnkStruct2() { return &m_unkStructs[1]; }
};