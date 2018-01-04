#pragma once
//----------------------------------------------------------------------//
#include "ffmpeg.h"
//----------------------------------------------------------------------//

class CCodec {
public:
	CCodec();
private:
	AVCodec* pCodec;
private:
	void Initialize();
public:
	AVCodec* Get();

	void Reset();

	bool FindDecoder(AVCodecID id);
	bool FindEncoder(AVCodecID id);
};

