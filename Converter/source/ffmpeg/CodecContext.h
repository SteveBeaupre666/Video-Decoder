#pragma once
//----------------------------------------------------------------------//
#include "Codec.h"
//----------------------------------------------------------------------//

class CCodecContext {
public:
	CCodecContext();
	~CCodecContext();
protected:
	AVCodec* Codec;
	AVCodecContext* CodecCtx;
protected:
	bool Allocated;
	void Initialize();
public:
	AVCodecContext* GetCtx();
	AVCodec*        GetCodec();

	bool Alloc();
	void Free();

	bool GetFromStream(AVStream* stream);

	bool FindDecoder();
	bool FindEncoder(AVCodecID id);

	bool Open();
	void Close();
};

//----------------------------------------------------------------------//
//----------------------------------------------------------------------//
//----------------------------------------------------------------------//
/*
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
*/