//---------------------------------------------------------------------------
#ifndef ConvertUnitH
#define ConvertUnitH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Samples.Gauges.hpp>
//---------------------------------------------------------------------------
#include "MainUnit.h"
//---------------------------------------------------------------------------
class TConvertForm : public TForm
{
__published:	// IDE-managed Components
	TPanel *MainPanel;
	TPanel *RenderPanel;
	TGauge *GaugeFileProgress;
	TGauge *GaugeTotalProgress;
	TLabel *LabelFileProgress;
	TLabel *LabelTotalProgress;
	TButton *ButtonCancel;
	TButton *ButtonPause;
	TPanel *RenderWindow;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall ButtonCancelClick(TObject *Sender);
	void __fastcall ButtonPauseClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TConvertForm(TComponent* Owner);
	void __fastcall ResetCancelButton();
};
//---------------------------------------------------------------------------
extern PACKAGE TConvertForm *ConvertForm;
//---------------------------------------------------------------------------
#endif
