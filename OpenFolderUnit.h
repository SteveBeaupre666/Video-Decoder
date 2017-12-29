//---------------------------------------------------------------------------

#ifndef OpenFolderUnitH
#define OpenFolderUnitH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.FileCtrl.hpp>
//---------------------------------------------------------------------------
class TOpenFolderForm : public TForm
{
__published:	// IDE-managed Components
	TButton *ButtonCancel;
	TButton *ButtonOk;
	TPanel *Panel;
	TDirectoryListBox *DirListBox;
	TDriveComboBox *DriveComboBox;
private:	// User declarations
public:		// User declarations
	__fastcall TOpenFolderForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TOpenFolderForm *OpenFolderForm;
//---------------------------------------------------------------------------
#endif
