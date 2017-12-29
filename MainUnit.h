//---------------------------------------------------------------------------
#ifndef MainUnitH
#define MainUnitH
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.StdCtrls.hpp>
//---------------------------------------------------------------------------
#include <System.IOUtils.hpp>
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.Menus.hpp>
//---------------------------------------------------------------------------
#include "ConvertUnit.h"
//---------------------------------------------------------------------------
typedef void (__stdcall * PFSETMAINWINDOW)(HWND hWnd);
typedef void (__stdcall * PFSETOPENGLWINDOW)(HWND hWnd);
typedef BOOL (__stdcall * PFINITIALIZEOPENGL)();
typedef void (__stdcall * PFCLEANUPOPENGL)();
typedef void (__stdcall * PFSETBGCOLOR)(float r, float g, float b);
typedef void (__stdcall * PFRENDER)();
typedef void (__stdcall * PFSTARTJOB)(int files_count, char *input_files, char *output_files);
typedef BOOL (__stdcall * PFISJOBRUNNING)();
typedef void (__stdcall * PFCANCELJOB)();
typedef void (__stdcall * PFPAUSEJOB)();
typedef UINT (__stdcall * PFCONVERTVIDEO)(char *input_fname, char *output_fname);
//---------------------------------------------------------------------------
#define WM_UPDATE_PROGRESS		WM_USER + 101
#define WM_THREAD_TERMINATED	WM_USER + 102
//---------------------------------------------------------------------------
class TMainForm : public TForm
{
__published:	// IDE-managed Components
	TGroupBox *GroupBoxInputFiles;
	TListBox *ListBoxInputFiles;
	TGroupBox *GroupBoxOutputFolder;
	TEdit *EditOutputFolder;
	TButton *ButtonBrowse;
	TMainMenu *MainMenu;
	TMenuItem *File1;
	TMenuItem *AddFilesMenu;
	TMenuItem *N1;
	TMenuItem *CloseMenu;
	TMenuItem *AddFolderMenu;
	TMenuItem *EditMenu;
	TMenuItem *OptionsMenu;
	TMenuItem *HelpMenu;
	TMenuItem *RemoveMenu;
	TMenuItem *ClearAllMenu;
	TMenuItem *N2;
	TMenuItem *MoveUpMenu;
	TMenuItem *MoveDownMenu;
	TMenuItem *SettingsMenu;
	TMenuItem *AboutMenu;
	TButton *ButtonConvert;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall ButtonConvertClick(TObject *Sender);
	void __fastcall ButtonBrowseClick(TObject *Sender);
private:	// User declarations
	void __fastcall WndProc(TMessage& Msg);

	//void __fastcall GetAppDir(AnsiString &dir);
	AnsiString __fastcall ChangeFileExt(AnsiString Name, AnsiString Ext);
	AnsiString __fastcall FixPath(AnsiString path);
public:		// User declarations
	PFSETMAINWINDOW    SetMainWindow;
	PFSETOPENGLWINDOW  SetOpenGLWindow;
	PFINITIALIZEOPENGL InitializeOpenGL;
	PFCLEANUPOPENGL    CleanupOpenGL;
	PFSETBGCOLOR       SetBgColor;
	PFRENDER           Render;
	PFSTARTJOB         StartJob;
	PFISJOBRUNNING     IsJobRunning;
	PFCANCELJOB        CancelJob;
	PFPAUSEJOB         PauseJob;
	PFCONVERTVIDEO     ConvertVideo;
public:		// User declarations
	__fastcall TMainForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TMainForm *MainForm;
//---------------------------------------------------------------------------
#endif
