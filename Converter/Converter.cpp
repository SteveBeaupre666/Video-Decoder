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

void EXP_FUNC _StartJob(int files_count, char *input_files, char *output_files)
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

BOOL EXP_FUNC _IsJobRunning()
{
	return Job.IsRunning();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void EXP_FUNC _CancelJob() // <--- could rename this to _AbortThread()
{
	if(Job.IsRunning())
		Job.Abort();
}

///////////////////////////////////////////////////////////////////////////////////////////////

void EXP_FUNC _PauseJob() // <--- could rename this to _PauseThread()
{
	if(Job.IsRunning()){
		switch(Job.IsPaused())
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

	bool JobCanceled = false;

	for(int i = 0; i < n; i++){
	
		int in_len  = GetFileNameLen(in);
		int out_len = GetFileNameLen(out);

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
			JobCanceled = res == JOB_CANCELED;
			break;
		}
	}

	Renderer.DeleteSecondaryContext();

	PostJobDoneMsg(JobCanceled);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

UINT EXP_FUNC _ConvertVideo(char *input_fname, char *output_fname)
{
	UINT res = UNKNOW_ERROR;

	CFileIO OutputFile;
	CBuffer FrameBuffer;

    ///////////////////////////////////////////////////////////////////////////////////////

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

	TEST(FormatContext.AllocContext());
	TEST(FormatContext.OpenInput(input_fname));
	TEST(FormatContext.FindStreamInfo());

    ///////////////////////////////////////////////////////////////////////////////////////

	int video_stream = FormatContext.FindVideoStream();
	int audio_stream = FormatContext.FindAudioStream();
	if(video_stream == INVALID_STREAM || audio_stream == INVALID_STREAM) // a fixer plus tard...
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	AVStream* stream = FormatContext.GetVideoStream();

	TEST(VideoDecoder.AllocContext());
	TEST(VideoDecoder.GetCodecFromStream(stream));
	TEST(VideoDecoder.OpenCodec());

	///////////////////////////////////////////////////////////////////////////////////////

	TEST(SrcFrame.Alloc());
	TEST(DstFrame.Alloc());

	///////////////////////////////////////////////////////////////////////////////////////

	TEST(VideoEncoder.FindEncoder(CODEC_ID_MPEG1VIDEO));
	TEST(VideoEncoder.AllocContext());

    ///////////////////////////////////////////////////////////////////////////////////////

	// Get video settings...
	/*
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
	*/

    ///////////////////////////////////////////////////////////////////////////////////////

	//VideoEncoder.Setup();

	VideoEncoder.OpenCodec();

	//FrameBuffer.Allocate(y_size + u_size + v_size);

	//DstFrame.SetupFrameBuffer(FrameBuffer.Get(), w, h, AV_PIX_FMT_YUV420P);
	
	//ConvertContext.GetContext();

    ///////////////////////////////////////////////////////////////////////////////////////

	DecoderPacket.Allocate();
	EncoderPacket.Allocate();

	DecoderPacket.InitPacket();
	EncoderPacket.InitPacket();

	///////////////////////////////////////////////////////////////////////////////////////

	// Initialize OpenGL
	/*
	Renderer.CreateTexture(dst_width, dst_height, 4);

	BYTE *pY = VideoDecoder->dst_frame->data[0];
	BYTE *pU = VideoDecoder->dst_frame->data[1];
	BYTE *pV = VideoDecoder->dst_frame->data[2];
	*/

	///////////////////////////////////////////////////////////////////////////////////////

	// Create the output file
	//if(!OutputFile.Create(output_fname))
	//	goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	while(av_read_frame(FormatContext.GetCtx(), DecoderPacket.Get()) >= 0){

		int stream_index = DecoderPacket.Get()->stream_index;
		
		//CheckThreadStatus();
	
		if(stream_index == video_stream){


			int got_frame = 0;
			int decoded = avcodec_decode_video2(VideoDecoder.GetCtx(), SrcFrame.Get(), &got_frame, DecoderPacket.Get());	
			if(decoded < 0)
				goto cleanup;


			if(got_frame){
				
				//sws_scale(VideoDecoder->convert_ctx, VideoDecoder->src_frame->data, VideoDecoder->src_frame->linesize, 0, src_height, VideoDecoder->dst_frame->data, VideoDecoder->dst_frame->linesize);

				// Render a frame

				EncoderPacket.InitPacket();

				int got_output = 0;
				int encoded = avcodec_encode_video2(VideoEncoder.GetCtx(), EncoderPacket.Get(), DstFrame.Get(), &got_output);
				if(encoded < 0)
					goto cleanup;
				
				if(got_output){
					
					//OutputFile.Write(VideoEncoder->packet->data, VideoEncoder->packet->size);
					
					// Update progress
					
					EncoderPacket.FreePacket();
				}

			}

		}

		DecoderPacket.FreePacket();
	}

	///////////////////////////////////////////////////////////////////////////////////////

	while(1){

	}

	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	res = JOB_SUCCEDED;

	///////////////////////////////////////////////////////////////////////////////////////

cleanup:

	// perform cleanup...

	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////

/*UINT EXP_FUNC _ConvertVideo(char *input_fname, char *output_fname)
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

			if(Job.IsPaused()){
				Sleep(16);
				Renderer.Render();
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
					Renderer.Render();

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

		if(Job.IsPaused()){
			Sleep(16);
			Renderer.Render();
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
		Renderer.Render();

		int got_output = 0;
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
}*/

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

void UpdateProgress(int frame, int frames_count)
{
	if(hMainWnd){
		
		if(frames_count <= 0 || frame > frames_count)
			frame = frames_count = 0;

		PostMessage(hMainWnd, WM_UPDATE_PROGRESS, frame, frames_count);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void PostJobDoneMsg(bool canceled)
{
	if(hMainWnd)
		PostMessage(hMainWnd, WM_THREAD_TERMINATED, 0, canceled ? 1 : 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NO_AUDIO
bool check_sample_fmt(AVCodec *codec, AVSampleFormat sample_fmt)
{
    AVSampleFormat *p = (AVSampleFormat*)codec->sample_fmts;

    while(*p != AV_SAMPLE_FMT_NONE){
        
		if(*p == sample_fmt)
            return true;

        p++;
    }

    return false;
}
#endif

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

///////////////////////////////////////////////////////////////////////////////////////////////

int GetFileNameLen(char *fname)
{
	int i = 0;
	int len = 0;

	while(1){
		
		char c = fname[i++];

		if(c == NULL)
			break;

		if(c == 0x22){
			if(len == 0){
				continue;
			} else {
				break;
			}
		}

		len++;		
	}

	return len;
}

