#pragma once
//----------------------------------------------------------------------//
#include "ffmpeg.h"
//----------------------------------------------------------------------//

class ACodecContext {
public:
	ACodecContext();
	~ACodecContext();
protected:
	AVCodec* Codec;
	AVCodecContext* CodecCtx;
protected:
	void Initialize();
	void Cleanup();
public:
	AVCodecContext* GetCtx();
	AVCodec*        GetCodec();

	bool AllocContext();
	virtual void FreeContext(){}

	bool GetCodecFromStream(AVStream* stream);
	bool OpenCodec();
};

class CDecoderCodecContext : public ACodecContext {
public:
	void FreeContext();

	bool FindDecoder(AVCodecID id);
};

class CEncoderCodecContext : public ACodecContext {
public:
	void FreeContext();

	bool FindEncoder(AVCodecID id);

	void SetParams();
};
