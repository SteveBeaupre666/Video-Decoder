#pragma once
//----------------------------------------------------------------------//
#include "ffmpeg.h"
//----------------------------------------------------------------------//

class CPacket {
public:
	CPacket();
	~CPacket();
private:
	AVPacket* Packet;
private:
	void Initialize();
public:
	AVPacket* Get();

	bool Alloc();
	void Free();

	void Reset();

	void InitPacket();
	void SetBuffer(void *buf, int size);
	void FreePacket();

	int  GetStreamIndex();
};
