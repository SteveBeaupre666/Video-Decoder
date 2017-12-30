//----------------------------------------------------------------------//
#define EXP_FUNC __stdcall
//----------------------------------------------------------------------//
#define WIN32_LEAN_AND_MEAN
#define VC_LEANMEAN
//----------------------------------------------------------------------//
#include <Windows.h>
#include <stdio.h>
//----------------------------------------------------------------------//
#include "Buffer.h"
#include "FileIO.h"
#include "Thread.h"
#include "Renderer.h"
#include "FileName.h"
//----------------------------------------------------------------------//
#include "FormatContext.h"
#include "ConvertContext.h"
#include "CodecContext.h"
#include "Packet.h"
#include "Frame.h"
//----------------------------------------------------------------------//
#include "SafeKill.h"
//----------------------------------------------------------------------//
#define MULTITHREADED
//----------------------------------------------------------------------//
#define JOB_CANCELED	0x00000001
#define JOB_SUCCEDED	0x00000000
#define UNKNOW_ERROR	0xFFFFFFFF
//----------------------------------------------------------------------//
#define WM_UPDATE_PROGRESS		WM_USER + 101
#define WM_THREAD_TERMINATED	WM_USER + 102
//----------------------------------------------------------------------//

//#define NO_AUDIO
//#define TEST(x)	if(!x){goto cleanup;}

#define DECODE_VIDEO  0
#define ENCODE_VIDEO  1

struct JobDataStruct {
	int   NumFiles;
	char *InputFiles;
	char *OutputFiles;
};
	
//----------------------------------------------------------------------//
// Internal Functions
//----------------------------------------------------------------------//
void InitDll();
void ShutDownDll();

//----------------------------------------------------------------------//
// Exported Functions
//----------------------------------------------------------------------//
void EXP_FUNC _SetMainWindow(HWND hWnd);
void EXP_FUNC _SetOpenGLWindow(HWND hWnd);

BOOL EXP_FUNC _InitializeOpenGL(HWND hWnd);
void EXP_FUNC _CleanupOpenGL();

void EXP_FUNC _SetBgColor(float r, float g, float b);
void EXP_FUNC _Render();

void EXP_FUNC _StartThread(int files_count, char *input_files, char *output_files);
BOOL EXP_FUNC _IsThreadRunning();
void EXP_FUNC _AbortThread();
void EXP_FUNC _PauseThread();

UINT EXP_FUNC _ConvertVideo(char *input_fname, char *output_fname);

//----------------------------------------------------------------------//
// Globals Functions
//----------------------------------------------------------------------//

bool DecodeLoop(CFileIO *OutputFile, CConvertContext *ConvertContext, CVideoDecoder *VideoDecoder, CVideoEncoder *VideoEncoder, CPacket *DecoderPacket, CPacket *EncoderPacket, CFrame *SrcFrame, CFrame *DstFrame, int video_stream, int audio_stream, int src_height, bool flushing);

bool ProcessVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, bool &got_frame, int mode);
bool DecodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, bool &got_frame);
bool EncodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, bool &got_frame);

bool WritePacket(CFileIO &File, CPacket &Packet);
void CloseOutputFile(CFileIO &OutputFile);

AVRational MakeRatio(int num, int den);

int CalcFrameBufferSize(int w, int h);

void SetAlignment(int &w, int &h, int n);
void SetSizeLimit(int &w, int &h, int limit);

bool CheckThreadStatus();

void PostConvertionDoneMsg(bool canceled);
void UpdateProgress(int frame, int frames_count);
