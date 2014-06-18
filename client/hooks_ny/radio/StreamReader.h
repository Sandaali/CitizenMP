#pragma once

#include "sysAllocator.h"

class CBaseStreamReader : public rage::sysUseAllocator
{
public:
	struct UnkStruct
	{
		char data[64];
	};

public:
	virtual ~CBaseStreamReader() = 0;

	virtual bool Open(const wchar_t* path, int a2) = 0;

	virtual bool Close() = 0;

	virtual size_t FillBuffer(void* buffer, size_t size) = 0;

	virtual void SetCursor(uint32_t msec) = 0;

	virtual uint32_t GetSampleRate() = 0;

	virtual uint32_t GetDuration() = 0;

	virtual uint32_t GetStreamPlayTimeMs() = 0;

	virtual UnkStruct* GetUnkStruct1() = 0;

	virtual UnkStruct* GetUnkStruct2() = 0;
};