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

	bool Allocate();
	void Free();

	void InitPacket(void *data = NULL, int size = 0);
	void SetupPacket(void *data, int size);
	void FreePacket();
};
