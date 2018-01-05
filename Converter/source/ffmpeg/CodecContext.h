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

	int  GetFrameWidth();
	int  GetFrameHeight();
	
	AVPixelFormat GetPixelFormat();

	void SetSize(int w, int h);
	void SetFormat(AVPixelFormat fmt);

	void SetBitrate(int bitrate);
	void SetFramerate(int num, int den);

	void SetGopSize(int gop_size);
	void SetMaxBFrames(int max_b_frames);
};