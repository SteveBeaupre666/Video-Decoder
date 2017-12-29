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
