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

void CVideoConverter::AdjustSize(int &w, int &h, int max, int align)
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

	if(!FormatContext.AllocContext())
		goto cleanup;

	if(!FormatContext.OpenInput(in))
		goto cleanup;

	if(!FormatContext.FindStreamInfo())
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	VideoStream.FindVideoStream(FormatContext.GetCtx());
	AudioStream.FindAudioStream(FormatContext.GetCtx());

	int video_stream = VideoStream.GetIndex();
	int audio_stream = AudioStream.GetIndex();

	if(video_stream == INVALID_STREAM)
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	if(!VideoDecoder.GetContextFromStream(VideoStream.Get()))
		goto cleanup;

	if(!VideoDecoder.FindDecoder())
		goto cleanup;

	if(!VideoDecoder.OpenCodec())
		goto cleanup;

    ///////////////////////////////////////////////////////////////////////////////////////

	bool got_audio = audio_stream != INVALID_STREAM;

	if(got_audio){

		if(!AudioDecoder.GetContextFromStream(AudioStream.Get()))
			goto cleanup;

		if(!AudioDecoder.FindDecoder())
			goto cleanup;

		//AudioDecoder.GetCtx()->channels = 1;
		//AudioDecoder.GetCtx()->bit_rate = 16;
		//AudioDecoder.GetCtx()->sample_rate = 44100;

		//AudioDecoder.SetAudioSettings(1, 16, 44100);

		if(!AudioDecoder.OpenCodec())
			goto cleanup;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	if(!VideoEncoder.FindEncoder(CODEC_ID_MPEG1VIDEO))
		goto cleanup;

	if(!VideoEncoder.AllocContext())
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	if(got_audio){
	
		if(!AudioEncoder.FindEncoder(AV_CODEC_ID_AAC))
			goto cleanup;

		if(!AudioEncoder.AllocContext())
			goto cleanup;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	int sw = VideoDecoder.GetFrameWidth();
	int sh = VideoDecoder.GetFrameHeight();
	
	int dw = sw;
	int dh = sh;

	AdjustSize(dw, dh, 1024, 16);

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

	if(got_audio){

		AudioEncoder.GetCtx()->channels       = 1;
		AudioEncoder.GetCtx()->bit_rate       = 64000;
		AudioEncoder.GetCtx()->sample_rate    = 44100;
		AudioEncoder.GetCtx()->sample_fmt     = AV_SAMPLE_FMT_S16;
		AudioEncoder.GetCtx()->channel_layout = AV_CH_LAYOUT_MONO;

		if(!AudioEncoder.OpenCodec())
			goto cleanup;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	if(!SndFrame.Alloc())
		goto cleanup;

	if(!SrcFrame.Alloc())
		goto cleanup;

	if(!DstFrame.Alloc())
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	if(!FrameBuffer.Alloc(dw, dh))
		goto cleanup;

	DstFrame.SetupFrameBuffer(FrameBuffer.Get(), dw, dh, df);

    ///////////////////////////////////////////////////////////////////////////////////////

	if(!DecoderPacket.Alloc())
		goto cleanup;

	if(!EncoderPacket.Alloc())
		goto cleanup;
	
    ///////////////////////////////////////////////////////////////////////////////////////

	if(!ConvertContext.GetContext(sw, sh, sf, dw, dh, df))
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	if(!pRenderer->CreateTexture(dw, dh, 4))
		goto cleanup;

	BYTE *y = DstFrame.GetChannel('Y');
	BYTE *u = DstFrame.GetChannel('U');
	BYTE *v = DstFrame.GetChannel('V');

	///////////////////////////////////////////////////////////////////////////////////////

	if(!OutputFile.Create(out))
		goto cleanup;

	///////////////////////////////////////////////////////////////////////////////////////

	int NumFramesDecoded = 0;
	int NumFrames = VideoStream.GetNumFrames();

	UpdateProgress(0, NumFrames);

	///////////////////////////////////////////////////////////////////////////////////////

	bool flush = false;

	while(1){

		int stream = DecoderPacket.GetStreamIndex();

		///////////////////////////////////////////////////////////////////////////////////////

		if(!flush){
			bool res = FormatContext.ReadFrame(DecoderPacket.Get());
			if(!res)
				goto cleanup;
		}

		///////////////////////////////////////////////////////////////////////////////////////

		int got_frame = 0;
		if(stream == video_stream){
			if(!DecodeVideo(VideoDecoder.GetCtx(), SrcFrame.Get(), DecoderPacket.Get(), got_frame))
				goto cleanup;
		} else if(stream == audio_stream && got_audio){
			if(!DecodeAudio(AudioDecoder.GetCtx(), SndFrame.Get(), DecoderPacket.Get(), got_frame))
				goto cleanup;
		}

		///////////////////////////////////////////////////////////////////////////////////////

		if(got_frame){
		
			if(stream == video_stream){
				ScaleFrame(sh);
				RenderFrame(pRenderer, y,u,v);
			}

			EncoderPacket.Reset();

			int got_output = 0;
			if(stream == video_stream){
				if(!EncodeVideo(VideoEncoder.GetCtx(), DstFrame.Get(), EncoderPacket.Get(), got_output))
					goto cleanup;
			} else if(stream == audio_stream && got_audio){
				if(!EncodeAudio(AudioEncoder.GetCtx(), SndFrame.Get(), EncoderPacket.Get(), got_output))
					goto cleanup;
			}

			if(got_output){
				WriteFrame(EncoderPacket.Get());
				UpdateProgress(NumFrames, NumFramesDecoded);
				EncoderPacket.FreePacket();
			}

		} else if(flush){
			DecoderPacket.FreePacket();
			break;
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
		//pRenderer->Render();
	}

	Cleanup();

	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::DecodeVideo(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int &got_frame)
{
	got_frame = 0;
	int res = avcodec_decode_video2(ctx, frame, &got_frame, pkt);
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::DecodeAudio(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int &got_frame)
{
	got_frame = 0;
	int res = avcodec_decode_audio4(ctx, frame, &got_frame, pkt);
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::EncodeVideo(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int &got_frame)
{
	got_frame = 0;
	int res = avcodec_encode_video2(ctx, pkt, frame, &got_frame);
	return res >= 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool CVideoConverter::EncodeAudio(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, int &got_frame)
{
	got_frame = 0;
	int res = avcodec_encode_audio2(ctx, pkt, frame, &got_frame);
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

