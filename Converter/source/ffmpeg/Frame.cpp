#include "Frame.h"

CFrame::CFrame()
{
	Initialize();
}

CFrame::~CFrame()
{
	Free();
}

void CFrame::Initialize()
{
	Frame = NULL;
}

AVFrame* CFrame::Get()
{
	return Frame;
}

void CFrame::GetSize(int &w, int &h)
{
	w = Frame->width;
	h = Frame->height;
}

int CFrame::GetWidth()
{
	return Frame->width;
}

int CFrame::GetHeight()
{
	return Frame->height;
}

void CFrame::SetSize(int w, int h)
{
	Frame->width = w;
	Frame->height = h;
}

void CFrame::SetWidth(int w)
{
	Frame->width = w;
}

void CFrame::SetHeight(int h)
{
	Frame->height = h;
}

bool CFrame::Alloc()
{
	Frame = av_frame_alloc();
	return Frame != NULL;
}

void CFrame::Free()
{
	if(Frame)
		av_frame_free(&Frame);
		
	Initialize();
}

void CFrame::SetupFrameBuffer(void *buf, int w, int h, AVPixelFormat fmt)
{
	av_image_fill_arrays(Frame->data, Frame->linesize, (uint8_t*)buf, AV_PIX_FMT_YUV420P, w, h, 1);
}

BYTE* CFrame::GetChannel(char c)
{
	BYTE *p = NULL;

	switch(c)
	{
	case 'Y': p = Frame->data[0]; break;
	case 'U': p = Frame->data[1]; break;
	case 'V': p = Frame->data[2]; break;
	}

	return p;
}

void CFrame::GetChannels(BYTE **y, BYTE **u, BYTE **v)
{
	*y = Frame->data[0];
	*u = Frame->data[1];
	*v = Frame->data[2];
}
