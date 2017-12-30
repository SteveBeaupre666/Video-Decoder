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

	bool FindDecoder(AVCodecID id);
	bool FindEncoder(AVCodecID id);

	int  GetWidth();
	int  GetHeight();

	void SetSize(int w, int h);
	void GetSize(int &w, int &h);
};

//----------------------------------------------------------------------//
//----------------------------------------------------------------------//
//----------------------------------------------------------------------//

class CVideoDecoder : public ACodecContext {
public:
	void Setup();
	void FreeContext();
};

//----------------------------------------------------------------------//

class CVideoEncoder : public ACodecContext {
public:
	void Setup(int width, int height, int bitrate, AVRational framerate, int gop_size = 10, int max_b_frames = 1, AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P);
	void FreeContext();
};

//----------------------------------------------------------------------//

class CAudioDecoder : public ACodecContext {
public:
	void FreeContext();
};

//----------------------------------------------------------------------//

class CAudioEncoder : public ACodecContext {
public:
	void FreeContext();
};


