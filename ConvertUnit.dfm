object ConvertForm: TConvertForm
  Left = 0
  Top = 0
  BorderStyle = bsSingle
  Caption = 'Converting...'
  ClientHeight = 464
  ClientWidth = 487
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object MainPanel: TPanel
    Left = 8
    Top = 8
    Width = 471
    Height = 417
    TabOrder = 0
    object GaugeFileProgress: TGauge
      Left = 8
      Top = 351
      Width = 455
      Height = 18
      Progress = 0
    end
    object GaugeTotalProgress: TGauge
      Left = 8
      Top = 391
      Width = 455
      Height = 18
      Progress = 0
    end
    object LabelFileProgress: TLabel
      Left = 8
      Top = 335
      Width = 65
      Height = 13
      Caption = 'File Progress:'
    end
    object LabelTotalProgress: TLabel
      Left = 8
      Top = 375
      Width = 73
      Height = 13
      Caption = 'Total Progress:'
    end
    object RenderPanel: TPanel
      Left = 8
      Top = 8
      Width = 455
      Height = 321
      BevelOuter = bvLowered
      TabOrder = 0
      object RenderWindow: TPanel
        Left = 1
        Top = 1
        Width = 453
        Height = 319
        Align = alClient
        BevelOuter = bvNone
        TabOrder = 0
      end
    end
  end
  object ButtonCancel: TButton
    Left = 247
    Top = 431
    Width = 232
    Height = 25
    Caption = 'Cancel'
    TabOrder = 1
    OnClick = ButtonCancelClick
  end
  object ButtonPause: TButton
    Left = 8
    Top = 431
    Width = 233
    Height = 25
    Caption = 'Pause'
    TabOrder = 2
    OnClick = ButtonPauseClick
  end
end
