//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
#include "ConvertUnit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TConvertForm *ConvertForm;
//---------------------------------------------------------------------------
__fastcall TConvertForm::TConvertForm(TComponent* Owner) : TForm(Owner){}
//---------------------------------------------------------------------------
void __fastcall TConvertForm::FormCreate(TObject *Sender)
{
	MainForm->SetOpenGLWindow(RenderWindow->Handle);
	MainForm->InitializeOpenGL();
}
//---------------------------------------------------------------------------
void __fastcall TConvertForm::ResetCancelButton()
{
	ButtonCancel->Caption = "Cancel";
}
//---------------------------------------------------------------------------
void __fastcall TConvertForm::ButtonCancelClick(TObject *Sender)
{
	if(ButtonCancel->Caption == "Cancel"){
		MainForm->AbortThread();
	} else {
		Close();
	}
}
//---------------------------------------------------------------------------
void __fastcall TConvertForm::ButtonPauseClick(TObject *Sender)
{
	MainForm->PauseThread();
}
//---------------------------------------------------------------------------

