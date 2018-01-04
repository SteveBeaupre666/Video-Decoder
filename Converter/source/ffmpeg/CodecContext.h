#pragma once
//----------------------------------------------------------------------//
#include "ffmpeg.h"
//----------------------------------------------------------------------//

class CCodecContext {
public:
	CCodecContext();
	~CCodecContext();
protected:
	AVCodec*        Codec;
	AVCodecContext* CodecCtx;
private:
	void Initialize();
public:
	AVCodecContext* GetCtx();
	bool GetContextFromStream(AVStream* stream);

	bool Allocated;
	bool AllocContext();
	void FreeContext();

	bool OpenCodec();
	void CloseCodec();

	bool FindDecoder();
	bool FindEncoder(AVCodecID id);

	void SetupDecoder();
	void SetupEncoder();

	int  GetFrameWidth();
	int  GetFrameHeight();
};