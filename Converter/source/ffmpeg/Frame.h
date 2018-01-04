#pragma once
//----------------------------------------------------------------------//
#include "ffmpeg.h"
#include "FileIO.h"
//#include "Vector2d.h"
//----------------------------------------------------------------------//

//typedef CVector2d vec2;

class CFrame {
public:
	CFrame();
	~CFrame();
private:
	AVFrame* Frame;
private:
	void Initialize();
private:
	int  GetWidth();
	int  GetHeight();
	void SetWidth(int w);
	void SetHeight(int h);
	void GetSize(int &w, int &h);
	void SetSize(int w, int h);
public:
	AVFrame* Get();

	bool Alloc();
	void Free();

	void SetupFrameBuffer(void *buf, int w, int h, AVPixelFormat fmt);

	BYTE* GetChannel(char c);
	void  GetChannels(BYTE **y, BYTE **u, BYTE **v);
};

