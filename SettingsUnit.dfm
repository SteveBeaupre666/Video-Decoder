object SettingsForm: TSettingsForm
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'Settings'
  ClientHeight = 176
  ClientWidth = 353
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
  object GroupBoxVideoSettings: TGroupBox
    Left = 8
    Top = 8
    Width = 337
    Height = 129
    Caption = 'Video Settings'
    TabOrder = 0
    object LabelResolution: TLabel
      Left = 175
      Top = 24
      Width = 54
      Height = 13
      Caption = 'Resolution:'
    end
    object LabelEncoder: TLabel
      Left = 16
      Top = 24
      Width = 43
      Height = 13
      Caption = 'Encoder:'
    end
    object LabelBitrate: TLabel
      Left = 175
      Top = 70
      Width = 36
      Height = 13
      Caption = 'Bitrate:'
    end
    object LabelFramerate: TLabel
      Left = 16
      Top = 70
      Width = 54
      Height = 13
      Caption = 'Framerate:'
    end
    object ComboBoxEncoder: TComboBox
      Left = 16
      Top = 43
      Width = 145
      Height = 21
      Style = csDropDownList
      ItemIndex = 0
      TabOrder = 0
      Text = 'MPEG'
      Items.Strings = (
        'MPEG')
    end
    object ComboBoxResolution: TComboBox
      Left = 175
      Top = 43
      Width = 145
      Height = 21
      Style = csDropDownList
      ItemIndex = 0
      TabOrder = 1
      Text = 'Keep Original'
      Items.Strings = (
        'Keep Original'
        '320 x 240'
        '640 x 480'
        '720 x 480'
        '1280 x 720'
        '1360 x 768')
    end
    object ComboBoxBitrate: TComboBox
      Left = 175
      Top = 89
      Width = 145
      Height = 21
      Style = csDropDownList
      ItemIndex = 0
      TabOrder = 2
      Text = '2000 kbps'
      Items.Strings = (
        '2000 kbps')
    end
    object ComboBoxFramerate: TComboBox
      Left = 16
      Top = 89
      Width = 145
      Height = 21
      Style = csDropDownList
      ItemIndex = 4
      TabOrder = 3
      Text = '25 fps'
      Items.Strings = (
        'Keep Original'
        '20 fps'
        '24 fps'
        '24.946 fps'
        '25 fps'
        '29.97 fps'
        '30 fps')
    end
  end
  object ButtonCancel: TButton
    Left = 254
    Top = 143
    Width = 91
    Height = 25
    Caption = 'Cancel'
    Default = True
    ModalResult = 2
    TabOrder = 1
  end
  object ButtonOk: TButton
    Left = 157
    Top = 143
    Width = 91
    Height = 25
    Caption = 'Ok'
    ModalResult = 1
    TabOrder = 2
  end
end
