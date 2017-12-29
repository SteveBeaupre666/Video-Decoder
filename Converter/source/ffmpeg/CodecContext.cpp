#include "CodecContext.h"

///////////////////////////////////////////////////////////////////////////////////////////////

ACodecContext::ACodecContext()
{
	Initialize();
}

///////////////////////////////////////////////////////////////////////////////////////////////

ACodecContext::~ACodecContext()
{
	Cleanup();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void ACodecContext::Initialize()
{
	Codec    = NULL;
	CodecCtx = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void ACodecContext::Cleanup()
{
	FreeContext();
}

///////////////////////////////////////////////////////////////////////////////////////////////

AVCodec* ACodecContext::GetCodec()
{
	return Codec;
}

///////////////////////////////////////////////////////////////////////////////////////////////

AVCodecContext* ACodecContext::GetCtx()
{
	return CodecCtx;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool ACodecContext::AllocContext()
{
	FreeContext();
	CodecCtx = avcodec_alloc_context3(Codec);
	return CodecCtx != NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool ACodecContext::GetCodecFromStream(AVStream* stream)
{
	CodecCtx = stream->codec;
	if(!CodecCtx)
		return false;

	Codec = avcodec_find_decoder(CodecCtx->codec_id);
	return Codec != NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool ACodecContext::OpenCodec()
{
	if(!CodecCtx || !Codec)
		return false;

	int res = avcodec_open2(CodecCtx, Codec, NULL);
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void CDecoderCodecContext::FreeContext()
{
	if(CodecCtx)
		avcodec_close(CodecCtx);

	Initialize();
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CDecoderCodecContext::FindDecoder(AVCodecID id)
{
	if(!CodecCtx)
		return false;

	Codec = avcodec_find_decoder(id);
	return Codec != NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void CEncoderCodecContext::FreeContext()
{
	if(CodecCtx){
		avcodec_close(CodecCtx);
		av_free(&CodecCtx);
	}

	Initialize();
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CEncoderCodecContext::FindEncoder(AVCodecID id)
{
	if(!CodecCtx)
		return false;

	Codec = avcodec_find_encoder(id);
	return Codec != NULL;
}

