object Form1: TForm1
  Left = 192
  Top = 107
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'KStore'
  ClientHeight = 250
  ClientWidth = 345
  Color = clBtnFace
  Font.Charset = SHIFTJIS_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = '‚l‚r ‚oƒSƒVƒbƒN'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCloseQuery = FormCloseQuery
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 12
  object PanelTop: TPanel
    Left = 0
    Top = 0
    Width = 345
    Height = 100
    Align = alTop
    BevelOuter = bvNone
    TabOrder = 0
    OnResize = PanelTopResize
    object LabelPort: TLabel
      Left = 5
      Top = 10
      Width = 60
      Height = 12
      Alignment = taCenter
      AutoSize = False
      Caption = 'Port'
    end
    object LabelProgress: TLabel
      Left = 5
      Top = 82
      Width = 100
      Height = 12
      Alignment = taRightJustify
      AutoSize = False
      Caption = '0/0'
    end
    object ComboPort: TComboBox
      Left = 70
      Top = 5
      Width = 60
      Height = 20
      ItemHeight = 12
      TabOrder = 0
      Text = 'COM1'
      Items.Strings = (
        'COM1'
        'COM2'
        'COM3'
        'COM4'
        'COM5'
        'COM6'
        'COM7'
        'COM8'
        'COM9')
    end
    object ComboUSB: TComboBox
      Left = 135
      Top = 5
      Width = 55
      Height = 20
      Style = csDropDownList
      ItemHeight = 12
      TabOrder = 1
      Items.Strings = (
        'Serial'
        'Modem1'
        'Modem2')
    end
    object ButtonOpen: TButton
      Left = 195
      Top = 5
      Width = 70
      Height = 20
      Caption = '&Open'
      Default = True
      TabOrder = 2
      OnClick = ButtonOpenClick
    end
    object EditOpenDir: TEdit
      Left = 70
      Top = 30
      Width = 250
      Height = 20
      TabOrder = 5
    end
    object EditSaveDir: TEdit
      Left = 70
      Top = 55
      Width = 250
      Height = 20
      TabOrder = 8
    end
    object CheckOpen: TCheckBox
      Left = 5
      Top = 35
      Width = 60
      Height = 12
      Caption = 'OpenDir'
      Checked = True
      State = cbChecked
      TabOrder = 4
    end
    object CheckSave: TCheckBox
      Left = 5
      Top = 60
      Width = 60
      Height = 12
      Caption = 'SaveDir'
      Checked = True
      State = cbChecked
      TabOrder = 7
    end
    object ButtonBrowseOpen: TButton
      Left = 320
      Top = 30
      Width = 20
      Height = 20
      Caption = '...'
      TabOrder = 6
      OnClick = ButtonBrowseOpenClick
    end
    object ButtonBrowseSave: TButton
      Left = 320
      Top = 55
      Width = 20
      Height = 20
      Caption = '...'
      TabOrder = 9
      OnClick = ButtonBrowseSaveClick
    end
    object ProgressBar2: TProgressBar
      Left = 230
      Top = 80
      Width = 110
      Height = 15
      Min = 0
      Max = 100
      Smooth = True
      TabOrder = 11
    end
    object ProgressBar1: TProgressBar
      Left = 110
      Top = 80
      Width = 115
      Height = 15
      Min = 0
      Max = 100
      Smooth = True
      TabOrder = 10
    end
    object ButtonClear: TButton
      Left = 270
      Top = 5
      Width = 70
      Height = 20
      Caption = 'Clea&r'
      TabOrder = 3
      OnClick = ButtonClearClick
    end
  end
  object Log: TRichEdit
    Left = 0
    Top = 100
    Width = 345
    Height = 150
    Align = alClient
    HideScrollBars = False
    ImeMode = imDisable
    PlainText = True
    ReadOnly = True
    ScrollBars = ssVertical
    TabOrder = 1
    WantReturns = False
    OnChange = LogChange
  end
end
