#pragma once
//----------------------------------------------------------------------//
#include "ffmpeg.h"
#include "FileIO.h"
//----------------------------------------------------------------------//

class CFrame {
public:
	CFrame();
	~CFrame();
private:
	AVFrame* Frame;
private:
	void Initialize();
public:
	AVFrame* Get();

	bool Alloc();
	void Free();

	//bool SetFrameBuffer(BYTE *buf, int w, int h, AVPixelFormat fmt);
	//bool WriteFrame(CFileIO *f, int w, int h, bool grayscale = false);

};

