//----------------------------------------------------------------------//
#define EXP_FUNC __stdcall
//----------------------------------------------------------------------//
#define WIN32_LEAN_AND_MEAN
#define VC_LEANMEAN
//----------------------------------------------------------------------//
#include <Windows.h>
#include <stdio.h>
//----------------------------------------------------------------------//
#include "FileName.h"
//----------------------------------------------------------------------//
#include "VideoConverter.h"
//----------------------------------------------------------------------//
#include "SafeKill.h"
//----------------------------------------------------------------------//
#define MULTITHREADED
//----------------------------------------------------------------------//
#define WM_UPDATE_PROGRESS		WM_USER + 101
#define WM_THREAD_TERMINATED	WM_USER + 102
//----------------------------------------------------------------------//
#define DECODER  0
#define ENCODER  1
//----------------------------------------------------------------------//
#define DECODE_VIDEO 0
#define ENCODE_VIDEO 1
//----------------------------------------------------------------------//
#define USE_OLD_CODE
//----------------------------------------------------------------------//
//#define TEST(x)	if(!x){goto cleanup;}
//----------------------------------------------------------------------//

//----------------------------------------------------------------------//
// Internal structures
//----------------------------------------------------------------------//

struct JobDataStruct {
	int   NumFiles;
	char *InputFiles;
	char *OutputFiles;
};
	
struct DecodeFramesParams {

	CFileIO *OutputFile;

	CFrame *SndFrame;
	CFrame *SrcFrame;
	CFrame *DstFrame;

	CPacket *DecoderPacket;
	CPacket *EncoderPacket;

	CStream *VideoStream;
	CStream *AudioStream;

	CCodecContext *VideoDecoder;
	CCodecContext *VideoEncoder;

	CCodecContext *AudioDecoder;
	CCodecContext *AudioEncoder;

	CConvertContext *ConvertContext;

	BYTE *y;
	BYTE *u;
	BYTE *v;

	int *frame;
	int  frames_count;

	int  src_height;

	bool *flushing;
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

bool DecodeFrames(DecodeFramesParams *params);

bool DecodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, int *got_frame);
bool EncodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, int *got_frame);

int IncrementFrameCounter(int *frame_counter);

int CalcFrameBufferSize(int w, int h);

bool CheckThreadSignals();

void SetAlignment(int &w, int &h, int n);
void SetSizeLimit(int &w, int &h, int limit);

bool WritePacket(CFileIO *f, AVPacket *pkt);
void CloseOutputFile(CFileIO &OutputFile);

void PostConvertionDoneMsg(bool canceled);
void UpdateProgress(int frame, int frames_count);

void WriteEndCode(CFileIO &OutputFile);
