#pragma once
//----------------------------------------------------------------------//
#include "Buffer.h"
//----------------------------------------------------------------------//

class CFrameBuffer : public CBuffer {
private:
	int CalcYUVSize(int w, int h);
public:
	bool Alloc(int w, int h);
};
