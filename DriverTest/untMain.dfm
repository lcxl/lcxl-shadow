object frmMain: TfrmMain
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  BorderStyle = bsSingle
  Caption = #39537#21160#27979#35797
  ClientHeight = 204
  ClientWidth = 427
  Color = clBtnFace
  Font.Charset = GB2312_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = #23435#20307
  Font.Style = []
  OldCreateOrder = False
  Position = poDesktopCenter
  PixelsPerInch = 96
  TextHeight = 12
  object lblFilePath: TLabel
    Left = 24
    Top = 16
    Width = 60
    Height = 12
    Caption = #25991#20214#36335#24452#65306
  end
  object lblStartType: TLabel
    Left = 24
    Top = 72
    Width = 60
    Height = 12
    Caption = #21551#21160#31867#22411#65306
  end
  object lblSerName: TLabel
    Left = 24
    Top = 43
    Width = 60
    Height = 12
    Caption = #26381#21153#21517#31216#65306
  end
  object edtFilePath: TEdit
    Left = 90
    Top = 13
    Width = 260
    Height = 20
    TabOrder = 0
    TextHint = #39537#21160#30340#25991#20214#36335#24452
  end
  object cbbStartType: TComboBox
    Left = 90
    Top = 69
    Width = 308
    Height = 20
    Style = csDropDownList
    ItemIndex = 3
    TabOrder = 1
    Text = #25163#21160#21551#21160'(SERVICE_DEMAND_START)'
    Items.Strings = (
      #24341#23548#21551#21160'(SERVICE_BOOT_START)'
      #31995#32479#21551#21160'(SERVICE_SYSTEM_START)'
      #33258#21160#21551#21160'(SERVICE_AUTO_START)'
      #25163#21160#21551#21160'(SERVICE_DEMAND_START)'
      #31105#29992'(SERVICE_DISABLED)')
  end
  object btnOpenFile: TButton
    Left = 356
    Top = 11
    Width = 42
    Height = 25
    Caption = '...'
    TabOrder = 2
    OnClick = btnOpenFileClick
  end
  object edtSerName: TEdit
    Left = 90
    Top = 40
    Width = 308
    Height = 20
    TabOrder = 3
    TextHint = #39537#21160#30340#26381#21153#21517#31216
  end
  object btnCreateSer: TButton
    Left = 24
    Top = 96
    Width = 89
    Height = 25
    Caption = #21019#24314#26381#21153'(&C)'
    TabOrder = 4
    OnClick = btnCreateSerClick
  end
  object btnStartSer: TButton
    Left = 119
    Top = 96
    Width = 89
    Height = 25
    Caption = #21551#21160#26381#21153'(&S)'
    TabOrder = 5
    OnClick = btnStartSerClick
  end
  object btnStopSer: TButton
    Left = 214
    Top = 96
    Width = 89
    Height = 25
    Caption = #20572#27490#26381#21153'(&S)'
    TabOrder = 6
    OnClick = btnStopSerClick
  end
  object btnDeleteSer: TButton
    Left = 309
    Top = 96
    Width = 89
    Height = 25
    Caption = #21024#38500#26381#21153'(&C)'
    TabOrder = 7
    OnClick = btnDeleteSerClick
  end
  object statMain: TStatusBar
    Left = 0
    Top = 185
    Width = 427
    Height = 19
    Panels = <>
    SimplePanel = True
  end
  object grpFilterOpt: TGroupBox
    Left = 24
    Top = 127
    Width = 374
    Height = 55
    Hint = #36873#25321#36807#28388#39537#21160#36873#39033#20250#33258#21160#23558#39537#21160#22797#21046#21040'System32\Drivers'#30446#24405#19979#12290
    Caption = #36807#28388#39537#21160
    ParentShowHint = False
    ShowHint = True
    TabOrder = 9
    object chkDisk: TCheckBox
      Left = 40
      Top = 24
      Width = 113
      Height = 17
      Caption = #30913#30424#36807#28388#39537#21160
      TabOrder = 0
      OnClick = chkDiskClick
    end
    object chkDiskVol: TCheckBox
      Left = 190
      Top = 23
      Width = 144
      Height = 17
      Caption = #30913#30424#21367#36807#28388#39537#21160
      TabOrder = 1
      OnClick = chkDiskVolClick
    end
  end
  object dlgOpenFile: TOpenDialog
    Filter = #39537#21160#31243#24207'|*.sys'
    Left = 16
  end
end
