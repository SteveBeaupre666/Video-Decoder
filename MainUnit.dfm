object MainForm: TMainForm
  Left = 0
  Top = 0
  BorderStyle = bsSingle
  Caption = 'Video Converter'
  ClientHeight = 434
  ClientWidth = 635
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Menu = MainMenu
  OldCreateOrder = False
  Position = poScreenCenter
  OnClose = FormClose
  OnCloseQuery = FormCloseQuery
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object GroupBoxOutputFolder: TGroupBox
    Left = 8
    Top = 380
    Width = 450
    Height = 46
    Caption = 'Output Folder:'
    TabOrder = 1
    object EditOutputFolder: TEdit
      Left = 8
      Top = 16
      Width = 345
      Height = 21
      ReadOnly = True
      TabOrder = 0
      Text = 'C:\New Programming Folder\Programs\Video Converter\Video'
    end
    object ButtonBrowse: TButton
      Left = 359
      Top = 16
      Width = 83
      Height = 21
      Caption = 'Browse...'
      TabOrder = 1
      OnClick = ButtonBrowseClick
    end
  end
  object GroupBoxInputFiles: TGroupBox
    Left = 8
    Top = 8
    Width = 619
    Height = 366
    Caption = 'Files List:'
    TabOrder = 0
    object ListBoxInputFiles: TListBox
      Left = 8
      Top = 16
      Width = 603
      Height = 342
      ItemHeight = 13
      Items.Strings = (
        
          'C:\New Programming Folder\Programs\Video Converter\Video\small.m' +
          'p4')
      TabOrder = 0
    end
  end
  object ButtonConvert: TButton
    Left = 464
    Top = 386
    Width = 163
    Height = 40
    Caption = 'Convert'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -24
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    TabOrder = 2
    OnClick = ButtonConvertClick
  end
  object MainMenu: TMainMenu
    Left = 32
    Top = 40
    object File1: TMenuItem
      Caption = 'File'
      object AddFilesMenu: TMenuItem
        Caption = 'Add Files...'
      end
      object AddFolderMenu: TMenuItem
        Caption = 'Add Folder...'
      end
      object N1: TMenuItem
        Caption = '-'
      end
      object CloseMenu: TMenuItem
        Caption = 'Close'
      end
    end
    object EditMenu: TMenuItem
      Caption = 'Edit'
      object RemoveMenu: TMenuItem
        Caption = 'Remove'
      end
      object ClearAllMenu: TMenuItem
        Caption = 'Clear All'
      end
      object N2: TMenuItem
        Caption = '-'
      end
      object MoveUpMenu: TMenuItem
        Caption = 'Move Up'
      end
      object MoveDownMenu: TMenuItem
        Caption = 'Move Down'
      end
    end
    object OptionsMenu: TMenuItem
      Caption = 'Options'
      object SettingsMenu: TMenuItem
        Caption = 'Settings...'
      end
    end
    object HelpMenu: TMenuItem
      Caption = 'Help'
      object AboutMenu: TMenuItem
        Caption = 'About'
      end
    end
  end
end
