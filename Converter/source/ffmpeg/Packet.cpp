#include "Packet.h"

CPacket::CPacket()
{
	Initialize();
}

CPacket::~CPacket()
{
	Free();
}

void CPacket::Initialize()
{
	Packet = NULL;
}

AVPacket* CPacket::Get()
{
	return Packet;
}

bool CPacket::Alloc()
{
	Free();

	static const int AVPacketSize = sizeof(AVPacket);
	Packet = (AVPacket*)malloc(AVPacketSize);
	
	if(Packet){
		InitPacket();
		SetBuffer(NULL, 0);
		return true;
	}

	return false;
}

void CPacket::Free()
{
	if(Packet){
		av_free_packet(Packet);
		free(Packet);
	}

	Initialize();
}

void CPacket::Reset()
{
	InitPacket();
	SetBuffer(NULL, 0);
}

void CPacket::InitPacket()
{
	if(Packet)
		av_init_packet(Packet);
}

void CPacket::SetBuffer(void *buf, int size)
{
	if(Packet){
		Packet->size = size;
		Packet->data = (uint8_t*)buf;
	}
}

void CPacket::FreePacket()
{
	if(Packet)
		av_free_packet(Packet);
}

int CPacket::GetStreamIndex()
{
	if(!Packet)
		return -1;

	return Packet->stream_index;
}