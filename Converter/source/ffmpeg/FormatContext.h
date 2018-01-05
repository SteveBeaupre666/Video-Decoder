#pragma once
//----------------------------------------------------------------------//
#include "ffmpeg.h"
//----------------------------------------------------------------------//

#define INVALID_STREAM	-1

class CFormatContext {
public:
	CFormatContext();
	~CFormatContext();
private:
	AVFormatContext* FormatCtx;
public:
	AVFormatContext* GetCtx();
	
	bool AllocContext();
	void FreeContext();

	bool OpenInput(char *fname);
	void CloseInput();

	bool FindStreamInfo();

	bool ReadFrame(AVPacket* pkt);
};

