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

#ifdef USE_OLD_CODE
UINT EXP_FUNC _ConvertVideo(char *input_fname, char *output_fname)
{
	UINT res = UNKNOW_ERROR;

	CFileIO OutputFile;
	CBuffer FrameBuffer;

	ffmpegStruct ffmpeg;
	ZeroMemory(&ffmpeg, sizeof(ffmpegStruct));

	VideoDecoderStruct *VideoDecoder = &ffmpeg.VideoDecoder;
	VideoEncoderStruct *VideoEncoder = &ffmpeg.VideoEncoder;
	
	#ifndef NO_AUDIO
	AudioDecoderStruct *AudioDecoder = &ffmpeg.AudioDecoder;
	AudioEncoderStruct *AudioEncoder = &ffmpeg.AudioEncoder;
	#endif

    ///////////////////////////////////////////////////////////////////////////////////////

	VideoDecoder->format_ctx = avformat_alloc_context();
	if(!VideoDecoder->format_ctx)
		goto cleanup;

	if(avformat_open_input(&VideoDecoder->format_ctx, input_fname, NULL, NULL) != 0)
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	if(avformat_find_stream_info(VideoDecoder->format_ctx, NULL) < 0)
		goto cleanup;

	// Try to find a video stream
	int video_stream = -1;
	for(int i = 0; i < (int)VideoDecoder->format_ctx->nb_streams; i++){
		if(VideoDecoder->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
			video_stream = i;
			break;
		}
	}

	// Check for error...
	if(video_stream == -1)
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	// Try to find an audio stream
	int audio_stream = -1;
	for(int i = 0; i < (int)VideoDecoder->format_ctx->nb_streams; i++){
		if(VideoDecoder->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
			audio_stream = i;
			break;
		}
	}

	// Check for error...
	if(audio_stream == -1)
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	// Get codec from the stream
	AVStream* stream = VideoDecoder->format_ctx->streams[video_stream];
	VideoDecoder->codec_ctx = stream->codec;
	VideoDecoder->codec = avcodec_find_decoder(VideoDecoder->codec_ctx->codec_id);

    // open it
    if(avcodec_open2(VideoDecoder->codec_ctx, VideoDecoder->codec, NULL) < 0)
		goto cleanup;

    VideoDecoder->src_frame = av_frame_alloc();
    VideoDecoder->dst_frame = av_frame_alloc();

    if(!VideoDecoder->src_frame || !VideoDecoder->dst_frame)
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	#ifndef NO_AUDIO
	AudioDecoder->codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
	AudioDecoder->codec_ctx = avcodec_alloc_context3(AudioDecoder->codec);

	AudioDecoder->codec_ctx->channels = 1;
    AudioDecoder->codec_ctx->bit_rate = 16;
    AudioDecoder->codec_ctx->sample_rate = 44100;

	if(avcodec_open2(AudioDecoder->codec_ctx, AudioDecoder->codec, NULL) < 0)
		goto cleanup;

    AudioDecoder->frame = av_frame_alloc();
	#endif

    ///////////////////////////////////////////////////////////////////////////////////////

	// find the mpeg1 video encoder
	VideoEncoder->codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
	if(!VideoEncoder->codec)
		goto cleanup;

	VideoEncoder->codec_ctx = avcodec_alloc_context3(VideoEncoder->codec);
	if(!VideoEncoder->codec_ctx)
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	#ifndef NO_AUDIO
	// find the acc audio encoder
	AudioEncoder->codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if(!AudioEncoder->codec)
		goto cleanup;

	AudioEncoder->codec_ctx = avcodec_alloc_context3(AudioEncoder->codec);
	if(!AudioEncoder->codec_ctx)
		goto cleanup;

    AudioEncoder->codec_ctx->bit_rate    = 64000;
    AudioEncoder->codec_ctx->sample_rate = 44100;

    AudioEncoder->codec_ctx->channels       = 1;
    AudioEncoder->codec_ctx->channel_layout = AV_CH_LAYOUT_MONO;
    AudioEncoder->codec_ctx->sample_fmt     = AV_SAMPLE_FMT_S16;

	if(!check_sample_fmt(AudioEncoder->codec, AudioEncoder->codec_ctx->sample_fmt))
		goto cleanup;
	#endif

	///////////////////////////////////////////////////////////////////////////////////////

	int src_width  = VideoDecoder->codec_ctx->width;
	int src_height = VideoDecoder->codec_ctx->height;

	int dst_width  = src_width;
	int dst_height = src_height;
	
	if(src_width > 1024){
		dst_width  = src_width  / 2;
		dst_height = src_height / 2;
	}

	const int align = 16;
	int width_gap  = dst_width  % align;
	int height_gap = dst_height % align;
	if(width_gap  != 0){dst_width  += align - width_gap;}
	if(height_gap != 0){dst_height += align - height_gap;}

	int num_pixels = dst_width * dst_height;
	int y_size = num_pixels;
	int u_size = num_pixels / 4;
	int v_size = num_pixels / 4;

	AVPixelFormat dst_format = AV_PIX_FMT_YUV420P;
	AVPixelFormat src_format = VideoDecoder->codec_ctx->pix_fmt;

	///////////////////////////////////////////////////////////////////////////////////////

	AVRational avRatio;
	avRatio.num = 1;
	avRatio.den = 25;

	VideoEncoder->codec_ctx->width  = dst_width;
	VideoEncoder->codec_ctx->height = dst_height;
	VideoEncoder->codec_ctx->bit_rate = 2000000;

	VideoEncoder->codec_ctx->time_base = avRatio;
	VideoEncoder->codec_ctx->gop_size = 10;
	VideoEncoder->codec_ctx->max_b_frames = 1;
	VideoEncoder->codec_ctx->pix_fmt = dst_format;

	// open it
	if(avcodec_open2(VideoEncoder->codec_ctx, VideoEncoder->codec, NULL) < 0)
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	FrameBuffer.Allocate(y_size + u_size + v_size);
	BYTE *frame_buffer = FrameBuffer.Get();

	av_image_fill_arrays(VideoDecoder->dst_frame->data, VideoDecoder->dst_frame->linesize, frame_buffer, dst_format, dst_width, dst_height, 1);
	
    ///////////////////////////////////////////////////////////////////////////////////////

	VideoDecoder->convert_ctx = sws_getContext(src_width, src_height, src_format, dst_width, dst_height, dst_format, SWS_BICUBIC, 0, 0, 0);

    ///////////////////////////////////////////////////////////////////////////////////////

	VideoDecoder->packet = (AVPacket*)malloc(sizeof(AVPacket));
	VideoEncoder->packet = (AVPacket*)malloc(sizeof(AVPacket));

	#ifndef NO_AUDIO
	AudioDecoder->packet = VideoDecoder->packet;
	AudioEncoder->packet = AudioDecoder->packet;
	#endif

    av_init_packet(VideoDecoder->packet);
	VideoDecoder->packet->data = NULL;
	VideoDecoder->packet->size = 0;
	
    //av_init_packet(AudioDecoder->packet);
	//AudioDecoder->packet->data = NULL;
	//AudioDecoder->packet->size = 0;
	
	///////////////////////////////////////////////////////////////////////////////////////

	// Initialize OpenGL

	Renderer.CreateTexture(dst_width, dst_height, 4);

	BYTE *pY = VideoDecoder->dst_frame->data[0];
	BYTE *pU = VideoDecoder->dst_frame->data[1];
	BYTE *pV = VideoDecoder->dst_frame->data[2];

	///////////////////////////////////////////////////////////////////////////////////////

	if(!OutputFile.Create(output_fname))
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	int frame = 1;
	int frames_count = (int)stream->nb_frames;

	int stream_index = 0;

	UpdateProgress(0, frames_count);

	///////////////////////////////////////////////////////////////////////////////////////

	while(av_read_frame(VideoDecoder->format_ctx, VideoDecoder->packet) >= 0){

		stream_index = VideoDecoder->packet->stream_index;

		#ifndef NO_AUDIO
		if(stream_index == video_stream || stream_index == audio_stream){
		#else
		if(stream_index == video_stream){
		#endif
			
			check_job_status1:
			if(Job.Aborted()){
				res = JOB_CANCELED;
				goto cleanup;
			}

			if(Job.Paused()){
				Sleep(16);
				Renderer.RenderFrame();
				goto check_job_status1;
			}

			int got_frame = 0;
			int decoded = 0;

			if(stream_index == video_stream){
				decoded = avcodec_decode_video2(VideoDecoder->codec_ctx, VideoDecoder->src_frame, &got_frame, VideoDecoder->packet);	
			} else {
				#ifndef NO_AUDIO
				decoded = avcodec_decode_audio4(AudioDecoder->codec_ctx, AudioDecoder->frame, &got_frame, AudioDecoder->packet);	
				#endif
			}

			if(decoded < 0)
				goto cleanup;

			if(got_frame){
				
				if(stream_index == video_stream){

					sws_scale(VideoDecoder->convert_ctx, VideoDecoder->src_frame->data, VideoDecoder->src_frame->linesize, 0, src_height, VideoDecoder->dst_frame->data, VideoDecoder->dst_frame->linesize);

					Renderer.UpdateTexture(pY, pU, pV);
					Renderer.RenderFrame();

					av_init_packet(VideoEncoder->packet);
					VideoEncoder->packet->size = 0;
					VideoEncoder->packet->data = NULL;

					int got_output = 0;
					int encoded = avcodec_encode_video2(VideoEncoder->codec_ctx, VideoEncoder->packet, VideoDecoder->dst_frame, &got_output);
					if(encoded < 0)
						goto cleanup;
				
					if(got_output){
						OutputFile.Write(VideoEncoder->packet->data, VideoEncoder->packet->size);
						UpdateProgress(++frame, frames_count);
						av_free_packet(VideoEncoder->packet);
					}
				} else {
					#ifndef NO_AUDIO
					av_init_packet(AudioEncoder->packet);
					AudioEncoder->packet->size = 0;
					AudioEncoder->packet->data = NULL;

					int got_output = 0;
					int encoded = avcodec_encode_video2(AudioEncoder->codec_ctx, AudioEncoder->packet, AudioDecoder->frame, &got_output);
					if(encoded < 0)
						goto cleanup;
				
					if(got_output){
						OutputFile.Write(AudioEncoder->packet->data, AudioEncoder->packet->size);
						av_free_packet(AudioEncoder->packet);
					}
					#endif
				}
			}

		}
		
		av_free_packet(VideoDecoder->packet);
    }

    ///////////////////////////////////////////////////////////////////////////////////////

	while(1){

		check_job_status2:
		if(Job.Aborted()){
			res = JOB_CANCELED;
			goto cleanup;
		}

		if(Job.Paused()){
			Sleep(16);
			Renderer.RenderFrame();
			goto check_job_status2;
		}

		int got_frame = 0;
		int decoded = avcodec_decode_video2(VideoDecoder->codec_ctx, VideoDecoder->src_frame, &got_frame, VideoDecoder->packet);
		if(decoded < 0)
			goto cleanup;

		if(!got_frame)
			break;

		sws_scale(VideoDecoder->convert_ctx, VideoDecoder->src_frame->data, VideoDecoder->src_frame->linesize, 0, src_height, VideoDecoder->dst_frame->data, VideoDecoder->dst_frame->linesize);

		Renderer.UpdateTexture(pY, pU, pV);
		Renderer.RenderFrame();

		int got_output = 0;                                                    /* i think this should NOT be commented... test last */
		int encoded = avcodec_encode_video2(VideoEncoder->codec_ctx, VideoEncoder->packet, VideoDecoder->dst_frame, &got_output);
		if(encoded < 0)
			goto cleanup;

		if(got_output){
			OutputFile.Write(VideoEncoder->packet->data, VideoEncoder->packet->size);
			UpdateProgress(++frame, frames_count);
			av_free_packet(VideoEncoder->packet);
		}

		av_free_packet(VideoDecoder->packet);
	}
	
	res = JOB_SUCCEDED;
	
cleanup:

	Renderer.DeleteTexture();
	//Renderer.Render();

	// Write 32 bits "End code" and close the file...
	WriteEndCode(OutputFile);

	// Cleanup everything
	FrameBuffer.Free();

	FreeFrame(&VideoDecoder->src_frame);
	FreeFrame(&VideoDecoder->dst_frame);
	FreePacket(&VideoDecoder->packet);
	FreePacket(&VideoEncoder->packet);	
	FreeCodecCtx(&VideoDecoder->codec_ctx, &VideoDecoder->codec, 0);
	FreeCodecCtx(&VideoEncoder->codec_ctx, &VideoEncoder->codec, 1);	
	FreeFormatCtx(&VideoDecoder->format_ctx);
	FreeConvertCtx(&VideoDecoder->convert_ctx);

	#ifndef NO_AUDIO
	FreeFrame(&AudioDecoder->frame);
	FreeCodecCtx(&AudioDecoder->codec_ctx, &AudioDecoder->codec, 0);
	FreeCodecCtx(&AudioEncoder->codec_ctx, &AudioDecoder->codec, 1);

	AudioDecoder->packet = NULL;
	AudioEncoder->packet = NULL;
	#endif

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

void FreeFrame(AVFrame** frame)
{
	if(*frame){
		av_frame_free(&(*frame));
		*frame = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void FreePacket(AVPacket** packet)
{
	if(*packet){
		free(*packet);
		*packet = NULL;
	}
}

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

///////////////////////////////////////////////////////////////////////////////////////////////

void FreeFormatCtx(AVFormatContext** format_ctx)
{
	if(*format_ctx){
		avformat_close_input(&(*format_ctx));
		*format_ctx = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void FreeConvertCtx(SwsContext** convert_ctx)
{
	if(*convert_ctx){
		sws_freeContext(*convert_ctx);
		*convert_ctx = NULL;
	}
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
	//Renderer.RenderFrame();

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

bool Decode(CFileIO *OutputFile, CConvertContext *ConvertContext, CVideoDecoder *VideoDecoder, CVideoEncoder *VideoEncoder, CPacket *DecoderPacket, CPacket *EncoderPacket, CFrame *SrcFrame, CFrame *DstFrame, int video_stream, int audio_stream, int src_height, BYTE *y, BYTE *u, BYTE *v, bool flushing)
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

			Renderer.UpdateTexture(y, u, v);
			Renderer.RenderFrame();

			EncoderPacket->InitPacket();
			EncoderPacket->SetupPacket(NULL, 0);

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

	DecoderPacket->FreePacket();

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

