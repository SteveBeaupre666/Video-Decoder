#include "OutputFile.h"

bool COutputFile::WriteEndCode()
{
	if(!IsOpened())
		return false;

	static const DWORD code = MPEG_END_CODE;
	return Write((DWORD*)&code, sizeof(DWORD));
}

