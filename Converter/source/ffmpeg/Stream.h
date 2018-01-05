#pragma once
//----------------------------------------------------------------------//
#include "ffmpeg.h"
//----------------------------------------------------------------------//

#define INVALID_STREAM	-1

class CStream {
public:
	CStream();
private:
	AVStream* Stream;
private:
	void Initialize();
	bool IsMediaTypeValid(AVMediaType type);
	bool FindStream(AVFormatContext* FormatCtx, AVMediaType type);
public:
	AVStream* Get();

	int GetIndex();
	int GetNumFrames();

	bool FindVideoStream(AVFormatContext* FormatCtx);
	bool FindAudioStream(AVFormatContext* FormatCtx);

};
