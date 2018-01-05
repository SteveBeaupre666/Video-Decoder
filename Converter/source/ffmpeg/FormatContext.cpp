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

bool CFormatContext::ReadFrame(AVPacket* pkt)
{
	int res = av_read_frame(FormatCtx, pkt);
	return res >= 0;
}