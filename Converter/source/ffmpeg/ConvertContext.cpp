#include "ConvertContext.h"

CConvertContext::CConvertContext()
{
	Initialize();
}

CConvertContext::~CConvertContext()
{
	Cleanup();
}

void CConvertContext::Initialize()
{
	ConvertCtx = NULL;
}

void CConvertContext::Cleanup()
{
	FreeContext();
}

bool CConvertContext::GetContext(int sw, int sh, AVPixelFormat sf, int dw, int dh, AVPixelFormat df)
{
	FreeContext();

	ConvertCtx = sws_getContext(sw, sh, sf, dw, dh, df, SWS_BICUBIC, 0, 0, 0); 
	return ConvertCtx != NULL;
}

void CConvertContext::Scale(AVFrame *in, AVFrame *out, int height)
{
	if(ConvertCtx)
		sws_scale(ConvertCtx, in->data, in->linesize, 0, height, out->data, out->linesize);
}

void CConvertContext::FreeContext()
{
	if(ConvertCtx)
		sws_freeContext(ConvertCtx);

	Initialize();
}


