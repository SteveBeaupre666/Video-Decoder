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
	int FindStream(AVMediaType type);
	AVStream* GetStream(AVMediaType type);
public:
	AVFormatContext* GetCtx();
	
	bool AllocContext();
	void FreeContext();

	bool OpenInput(char *fname);
	void CloseInput();

	bool FindStreamInfo();

	int GetNumStreams();

	int FindVideoStream();
	int FindAudioStream();

	AVStream* GetVideoStream();
	AVStream* GetAudioStream();

	bool ReadFrame(AVPacket* pkt);
};

