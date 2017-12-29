#pragma once
//----------------------------------------------------------------------//
#include "ffmpeg.h"
//----------------------------------------------------------------------//

class CConvertContext {
public:
	CConvertContext();
	~CConvertContext();
private:
	SwsContext *ConvertCtx;
private:
	void Initialize();
	void Cleanup();
public:
	SwsContext* GetCtx();

	bool GetContext(int sw, int sh, AVPixelFormat sf, int dw, int dh, AVPixelFormat df);
	void FreeContext();

	void ScaleFrame(AVFrame *in, AVFrame *out, int height);
};

 