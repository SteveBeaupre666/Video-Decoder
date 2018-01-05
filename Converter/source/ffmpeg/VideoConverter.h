#pragma once
//----------------------------------------------------------------------//
#include "Thread.h"
#include "Renderer.h"
//----------------------------------------------------------------------//
#include "OutputFile.h"
#include "FrameBuffer.h"
//----------------------------------------------------------------------//
#include "Frame.h"
#include "Packet.h"
#include "Stream.h"
//----------------------------------------------------------------------//
#include "CodecContext.h"
#include "FormatContext.h"
#include "ConvertContext.h"
//----------------------------------------------------------------------//
#define WM_UPDATE_PROGRESS		WM_USER + 101
#define WM_THREAD_TERMINATED	WM_USER + 102
//----------------------------------------------------------------------//
#define JOB_CANCELED	0x00000001
#define JOB_SUCCEDED	0x00000000
#define UNKNOW_ERROR	0xFFFFFFFF
//----------------------------------------------------------------------//
#define TEST(x)	if(!x){goto cleanup;}
//----------------------------------------------------------------------//

class CVideoConverter {
public:
	CVideoConverter();
	~CVideoConverter();
private:
	HWND hMainWnd;
	HWND hRenderWnd;
private:
	COutputFile  OutputFile;
	CFrameBuffer FrameBuffer;
private:
	CFrame SndFrame;
	CFrame SrcFrame;
	CFrame DstFrame;

	CPacket DecoderPacket;
	CPacket EncoderPacket;

	CStream VideoStream;
	CStream AudioStream;

	CCodecContext VideoDecoder;
	CCodecContext VideoEncoder;
	CCodecContext AudioDecoder;
	CCodecContext AudioEncoder;

	CFormatContext  FormatContext;
    CConvertContext ConvertContext;
private:
	void Initialize();
	void Cleanup();
private:
	void AdjustFrameSize(int &w, int &h, int max = 0, int align = 0);

	void ScaleFrame(int h);
	void RenderFrame(CRenderer* pRenderer, BYTE *y, BYTE *u, BYTE *v);

	void UpdateProgress(int frame, int num_frames);

	bool DecodeVideo(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int *got_frame);
	bool DecodeAudio(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int *got_frame);
	bool EncodeVideo(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int *got_output);
	bool EncodeAudio(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int *got_output);

	bool Aborted(CThread *pThread);
	bool Paused(CThread *pThread);
	bool Throttle(CThread *pThread, CRenderer *pRenderer);

	bool WriteFrame(AVPacket *pkt);
	bool CloseOutputFile();
public:
	void SetWindow(HWND hWnd);
	UINT ConvertVideo(char *in, char *out, CRenderer *pRenderer, CThread *pThread);
};
