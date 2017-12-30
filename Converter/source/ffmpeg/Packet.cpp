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

bool CPacket::Allocate()
{
	Free();

	static const int AVPacketSize = sizeof(AVPacket);
	Packet = (AVPacket*)malloc(AVPacketSize);
	
	if(Packet)
		InitPacket();

	return Packet != NULL;
}

void CPacket::Free()
{
	if(Packet){
		av_free_packet(Packet);
		free(Packet);
	}

	Initialize();
}

void CPacket::InitPacket(void *data, int size)
{
	if(Packet){
		av_init_packet(Packet);
		SetupPacket(data, size);
	}
}

void CPacket::SetupPacket(void *data, int size)
{
	if(Packet){
		Packet->size = size;
		Packet->data = (uint8_t*)data;
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