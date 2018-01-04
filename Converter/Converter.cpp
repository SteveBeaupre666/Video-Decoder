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

    CFormatContext  FormatContext;
    CConvertContext ConvertContext;

	ffmpegStruct ffmpeg;
	ZeroMemory(&ffmpeg, sizeof(ffmpegStruct));

	VideoDecoderStruct *VideoDecoder = &ffmpeg.VideoDecoder;
	VideoEncoderStruct *VideoEncoder = &ffmpeg.VideoEncoder;
	
    ///////////////////////////////////////////////////////////////////////////////////////

	if(!FormatContext.AllocContext())
		goto cleanup;

	if(!FormatContext.OpenInput(input_fname))
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	if(!FormatContext.FindStreamInfo())
		goto cleanup;

	int video_stream = FormatContext.FindVideoStream();
	if(video_stream < 0)
		goto cleanup;

	AVStream* stream = FormatContext.GetVideoStream();

    ///////////////////////////////////////////////////////////////////////////////////////

	VideoDecoder->codec_ctx = stream->codec;
	VideoDecoder->codec = avcodec_find_decoder(VideoDecoder->codec_ctx->codec_id);

    // open it
    if(avcodec_open2(VideoDecoder->codec_ctx, VideoDecoder->codec, NULL) < 0)
		goto cleanup;

    if(!SrcFrame.Alloc() || !DstFrame.Alloc())
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	// find the mpeg1 video encoder
	VideoEncoder->codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
	if(!VideoEncoder->codec)
		goto cleanup;

	VideoEncoder->codec_ctx = avcodec_alloc_context3(VideoEncoder->codec);
	if(!VideoEncoder->codec_ctx)
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	int src_width  = VideoDecoder->codec_ctx->width;
	int src_height = VideoDecoder->codec_ctx->height;

	int dst_width  = src_width;
	int dst_height = src_height;

	SetSizeLimit(dst_width, dst_height, 1024);
	SetAlignment(dst_width, dst_height, 16);

	AVPixelFormat dst_format = AV_PIX_FMT_YUV420P;
	AVPixelFormat src_format = VideoDecoder->codec_ctx->pix_fmt;

	///////////////////////////////////////////////////////////////////////////////////////

	VideoEncoder->codec_ctx->width  = dst_width;
	VideoEncoder->codec_ctx->height = dst_height;
	VideoEncoder->codec_ctx->bit_rate = 2000000;

	VideoEncoder->codec_ctx->time_base = MakeRatio(1, 25);
	VideoEncoder->codec_ctx->gop_size = 10;
	VideoEncoder->codec_ctx->max_b_frames = 1;
	VideoEncoder->codec_ctx->pix_fmt = dst_format;

	// open it
	if(avcodec_open2(VideoEncoder->codec_ctx, VideoEncoder->codec, NULL) < 0)
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	const int FrameBufferSize = CalcFrameBufferSize(dst_width, dst_height);
	FrameBuffer.Allocate(FrameBufferSize);

	DstFrame.SetupFrameBuffer(FrameBuffer.Get(), dst_width, dst_height, dst_format);
	
	BYTE *y = DstFrame.GetChannel('Y');
	BYTE *u = DstFrame.GetChannel('U');
	BYTE *v = DstFrame.GetChannel('V');

    ///////////////////////////////////////////////////////////////////////////////////////

	ConvertContext.GetContext(src_width, src_height, src_format, dst_width, dst_height, dst_format);

    ///////////////////////////////////////////////////////////////////////////////////////

	DecoderPacket.Alloc();
	EncoderPacket.Alloc();

	DecoderPacket.InitPacket();
	DecoderPacket.SetBuffer(NULL, 0);
	
	///////////////////////////////////////////////////////////////////////////////////////

	Renderer.CreateTexture(dst_width, dst_height, 4);

	///////////////////////////////////////////////////////////////////////////////////////

	if(!OutputFile.Create(output_fname))
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	int frame = 1;
	int frames_count = (int)stream->nb_frames;

	UpdateProgress(0, frames_count);

	///////////////////////////////////////////////////////////////////////////////////////

	while(FormatContext.ReadFrame(DecoderPacket.Get())){

		if(!Decode(OutputFile, SrcFrame, DstFrame, DecoderPacket, EncoderPacket, ConvertContext, VideoDecoder->codec_ctx, VideoEncoder->codec_ctx, frame, frames_count, video_stream, src_height, y, u, v, false))
			goto cleanup;

		DecoderPacket.FreePacket();
    }

	///////////////////////////////////////////////////////////////////////////////////////

	while(1){

		if(!Decode(OutputFile, SrcFrame, DstFrame, DecoderPacket, EncoderPacket, ConvertContext, VideoDecoder->codec_ctx, VideoEncoder->codec_ctx, frame, frames_count, video_stream, src_height, y, u, v, true))
			goto cleanup;

		DecoderPacket.FreePacket();
	}
	
	///////////////////////////////////////////////////////////////////////////////////////
	res = JOB_SUCCEDED;
	///////////////////////////////////////////////////////////////////////////////////////

cleanup:

	Renderer.DeleteTexture();
	//Renderer.Render();

	// Write 32 bits "End code" and close the file...
	WriteEndCode(OutputFile);

	// Cleanup everything
	FrameBuffer.Free();

	SrcFrame.Free();
	DstFrame.Free();

	DecoderPacket.Free();
	EncoderPacket.Free();

	FreeCodecCtx(&VideoDecoder->codec_ctx, &VideoDecoder->codec, 0);
	FreeCodecCtx(&VideoEncoder->codec_ctx, &VideoEncoder->codec, 1);

	FormatContext.FreeContext();
	ConvertContext.FreeContext();

	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void WriteEndCode(CFileIO &OutputFile)
{
	if(OutputFile.IsOpened()){
		DWORD EndCode = 0x000001B7;
		OutputFile.Write(&EndCode, sizeof(DWORD));
		OutputFile.Close();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void FreeCodecCtx(AVCodecContext** codec_ctx, AVCodec** codec, bool free_ctx)
{
	if(*codec_ctx){
		avcodec_close(*codec_ctx);
		if(free_ctx)
			av_free(&(*codec_ctx));
		*codec_ctx = NULL;
	}
	*codec = NULL;
}

#else

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

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

bool Decode(CFileIO &OutputFile, CFrame &SrcFrame, CFrame &DstFrame, CPacket &DecoderPacket, CPacket &EncoderPacket, CConvertContext &ConvertContext, AVCodecContext *pDecoderCodecCtx, AVCodecContext *pEncoderCodecCtx, int &frame, int frames_count, int video_stream, int src_height, BYTE *y, BYTE *u, BYTE *v, bool flushing)
{
	int stream_index = DecoderPacket.GetStreamIndex();

	if(stream_index == video_stream){
			
		if(!CheckThreadSignals())
			return false;

		int got_frame = 0;
		if(!DecodeVideo(pDecoderCodecCtx, SrcFrame.Get(), DecoderPacket.Get(), &got_frame))
			return false;

		if(!got_frame && flushing)
			return false;

		if(got_frame){
				
			ConvertContext.Scale(SrcFrame.Get(), DstFrame.Get(), src_height);

			Renderer.UpdateTexture(y,u,v);
			Renderer.Render();

			EncoderPacket.InitPacket();
			EncoderPacket.SetBuffer(NULL, 0);

			int got_output = 0;
			if(!EncodeVideo(pEncoderCodecCtx, DstFrame.Get(), EncoderPacket.Get(), &got_output))
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

bool xxcodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, int *got_frame, int mode)
{
	int res = 0;

	switch(mode)
	{
	case DECODE_VIDEO: res = avcodec_decode_video2(ctx, frame, got_frame, pkt); break;
	case ENCODE_VIDEO: res = avcodec_encode_video2(ctx, pkt, frame, got_frame); break;
	}
	
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool DecodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, int *got_frame)
{
	return xxcodeVideo(ctx, frame, pkt, got_frame, DECODE_VIDEO);
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool EncodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, int *got_frame)
{
	return xxcodeVideo(ctx, frame, pkt, got_frame, ENCODE_VIDEO);
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

AVRational MakeRatio(int num, int den)
{
	AVRational ratio;
	ratio.num = num;
	ratio.den = den;
	return ratio;
}

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

bool WritePacket(CFileIO &File, CPacket &Packet)
{
	AVPacket* pkt = (AVPacket*)Packet.Get();

	return File.Write(pkt->data, pkt->size);
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

