#include "Converter.h"

HWND hMainWnd = NULL;
HWND hRendererWnd = NULL;

CThread Job;
JobDataStruct JobData;
DWORD WINAPI JobThread(void *params);

CBuffer InputFiles;
CBuffer OutputFiles;

CRenderer Renderer;

///////////////////////////////////////////////////
// Dll entry point...
///////////////////////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwreason, LPVOID lpReserved)
{
    switch(fdwreason)
	{
    case DLL_PROCESS_ATTACH: InitDll();     break;
    case DLL_PROCESS_DETACH: ShutDownDll(); break; 
    case DLL_THREAD_ATTACH:  break;
    case DLL_THREAD_DETACH:  break;
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void InitDll()
{
	av_register_all();
	avformat_network_init();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void ShutDownDll()
{
	InputFiles.Free();
	OutputFiles.Free();

	_CleanupOpenGL();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void EXP_FUNC _SetMainWindow(HWND hWnd)
{
	hMainWnd = hWnd;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void EXP_FUNC _SetOpenGLWindow(HWND hWnd)
{
	hRendererWnd = hWnd;
}

///////////////////////////////////////////////////////////////////////////////////////////////

BOOL EXP_FUNC _InitializeOpenGL()
{
	Renderer.SetWindow(hRendererWnd);
	Renderer.CreatePrimaryContext();

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void EXP_FUNC _CleanupOpenGL()
{
	Renderer.DeletePrimaryContext();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void EXP_FUNC _Render()
{
	Renderer.SelectContext(PRIMARY_THREAD);
	Renderer.Render();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void EXP_FUNC _SetBgColor(float r, float g, float b)
{
	Renderer.SetBackgroundColor(r,g,b);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void EXP_FUNC _StartThread(int files_count, char *input_files, char *output_files)
{
	if(!Job.IsRunning()){
		
		int in_len  = strlen(input_files);
		int out_len = strlen(output_files);

		InputFiles.Allocate(in_len+1);
		OutputFiles.Allocate(out_len+1);

		char *in  = InputFiles.String();
		char *out = OutputFiles.String();

		memcpy(in,  input_files,  in_len);
		memcpy(out, output_files, out_len);

		JobData.InputFiles  = in;
		JobData.OutputFiles = out;
		JobData.NumFiles    = files_count;

		Job.Start(JobThread, &JobData);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

BOOL EXP_FUNC _IsThreadRunning()
{
	return Job.IsRunning();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void EXP_FUNC _AbortThread()
{
	if(Job.IsRunning())
		Job.Abort();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void EXP_FUNC _PauseThread()
{
	if(Job.IsRunning()){
		switch(Job.Paused())
		{
		case false: Job.Pause();  break;
		case true:  Job.Resume(); break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI JobThread(void *params)
{
	UINT res = UNKNOW_ERROR;

	JobDataStruct *pJobData = (JobDataStruct*)params;

	Renderer.CreateSecondaryContext();

	char *in  = pJobData->InputFiles;
	char *out = pJobData->OutputFiles;

	int n = pJobData->NumFiles;

	bool canceled = false;

	for(int i = 0; i < n; i++){
	
		int in_len  = GetFileNameSize(in);
		int out_len = GetFileNameSize(out);

		CBuffer InputName(in_len+1);
		CBuffer OutputName(out_len+1);

		memcpy(InputName.Get(),  &in[1],  in_len);
		memcpy(OutputName.Get(), &out[1], out_len);
		
		in  += in_len  + 3;
		out += out_len + 3;

		char *input_name  = InputName.String();
		char *output_name = OutputName.String();

		res = _ConvertVideo(input_name, output_name);
		if(res != JOB_SUCCEDED){
			canceled = res == JOB_CANCELED;
			break;
		}
	}

	Renderer.DeleteSecondaryContext();
	PostConvertionDoneMsg(canceled);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_OLD_CODE
UINT EXP_FUNC _ConvertVideo(char *input_fname, char *output_fname)
{
	UINT res = UNKNOW_ERROR;

	CFileIO OutputFile;
	CBuffer FrameBuffer;

	CFrame SrcFrame;
	CFrame DstFrame;

	CPacket DecoderPacket;
	CPacket EncoderPacket;

	CStream VideoStream;
	CStream AudioStream;

	CCodecContext VideoDecoder;
	CCodecContext VideoEncoder;

    CFormatContext  FormatContext;
    CConvertContext ConvertContext;

    ///////////////////////////////////////////////////////////////////////////////////////

	if(!FormatContext.AllocContext())
		goto cleanup;

	if(!FormatContext.OpenInput(input_fname))
		goto cleanup;

	if(!FormatContext.FindStreamInfo())
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	VideoStream.FindVideoStream(FormatContext.GetCtx());
	AudioStream.FindAudioStream(FormatContext.GetCtx());

	if(VideoStream.GetIndex() == INVALID_STREAM)
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	if(!VideoDecoder.GetContextFromStream(VideoStream.Get()))
		goto cleanup;

	if(!VideoDecoder.FindDecoder())
		goto cleanup;

	if(!VideoDecoder.OpenCodec())
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	if(!VideoEncoder.FindEncoder(CODEC_ID_MPEG1VIDEO))
		goto cleanup;

	if(!VideoEncoder.AllocContext())
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	int sw = VideoDecoder.GetFrameWidth();
	int sh = VideoDecoder.GetFrameHeight();
	
	int dw = sw;
	int dh = sh;

	SetSizeLimit(dw, dh, 1024);
	SetAlignment(dw, dh, 16);

	AVPixelFormat sf = VideoDecoder.GetPixelFormat();
	AVPixelFormat df = AV_PIX_FMT_YUV420P;

	///////////////////////////////////////////////////////////////////////////////////////

	VideoEncoder.SetFormat(df);
	VideoEncoder.SetSize(dw, dh);
	VideoEncoder.SetBitrate(2000000);
	VideoEncoder.SetFramerate(1, 25);
	VideoEncoder.SetGopSize(10);
	VideoEncoder.SetMaxBFrames(1);

	if(!VideoEncoder.OpenCodec())
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	if(!SrcFrame.Alloc())
		goto cleanup;

	if(!DstFrame.Alloc())
		goto cleanup;

	const int FrameBufferSize = CalcFrameBufferSize(dw, dh);
	if(!FrameBuffer.Allocate(FrameBufferSize))
		goto cleanup;

	DstFrame.SetupFrameBuffer(FrameBuffer.Get(), dw, dh, df);
	
	BYTE *y = DstFrame.GetChannel('Y');
	BYTE *u = DstFrame.GetChannel('U');
	BYTE *v = DstFrame.GetChannel('V');

    ///////////////////////////////////////////////////////////////////////////////////////

	if(!DecoderPacket.Alloc())
		goto cleanup;

	if(!EncoderPacket.Alloc())
		goto cleanup;
	
    ///////////////////////////////////////////////////////////////////////////////////////

	if(!ConvertContext.GetContext(sw, sh, sf, dw, dh, df))
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	if(!Renderer.CreateTexture(dw, dh, 4))
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	if(!OutputFile.Create(output_fname))
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	int frame = 0;
	int frames_count = VideoStream.GetNumFrames();

	UpdateProgress(0, frames_count);

	///////////////////////////////////////////////////////////////////////////////////////

	while(FormatContext.ReadFrame(DecoderPacket.Get())){

		if(!DecodeFrames(OutputFile, SrcFrame, DstFrame, DecoderPacket, EncoderPacket, ConvertContext, VideoDecoder, VideoEncoder, frame, frames_count, VideoStream.GetIndex(), sh, y, u, v, false))
			goto cleanup;

		DecoderPacket.FreePacket();
    }

	///////////////////////////////////////////////////////////////////////////////////////

	while(1){

		if(!DecodeFrames(OutputFile, SrcFrame, DstFrame, DecoderPacket, EncoderPacket, ConvertContext, VideoDecoder, VideoEncoder, frame, frames_count, VideoStream.GetIndex(), sh, y, u, v, true))
			goto cleanup;

		DecoderPacket.FreePacket();
	}
	
	///////////////////////////////////////////////////////////////////////////////////////

	res = JOB_SUCCEDED;

cleanup:

	WriteEndCode(OutputFile);
	OutputFile.Close();

	Renderer.DeleteTexture();
	//Renderer.Render();

	FrameBuffer.Free();

	SrcFrame.Free();
	DstFrame.Free();

	DecoderPacket.Free();
	EncoderPacket.Free();

	VideoDecoder.FreeContext();
	VideoEncoder.FreeContext();

	FormatContext.FreeContext();
	ConvertContext.FreeContext();

	return res;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef USE_OLD_CODE
UINT EXP_FUNC _ConvertVideo(char *input_fname, char *output_fname)
{
	UINT res = UNKNOW_ERROR;

	CFileIO OutputFile;
	CBuffer FrameBuffer;

	CFrame SrcFrame;
	CFrame DstFrame;

	CPacket DecoderPacket;
	CPacket EncoderPacket;

	CVideoDecoder VideoDecoder;
	CVideoEncoder VideoEncoder;

    CFormatContext  FormatContext;
    CConvertContext ConvertContext;

    ///////////////////////////////////////////////////////////////////////////////////////

	if(!FormatContext.AllocContext())
		goto cleanup;
		
	if(!FormatContext.OpenInput(input_fname))
		goto cleanup;
		
	if(!FormatContext.FindStreamInfo())
		goto cleanup;	

    ///////////////////////////////////////////////////////////////////////////////////////

	int video_stream = FormatContext.FindVideoStream();
	int audio_stream = FormatContext.FindAudioStream();

	if(video_stream == INVALID_STREAM)
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	AVStream* stream = FormatContext.GetVideoStream();

	if(!VideoDecoder.GetCodecFromStream(stream))
		goto cleanup;
		
	if(!VideoDecoder.FindDecoder())
		goto cleanup;
		
	if(!VideoDecoder.OpenCodec())
		goto cleanup;
		
	///////////////////////////////////////////////////////////////////////////////////////

	if(!SrcFrame.Alloc())
		goto cleanup;
		
	if(!DstFrame.Alloc())
		goto cleanup;
		
	///////////////////////////////////////////////////////////////////////////////////////

	if(!VideoEncoder.FindEncoder(CODEC_ID_MPEG1VIDEO))
		goto cleanup;
		
	if(!VideoEncoder.AllocContext())
		goto cleanup;
		
    ///////////////////////////////////////////////////////////////////////////////////////

	int sw,sh,dw,dh;
	dw = sw = VideoDecoder.GetWidth();
	dh = sh = VideoDecoder.GetHeight();
	
	SetSizeLimit(dw, dh, 1024);   // Set Width/Height limit
	SetAlignment(dw, dh, 16);     // Force 16 bytes alignment
	
	///////////////////////////////////////////////////////////////////////////////////////

	AVPixelFormat fmt = AV_PIX_FMT_YUV420P;
	VideoEncoder.Setup(dw, dh, 2000000, MakeRatio(1, 25), 10, 1, AV_PIX_FMT_YUV420P);

	if(!VideoEncoder.OpenCodec())
		goto cleanup;
		
	///////////////////////////////////////////////////////////////////////////////////////

	const int BufSize = CalcFrameBufferSize(dw, dh);
	FrameBuffer.Allocate(BufSize);
	
	DstFrame.SetupFrameBuffer(FrameBuffer.Get(), dw, dh, fmt);
	
	ConvertContext.GetContext(sw, sh, fmt, dw, dh, fmt);

    ///////////////////////////////////////////////////////////////////////////////////////

	DecoderPacket.Allocate();
	EncoderPacket.Allocate();

	DecoderPacket.SetupPacket(NULL, 0);

	///////////////////////////////////////////////////////////////////////////////////////

	// Initialize OpenGL
	Renderer.CreateTexture(dw, dh, 4);

	BYTE *y = DstFrame.GetChannel('Y');
	BYTE *u = DstFrame.GetChannel('U');
	BYTE *v = DstFrame.GetChannel('V');

	///////////////////////////////////////////////////////////////////////////////////////

	// Create the output file
	if(!OutputFile.Create(output_fname))
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	while(FormatContext.ReadFrame(DecoderPacket.Get())){
		if(!Decode(&OutputFile, &ConvertContext, &VideoDecoder, &VideoEncoder, &DecoderPacket, &EncoderPacket, &SrcFrame, &DstFrame, video_stream, audio_stream, sh, y, u, v))
			goto cleanup;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	//while(1){

	//}

	///////////////////////////////////////////////////////////////////////////////////////

	res = JOB_SUCCEDED;

	///////////////////////////////////////////////////////////////////////////////////////

cleanup:

	Renderer.DeleteTexture();
	//Renderer.Render();

	CloseOutputFile(OutputFile);

	SrcFrame.Free();
	DstFrame.Free();

	FrameBuffer.Free();

	DecoderPacket.Free();
	EncoderPacket.Free();

	VideoDecoder.FreeContext();
	VideoEncoder.FreeContext();

    FormatContext.FreeContext();
    ConvertContext.FreeContext();

	return res;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

bool DecodeFrames(CFileIO &OutputFile, CFrame &SrcFrame, CFrame &DstFrame, CPacket &DecoderPacket, CPacket &EncoderPacket, CConvertContext &ConvertContext, CCodecContext &VideoDecoder, CCodecContext &VideoEncoder, int &frame, int frames_count, int video_stream, int src_height, BYTE *y, BYTE *u, BYTE *v, bool flushing)
{
	int stream_index = DecoderPacket.GetStreamIndex();

	if(stream_index == video_stream){
			
		if(!CheckThreadSignals())
			return false;

		int got_frame = 0;
		if(!DecodeVideo(VideoDecoder.GetCtx(), SrcFrame.Get(), DecoderPacket.Get(), &got_frame))
			return false;

		if(flushing && !got_frame)
			return false;

		if(got_frame){
				
			ConvertContext.Scale(SrcFrame.Get(), DstFrame.Get(), src_height);

			Renderer.UpdateTexture(y,u,v);
			Renderer.Render();

			EncoderPacket.InitPacket();
			EncoderPacket.SetBuffer(NULL, 0);

			int got_output = 0;
			if(!EncodeVideo(VideoEncoder.GetCtx(), DstFrame.Get(), EncoderPacket.Get(), &got_output))
				return false;
				
			if(got_output){
				frame++;
				OutputFile.Write(EncoderPacket.Get()->data, EncoderPacket.Get()->size);
				UpdateProgress(frame, frames_count);
				EncoderPacket.FreePacket();
			}
		}

	}
		
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool DecodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, int *got_frame)
{
	int res = avcodec_decode_video2(ctx, frame, got_frame, pkt);
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool EncodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, int *got_frame)
{
	int res = avcodec_encode_video2(ctx, pkt, frame, got_frame);
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

int CalcFrameBufferSize(int w, int h)
{
	int n = w * h;
	
	int y = n;
	int u = n / 4;
	int v = n / 4;

	return y + u + v;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void SetAlignment(int &w, int &h, int n)
{
	int wg = w % n;
	int hg = h % n;
	if(wg != 0){w += n - wg;}
	if(hg != 0){h += n - hg;}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void SetSizeLimit(int &w, int &h, int limit)
{
	if(w > 1 && h > 1){
		while(w > limit || h > limit){
			w >>= 1;
			h >>= 1;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool WritePacket(CFileIO &OutputFile, CPacket &Packet)
{
	AVPacket* pkt = (AVPacket*)Packet.Get();
	return OutputFile.Write(pkt->data, pkt->size);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void WriteEndCode(CFileIO &OutputFile)
{
	if(OutputFile.IsOpened()){
		DWORD EndCode = 0x000001B7;
		OutputFile.Write(&EndCode, sizeof(DWORD));
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CloseOutputFile(CFileIO &OutputFile)
{
	DWORD EndCode = 0x000001B7;
	OutputFile.Write(&EndCode, sizeof(EndCode));
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CheckThreadSignals()
{
	while(1){
	
		if(Job.Aborted())
			return false;
	
		if(!Job.Paused())
			break;
			
		Renderer.Render();
		Sleep(16);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void UpdateProgress(int frame, int frames_count)
{
	if(hMainWnd){
		
		if(frames_count <= 0 || frame > frames_count)
			frame = frames_count = 0;

		PostMessage(hMainWnd, WM_UPDATE_PROGRESS, frame, frames_count);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void PostConvertionDoneMsg(bool canceled)
{
	if(hMainWnd)
		PostMessage(hMainWnd, WM_THREAD_TERMINATED, 0, canceled ? 1 : 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*bool check_sample_fmt(AVCodec *codec, AVSampleFormat sample_fmt)
{
    AVSampleFormat *p = (AVSampleFormat*)codec->sample_fmts;

    while(*p != AV_SAMPLE_FMT_NONE){
        
		if(*p == sample_fmt)
            return true;

        p++;
    }

    return false;
}
*/

