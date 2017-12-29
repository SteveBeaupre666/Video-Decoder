//---------------------------------------------------------------------------

#ifndef SettingsUnitH
#define SettingsUnitH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
//---------------------------------------------------------------------------
class TSettingsForm : public TForm
{
__published:	// IDE-managed Components
	TGroupBox *GroupBoxVideoSettings;
	TLabel *LabelResolution;
	TComboBox *ComboBoxEncoder;
	TLabel *LabelEncoder;
	TComboBox *ComboBoxResolution;
	TComboBox *ComboBoxBitrate;
	TLabel *LabelBitrate;
	TComboBox *ComboBoxFramerate;
	TLabel *LabelFramerate;
	TButton *ButtonCancel;
	TButton *ButtonOk;
private:	// User declarations
public:		// User declarations
	__fastcall TSettingsForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TSettingsForm *SettingsForm;
//---------------------------------------------------------------------------
#endif
