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

UINT EXP_FUNC _ConvertVideo(char *input_fname, char *output_fname)
{
	CVideoConverter VideoConverter;
	VideoConverter.SetWindow(hMainWnd);
	return VideoConverter.ConvertVideo(input_fname, output_fname, &Renderer, &Job);
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void PostJobDoneMsg(bool canceled)
{
	if(IsWindow(hMainWnd))
		PostMessage(hMainWnd, WM_THREAD_TERMINATED, 0, canceled ? 1 : 0);
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
	PostJobDoneMsg(canceled);

	return 0;
}
