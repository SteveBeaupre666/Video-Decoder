#include "Codec.h"

CCodec::CCodec()
{
	Initialize();
}

void CCodec::Initialize()
{
	pCodec = NULL;
}

AVCodec* CCodec::Get()
{
	return pCodec;
}

void CCodec::Reset()
{
	Initialize();
}

bool CCodec::FindDecoder(AVCodecID id)
{
	pCodec = avcodec_find_decoder(id);
	return pCodec != NULL;
}

bool CCodec::FindEncoder(AVCodecID id)
{
	pCodec = avcodec_find_encoder(id);
	return pCodec != NULL;
}

