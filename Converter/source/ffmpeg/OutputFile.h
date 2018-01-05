#pragma once
//----------------------------------------------------------------------//
#include "FileIO.h"
//----------------------------------------------------------------------//

#define MPEG_END_CODE 0x000001B7

class COutputFile : public CFileIO {
public:
	bool WriteEndCode();
};
