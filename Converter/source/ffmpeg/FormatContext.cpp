#include "FormatContext.h"

CFormatContext::CFormatContext()
{
	FormatCtx = NULL;
}

CFormatContext::~CFormatContext()
{
	FreeContext();
}

AVFormatContext* CFormatContext::GetCtx()
{
	return FormatCtx;
}

bool CFormatContext::AllocContext()
{
	FreeContext();
	FormatCtx = avformat_alloc_context();
	return FormatCtx != NULL;
}

void CFormatContext::FreeContext()
{
	if(FormatCtx){
		CloseInput();
		FormatCtx = NULL;
	}
}

bool CFormatContext::OpenInput(char *fname)
{
	int res = avformat_open_input(&FormatCtx, fname, NULL, NULL);
	return res == 0;
}

void CFormatContext::CloseInput()
{
	if(FormatCtx)
		avformat_close_input(&FormatCtx);
}

bool CFormatContext::FindStreamInfo()
{
	int res = avformat_find_stream_info(FormatCtx, NULL);
	return res >= 0;
}

int CFormatContext::GetNumStreams()
{
	return (int)FormatCtx->nb_streams;
}

int CFormatContext::FindStream(AVMediaType type)
{
	if(type == AVMEDIA_TYPE_VIDEO || type == AVMEDIA_TYPE_AUDIO)
		return INVALID_STREAM;

	int n = GetNumStreams();
	for(int i = 0; i < n; i++){
		if(FormatCtx->streams[i]->codec->codec_type == type)
			return i;
	}

	return INVALID_STREAM;
}

AVStream* CFormatContext::GetStream(AVMediaType type)
{
	int i = FindStream(type);
	int n = GetNumStreams();

	if(i == INVALID_STREAM || i >= n)
		return NULL;
	
	return FormatCtx->streams[i];
}

int CFormatContext::FindVideoStream()
{
	return FindStream(AVMEDIA_TYPE_VIDEO);
}

int CFormatContext::FindAudioStream()
{
	return FindStream(AVMEDIA_TYPE_AUDIO);
}

AVStream* CFormatContext::GetVideoStream()
{
	return GetStream(AVMEDIA_TYPE_VIDEO);
}

AVStream* CFormatContext::GetAudioStream()
{
	return GetStream(AVMEDIA_TYPE_AUDIO);
}

bool CFormatContext::ReadFrame(AVPacket* pkt)
{
	int res = av_read_frame(FormatCtx, pkt);
	return res >= 0;
}