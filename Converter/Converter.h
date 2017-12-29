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
//----------------------------------------------------------------------//
#include "FormatContext.h"
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

#define NO_AUDIO

struct JobDataStruct {
	int   NumFiles;
	char *InputFiles;
	char *OutputFiles;
};

struct VideoDecoderStruct {
    AVFormatContext *format_ctx;
    SwsContext      *convert_ctx;
	AVCodec         *codec;
    AVCodecContext  *codec_ctx;
    AVPacket        *packet; 
	AVFrame         *src_frame;
    AVFrame         *dst_frame;
};

struct VideoEncoderStruct {
	AVCodec         *codec;
    AVCodecContext  *codec_ctx;
    AVPacket        *packet; 
};

#ifndef NO_AUDIO
struct AudioDecoderStruct {
	AVCodec         *codec;
    AVCodecContext  *codec_ctx;
    AVPacket        *packet; 
    AVFrame         *frame;
};

struct AudioEncoderStruct {
	AVCodec         *codec;
    AVCodecContext  *codec_ctx;
    AVPacket        *packet; 
};
#endif

struct ffmpegStruct {
	VideoDecoderStruct VideoDecoder;
	VideoEncoderStruct VideoEncoder;
	#ifndef NO_AUDIO
	AudioDecoderStruct AudioDecoder;
	AudioEncoderStruct AudioEncoder;
	#endif
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

void EXP_FUNC _StartJob(int files_count, char *input_files, char *output_files);
BOOL EXP_FUNC _IsJobRunning();
void EXP_FUNC _CancelJob();
void EXP_FUNC _PauseJob();

UINT EXP_FUNC _ConvertVideo(char *input_fname, char *output_fname);

//----------------------------------------------------------------------//
// Globals Functions
//----------------------------------------------------------------------//
void WriteEndCode(CFileIO &OutputFile);
void UpdateProgress(int frame, int frames_count);
void PostJobDoneMsg(bool canceled);

#ifndef NO_AUDIO
bool check_sample_fmt(AVCodec *codec, AVSampleFormat sample_fmt);
#endif

void FreeFrame(AVFrame** frame);
void FreePacket(AVPacket** packet);
void FreeCodecCtx(AVCodecContext** codec_ctx, AVCodec** codec, bool free_ctx);
void FreeFormatCtx(AVFormatContext** format_ctx);
void FreeConvertCtx(SwsContext** convert_ctx);

int  GetFileNameLen(char *fname);
