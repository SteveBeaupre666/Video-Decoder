object OpenFolderForm: TOpenFolderForm
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'Select Folder'
  ClientHeight = 435
  ClientWidth = 396
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  PixelsPerInch = 96
  TextHeight = 13
  object ButtonCancel: TButton
    Left = 297
    Top = 402
    Width = 91
    Height = 25
    Caption = 'Cancel'
    Default = True
    ModalResult = 2
    TabOrder = 0
  end
  object ButtonOk: TButton
    Left = 200
    Top = 402
    Width = 91
    Height = 25
    Caption = 'Ok'
    ModalResult = 1
    TabOrder = 1
  end
  object Panel: TPanel
    Left = 8
    Top = 8
    Width = 380
    Height = 388
    TabOrder = 2
    object DirListBox: TDirectoryListBox
      Left = 8
      Top = 8
      Width = 364
      Height = 347
      TabOrder = 0
    end
    object DriveComboBox: TDriveComboBox
      Left = 8
      Top = 361
      Width = 364
      Height = 19
      DirList = DirListBox
      TabOrder = 1
    end
  end
end
