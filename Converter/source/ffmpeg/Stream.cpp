#include "Stream.h"

CStream::CStream()
{
	Initialize();
}

void CStream::Initialize()
{
	Stream = NULL;
}

AVStream* CStream::Get()
{
	return Stream;
}

int CStream::GetIndex()
{
	if(Stream)
		return Stream->index;

	return INVALID_STREAM;
}

int CStream::GetNumFrames()
{
	if(Stream)
		return (int)Stream->nb_frames;

	return 0;
}

bool CStream::IsMediaTypeValid(AVMediaType type)
{
	return type == AVMEDIA_TYPE_VIDEO || type == AVMEDIA_TYPE_AUDIO;
}

bool CStream::FindStream(AVFormatContext* FormatCtx, AVMediaType type)
{
	Stream = NULL;

	if(!FormatCtx || !IsMediaTypeValid(type))
		return false;

	int NumStreams = FormatCtx->nb_streams;

	for(int i = 0; i < NumStreams; i++){
		if(FormatCtx->streams[i]->codec->codec_type == type){
			Stream = FormatCtx->streams[i];
			break;
		}
	}

	return Stream != NULL;
}

bool CStream::FindVideoStream(AVFormatContext* FormatCtx)
{
	return FindStream(FormatCtx, AVMEDIA_TYPE_VIDEO);
}

bool CStream::FindAudioStream(AVFormatContext* FormatCtx)
{
	return FindStream(FormatCtx, AVMEDIA_TYPE_AUDIO);
}

