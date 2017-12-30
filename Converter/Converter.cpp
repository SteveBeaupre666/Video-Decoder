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
	Renderer.RenderFrame();
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

UINT EXP_FUNC _ConvertVideo(char *input_fname, char *output_fname)
{
	UINT res = UNKNOW_ERROR;

    ///////////////////////////////////////////////////////////////////////////////////////

	CFileIO OutputFile;
	CBuffer FrameBuffer;

	CFrame SrcFrame;
	CFrame DstFrame;

	CPacket DecoderPacket;
	CPacket EncoderPacket;

	CVideoDecoder VideoDecoder;
	CVideoEncoder VideoEncoder;
	CAudioDecoder AudioDecoder;
	CAudioEncoder AudioEncoder;

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

	if(video_stream == INVALID_STREAM || audio_stream == INVALID_STREAM)  // <--- A fixer plus tard (un video peu ne pas avoir de son...)
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	AVStream* stream = FormatContext.GetVideoStream();

	if(!VideoDecoder.AllocContext())
		goto cleanup;
		
	if(!VideoDecoder.GetCodecFromStream(stream))
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
	
	SetAlignment(dw, dh, 16);     // Force 16 bytes alignment
	SetSizeLimit(dw, dh, 1024);   // Set Width/Height limit
	
	///////////////////////////////////////////////////////////////////////////////////////

	AVPixelFormat fmt = AV_PIX_FMT_YUV420P;
	VideoEncoder.Setup(dw, dh, 2000000, MakeRatio(1, 25), 10, 1, AV_PIX_FMT_YUV420P);

	if(!VideoEncoder.OpenCodec())
		goto cleanup;
		
	///////////////////////////////////////////////////////////////////////////////////////

	FrameBuffer.Allocate(CalcFrameBufferSize(dw, dh));
	DstFrame.SetupFrameBuffer(FrameBuffer.Get(), dw, dh, fmt);
	ConvertContext.GetContext(sw, sh, fmt, dw, dh, fmt);

    ///////////////////////////////////////////////////////////////////////////////////////

	DecoderPacket.Allocate();
	EncoderPacket.Allocate();

	///////////////////////////////////////////////////////////////////////////////////////

	// Initialize OpenGL
	Renderer.CreateTexture(dw, dh, 4);

	///////////////////////////////////////////////////////////////////////////////////////

	// Create the output file
	if(!OutputFile.Create(output_fname))
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////

	/*while(FormatContext.ReadFrame(DecoderPacket.Get())){

		if(!CheckThreadStatus())
			goto cleanup;
	
		if(DecoderPacket.GetStreamIndex() == video_stream){

			bool got_frame = false;
			if(!DecodeVideo(VideoDecoder, SrcFrame, DecoderPacket, got_frame))
				goto cleanup;
				
			if(got_frame){
				
				ConvertContext.ScaleFrame(DstFrame.Get(), SrcFrame.Get(), sh);
				Renderer.RenderFrame(&DstFrame);
				EncoderPacket.InitPacket();

				if(!EncodeVideo(VideoEncoder, DstFrame, EncoderPacket, got_frame))
					goto cleanup;
					
				if(got_frame){
					
					if(!WritePacket(OutputFile, EncoderPacket))
						goto cleanup;
					
					// Update progress
					
					EncoderPacket.FreePacket();
				}

			}

		}

		DecoderPacket.FreePacket();
	}*/

	///////////////////////////////////////////////////////////////////////////////////////

	while(1){

	}

	///////////////////////////////////////////////////////////////////////////////////////

	res = JOB_SUCCEDED;

	///////////////////////////////////////////////////////////////////////////////////////

cleanup:

	CloseOutputFile(OutputFile);

	SrcFrame.Free();
	DstFrame.Free();

	FrameBuffer.Free();

	DecoderPacket.Free();
	EncoderPacket.Free();

	VideoDecoder.FreeContext();
	VideoEncoder.FreeContext();
	AudioDecoder.FreeContext();
	AudioEncoder.FreeContext();

    FormatContext.FreeContext();
    ConvertContext.FreeContext();

	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

bool DecodeLoop(CFileIO *OutputFile, CConvertContext *ConvertContext, CVideoDecoder *VideoDecoder, CVideoEncoder *VideoEncoder, CPacket *DecoderPacket, CPacket *EncoderPacket, CFrame *SrcFrame, CFrame *DstFrame, int video_stream, int audio_stream, int src_height, bool flushing)
{
	AVFrame* src_frame = SrcFrame->Get();
	AVFrame* dst_frame = DstFrame->Get();
	
	AVPacket* decoder_packet = DecoderPacket->Get();
	AVPacket* encoder_packet = EncoderPacket->Get();

	AVCodecContext* video_decoder = VideoDecoder->GetCtx();
	AVCodecContext* video_encoder = VideoEncoder->GetCtx();

	int stream = DecoderPacket->GetStreamIndex();

	if(stream == video_stream){

		bool got_decoder_frame = false;
		if(!DecodeVideo(video_decoder, src_frame, decoder_packet, got_decoder_frame))
			return false;
				
		if(got_decoder_frame){
				
			ConvertContext->ScaleFrame(dst_frame, src_frame, src_height);

			Renderer.RenderFrame(DstFrame);

			EncoderPacket->InitPacket();

			bool got_encoder_frame = false;
			if(!EncodeVideo(video_encoder, dst_frame, encoder_packet, got_encoder_frame))
				return false;
					
			if(got_encoder_frame){
					
				OutputFile->Write(encoder_packet->data, encoder_packet->size);
					
				//UpdateProgress(, );
					
				EncoderPacket->FreePacket();
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool ProcessVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, bool &got_frame, int mode)
{
	int res = 0;
	int got_packet = 0;

	switch(mode)
	{
	case DECODE_VIDEO: res = avcodec_decode_video2(ctx, frame, &got_packet, pkt); break;
	case ENCODE_VIDEO: res = avcodec_encode_video2(ctx, pkt, frame, &got_packet); break;
	}
	
	got_frame = got_packet != 0;
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool DecodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, bool &got_frame)
{
	return ProcessVideo(ctx, frame, pkt, got_frame, DECODE_VIDEO);
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool EncodeVideo(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, bool &got_frame)
{
	return ProcessVideo(ctx, frame, pkt, got_frame, ENCODE_VIDEO);
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

bool CheckThreadStatus()
{
	while(1){
	
		if(Job.Aborted())
			return false;
	
		if(!Job.Paused())
			break;
			
		Renderer.RenderFrame();
		Sleep(16);
	}

	return true;
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

