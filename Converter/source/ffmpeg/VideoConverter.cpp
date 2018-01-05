#include "VideoConverter.h"

CVideoConverter::CVideoConverter()
{
	Initialize();
}

///////////////////////////////////////////////////////////////////////////////////////////////

CVideoConverter::~CVideoConverter()
{
	Cleanup();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void CVideoConverter::Initialize()
{
	hMainWnd = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CVideoConverter::Cleanup()
{
	FrameBuffer.Free();

	SndFrame.Free();
	SrcFrame.Free();
	DstFrame.Free();

	DecoderPacket.Free();
	EncoderPacket.Free();

	VideoDecoder.FreeContext();
	VideoEncoder.FreeContext();

	AudioDecoder.FreeContext();
	AudioEncoder.FreeContext();

	FormatContext.FreeContext();
	ConvertContext.FreeContext();

	Initialize();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void CVideoConverter::SetWindow(HWND hWnd)
{
	hMainWnd = hWnd;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void CVideoConverter::UpdateProgress(int frame, int total)
{
	if(IsWindow(hMainWnd)){

		if(total <= 0 || frame > total)
			frame = total = 0;

		PostMessage(hMainWnd, WM_UPDATE_PROGRESS, frame, total);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CVideoConverter::AdjustFrameSize(int &w, int &h, int max, int align)
{
	if(max > 1){
		while(w > max || h > max){
			w >>= 1;
			h >>= 1;

			if(w <= 1 || h <= 1)
				break;
		}
	}

	if(align > 1){
		int gw = w % align;
		int gh = h % align;
		if(gw != 0){w += align - gw;}
		if(gh != 0){h += align - gh;}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

UINT CVideoConverter::ConvertVideo(char *in, char *out, CRenderer *pRenderer, CThread *pThread)
{
	UINT res = UNKNOW_ERROR;

    ///////////////////////////////////////////////////////////////////////////////////////

	TEST(FormatContext.AllocContext());
	TEST(FormatContext.OpenInput(in));
	TEST(FormatContext.FindStreamInfo());

    ///////////////////////////////////////////////////////////////////////////////////////

	VideoStream.FindVideoStream(FormatContext.GetCtx());
	AudioStream.FindAudioStream(FormatContext.GetCtx());

	int video_stream = VideoStream.GetIndex();
	int audio_stream = AudioStream.GetIndex();

	if(video_stream == INVALID_STREAM)
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	TEST(VideoDecoder.GetContextFromStream(VideoStream.Get()));
	TEST(VideoDecoder.FindDecoder());
	TEST(VideoDecoder.OpenCodec());

    ///////////////////////////////////////////////////////////////////////////////////////

	bool got_audio = false; // audio_stream != INVALID_STREAM;

	if(got_audio){
		TEST(AudioDecoder.GetContextFromStream(AudioStream.Get()));
		TEST(AudioDecoder.FindDecoder());
		TEST(AudioDecoder.OpenCodec());
	}

	///////////////////////////////////////////////////////////////////////////////////////

	TEST(VideoEncoder.FindEncoder(CODEC_ID_MPEG1VIDEO));
	TEST(VideoEncoder.AllocContext());

	///////////////////////////////////////////////////////////////////////////////////////

	if(got_audio){
		TEST(AudioEncoder.FindEncoder(AV_CODEC_ID_AAC));
		TEST(AudioEncoder.AllocContext());
	}

	///////////////////////////////////////////////////////////////////////////////////////

	int sw = VideoDecoder.GetFrameWidth();
	int sh = VideoDecoder.GetFrameHeight();
	
	int dw = sw;
	int dh = sh;

	AdjustFrameSize(dw, dh, 1024, 16);

	AVPixelFormat sf = VideoDecoder.GetPixelFormat();
	AVPixelFormat df = AV_PIX_FMT_YUV420P;

	///////////////////////////////////////////////////////////////////////////////////////

	VideoEncoder.SetFormat(df);
	VideoEncoder.SetSize(dw, dh);
	VideoEncoder.SetBitrate(2000000);
	VideoEncoder.SetFramerate(1, 25);
	VideoEncoder.SetGopSize(10);
	VideoEncoder.SetMaxBFrames(1);

	TEST(VideoEncoder.OpenCodec());

	///////////////////////////////////////////////////////////////////////////////////////

	if(got_audio){

		AudioEncoder.GetCtx()->channels       = 1;
		AudioEncoder.GetCtx()->bit_rate       = 64000;
		AudioEncoder.GetCtx()->sample_rate    = 44100;
		AudioEncoder.GetCtx()->sample_fmt     = AV_SAMPLE_FMT_S16;
		AudioEncoder.GetCtx()->channel_layout = AV_CH_LAYOUT_MONO;

		TEST(AudioEncoder.OpenCodec());
	}

	///////////////////////////////////////////////////////////////////////////////////////

	TEST(SndFrame.Alloc());
	TEST(SrcFrame.Alloc());
	TEST(DstFrame.Alloc());

	///////////////////////////////////////////////////////////////////////////////////////

	TEST(FrameBuffer.Alloc(dw, dh));

	DstFrame.SetupFrameBuffer(FrameBuffer.Get(), dw, dh, df);

    ///////////////////////////////////////////////////////////////////////////////////////

	TEST(DecoderPacket.Alloc());
	TEST(EncoderPacket.Alloc());
	
    ///////////////////////////////////////////////////////////////////////////////////////

	TEST(ConvertContext.GetContext(sw, sh, sf, dw, dh, df));

	///////////////////////////////////////////////////////////////////////////////////////

	TEST(pRenderer->CreateTexture(dw, dh, 4));

	BYTE *y = DstFrame.GetChannel('Y');
	BYTE *u = DstFrame.GetChannel('U');
	BYTE *v = DstFrame.GetChannel('V');

	///////////////////////////////////////////////////////////////////////////////////////

	TEST(OutputFile.Create(out));

	///////////////////////////////////////////////////////////////////////////////////////

	int NumFramesDecoded = 0;
	int NumFrames = VideoStream.GetNumFrames();

	UpdateProgress(NumFramesDecoded, NumFrames);

	///////////////////////////////////////////////////////////////////////////////////////

	bool flushing = false;

	while(1){

		if(Aborted(pThread))
			goto cleanup;

		if(Paused(pThread))
			Throttle(pThread, pRenderer);

		if(!flushing){
			bool res = FormatContext.ReadFrame(DecoderPacket.Get());
			if(!res)
				flushing = true;
		}

		int stream = !flushing ? DecoderPacket.GetStreamIndex() : video_stream;

		if(stream == video_stream){

			int got_frame = 0;
			if(stream == video_stream){
				TEST(DecodeVideo(VideoDecoder.GetCtx(), SrcFrame.Get(), DecoderPacket.Get(), &got_frame));
			} else {
				TEST(DecodeAudio(AudioDecoder.GetCtx(), SndFrame.Get(), DecoderPacket.Get(), &got_frame));
			}

			if(!got_frame && flushing)
				break;

			if(got_frame){

				NumFramesDecoded++;

				if(stream == video_stream){
					ScaleFrame(sh);
					RenderFrame(pRenderer, y,u,v);
				}

				EncoderPacket.Reset();

				int got_output = 0;
				if(stream == video_stream){
					TEST(EncodeVideo(VideoEncoder.GetCtx(), DstFrame.Get(), EncoderPacket.Get(), &got_output));
				} else {
					TEST(EncodeAudio(AudioEncoder.GetCtx(), SndFrame.Get(), EncoderPacket.Get(), &got_output));
				}

				if(got_output){

					WriteFrame(EncoderPacket.Get());
					EncoderPacket.FreePacket();

					if(stream == video_stream)
						UpdateProgress(NumFramesDecoded, NumFrames);
				}
			}

		}

		DecoderPacket.FreePacket();
	}

	///////////////////////////////////////////////////////////////////////////////////////
	res = JOB_SUCCEDED;
	///////////////////////////////////////////////////////////////////////////////////////

cleanup:

	CloseOutputFile();

	if(pRenderer){
		pRenderer->DeleteTexture();
		pRenderer->Render();
	}

	Cleanup();

	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::DecodeVideo(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int *got_frame)
{
	*got_frame = 0;
	int res = avcodec_decode_video2(ctx, frame, got_frame, pkt);
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::DecodeAudio(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int *got_frame)
{
	*got_frame = 0;
	int res = avcodec_decode_audio4(ctx, frame, got_frame, pkt);
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::EncodeVideo(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int *got_output)
{
	*got_output = 0;
	int res = avcodec_encode_video2(ctx, pkt, frame, got_output);
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::EncodeAudio(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int *got_output)
{
	*got_output = 0;
	int res = avcodec_encode_audio2(ctx, pkt, frame, got_output);
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void CVideoConverter::ScaleFrame(int h)
{
	AVFrame *src = SrcFrame.Get();
	AVFrame *dst = DstFrame.Get();

	ConvertContext.Scale(src, dst, h);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CVideoConverter::RenderFrame(CRenderer* pRenderer, BYTE *y, BYTE *u, BYTE *v)
{
	if(pRenderer){
		pRenderer->UpdateTexture(y,u,v);
		pRenderer->Render();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::WriteFrame(AVPacket *pkt)
{
	return OutputFile.Write(pkt->data, pkt->size);
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::CloseOutputFile()
{
	bool res = false;

	if(OutputFile.IsOpened()){
		res = OutputFile.WriteEndCode();
		OutputFile.Close();
	}

	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::Aborted(CThread *pThread)
{
	if(pThread)
		return pThread->Aborted();

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::Paused(CThread *pThread)
{
	if(pThread)
		return pThread->Paused();

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::Throttle(CThread *pThread, CRenderer *pRenderer)
{
	if(!pThread)
		return false;

	while(1){
	
		if(Aborted(pThread))
			return false;
	
		if(!Paused(pThread))
			break;
		
		if(pRenderer)
			pRenderer->Render();
		
		Sleep(16);
	}

	return true;
}

