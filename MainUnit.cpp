//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
#include "MainUnit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm *MainForm;
//---------------------------------------------------------------------------
AnsiString AppDir;
AnsiString SettingsFileName;
//---------------------------------------------------------------------------
HINSTANCE hDll = NULL;
//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner):TForm(Owner){}
//---------------------------------------------------------------------------
/*void __fastcall TMainForm::GetAppDir(AnsiString &dir)
{
	GetDir(0, dir);
	int len = strlen(dir.c_str());
	if(len > 0 && dir[len] != '\')
		dir = dir + "\";
}*/
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormCreate(TObject *Sender)
{
	AppDir = "C:\\New Programming Folder\\Programs\\Video Converter\\";
	SetCurrentDirectoryA(AppDir.c_str());
	SettingsFileName = AppDir + "Settings.bin";

	if(!LoadDll(AnsiString("Converter.dll")))
		Application->Terminate();

	//LoadSettings();

	SetMainWindow(this->Handle);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	CanClose = !IsThreadRunning();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormClose(TObject *Sender, TCloseAction &Action)
{
	//SaveSettings();

	FreeDll();
}
//---------------------------------------------------------------------------
bool __fastcall TMainForm::LoadDll(AnsiString DllName)
{
	hDll = LoadLibraryA(DllName.c_str());

	if(hDll){

		SetMainWindow    = (PFSETMAINWINDOW)GetProcAddress(hDll, "_SetMainWindow");
		SetOpenGLWindow  = (PFSETOPENGLWINDOW)GetProcAddress(hDll, "_SetOpenGLWindow");
		InitializeOpenGL = (PFINITIALIZEOPENGL)GetProcAddress(hDll, "_InitializeOpenGL");
		CleanupOpenGL    = (PFCLEANUPOPENGL)GetProcAddress(hDll, "_CleanupOpenGL");
		SetBgColor       = (PFSETBGCOLOR)GetProcAddress(hDll, "_SetBgColor");
		Render           = (PFRENDER)GetProcAddress(hDll, "_Render");
		StartThread      = (PFSTARTTHREAD)GetProcAddress(hDll, "_StartThread");
		IsThreadRunning  = (PFISTHREADRUNNING)GetProcAddress(hDll, "_IsThreadRunning");
		AbortThread      = (PFABORTTHREAD)GetProcAddress(hDll, "_AbortThread");
		PauseThread      = (PFPAUSETHREAD)GetProcAddress(hDll, "_PauseThread");
		ConvertVideo     = (PFCONVERTVIDEO)GetProcAddress(hDll, "_ConvertVideo");

		if(!SetOpenGLWindow ||
		   !InitializeOpenGL ||
		   !CleanupOpenGL ||
		   !SetBgColor ||
		   !Render ||
		   !StartThread ||
		   !IsThreadRunning ||
		   !AbortThread ||
		   !PauseThread ||
		   !ConvertVideo)
		{
			return false;
		}

	} else {
		ShowMessage(DllName + " not found.");
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FreeDll()
{
	if(hDll){
	  CleanupOpenGL();
	  FreeLibrary(hDll);
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::WndProc(TMessage& Msg)
{
	switch(Msg.Msg)
	{
	case WM_UPDATE_PROGRESS:
		{
			int frame = Msg.WParam;
			int num_frames = Msg.LParam;

			if(num_frames > 0){
				ConvertForm->GaugeFileProgress->MaxValue = num_frames;
				ConvertForm->GaugeFileProgress->Progress = frame;
				Application->ProcessMessages();
			}
		}
		break;

	case WM_THREAD_TERMINATED:
		{
			if(Msg.LParam == 0){
				ConvertForm->Hide();
				//EnableUI(True);
				//MainForm.Enabled = true;
			} else {
				ConvertForm->ButtonCancel->Caption = "Close";
			}

		}
		break;
	}

	try {
		Dispatch(&Msg);
	}
	catch (...) {
		Application->HandleException(this);
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ButtonBrowseClick(TObject *Sender)
{
//
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ButtonConvertClick(TObject *Sender)
{
	ConvertForm->ResetCancelButton();
	ConvertForm->Show();
	Application->ProcessMessages();

	int FilesCount = ListBoxInputFiles->Items->Count;
	if(FilesCount <= 0)
		return;

	//EnableUI(False);

	AnsiString InputFiles  = "";
	AnsiString OutputFiles = "";

	for(int i = 0; i < FilesCount; i++){

		AnsiString InputFileName  = ListBoxInputFiles->Items->Strings[i];
		AnsiString OutputFilePath = EditOutputFolder->Text;
		AnsiString OutputFileName = FixPath(OutputFilePath) + ExtractFileName(ChangeFileExt(InputFileName, ".mpg"));

		InputFiles  = InputFiles  + '"' + InputFileName  + '"';
		OutputFiles = OutputFiles + '"' + OutputFileName + '"';

		if(i < FilesCount - 1){
			InputFiles  = InputFiles  + " ";
			OutputFiles = OutputFiles + " ";
		}
	}

	StartThread(FilesCount, InputFiles.c_str(), OutputFiles.c_str());
}
//---------------------------------------------------------------------------
AnsiString __fastcall TMainForm::ChangeFileExt(AnsiString Name, AnsiString Ext)
{
	int NameLen, ExtLen, ExtPos;
	AnsiString OldName, OldExt, NewName, NewExt;

	NewExt  = Ext;
	NewName = Name;
	OldName = ExtractFileName(Name);
	NameLen = OldName.Length();
	OldExt  = ExtractFileExt(OldName);
	ExtLen  = OldExt.Length();

	if(ExtLen > 0){
	  ExtPos = (NameLen - ExtLen) + 1;
	  OldName.Delete(ExtPos, ExtLen);
	  NewName = OldName + NewExt;
	}

	return NewName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

AnsiString __fastcall TMainForm::FixPath(AnsiString path)
{
	char backslash = 0x92;
	int len = path.Length();
	char *s = path.c_str();
	if(s[len-1] != backslash)
		path = path + backslash;

	return path;
}

