#include "FileName.h"

int GetFileNameSize(char *fname)
{
	int i   = 0;
	int len = 0;

	while(1){
		
		char c = fname[i++];

		if(!c)
			break;

		if(c == 0x22){

			if(len == 0)
				continue;
			
			break;
		}

		len++;		
	}

	return len;
}

