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
private:
public:
	AVFormatContext* GetCtx();
	
	bool AllocContext();
	void FreeContext();

	bool OpenInput(char *fname);
	void CloseInput();

	bool FindStreamInfo();

	int GetNumStreams();

	int FindStream(AVMediaType type);
	AVStream* GetStream(AVMediaType type);

	AVStream* GetVideoStream();
	AVStream* GetAudioStream();

	bool ReadFrame(AVPacket* pkt);
};

