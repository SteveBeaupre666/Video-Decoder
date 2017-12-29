//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "OpenFolderUnit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TOpenFolderForm *OpenFolderForm;
//---------------------------------------------------------------------------
__fastcall TOpenFolderForm::TOpenFolderForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
