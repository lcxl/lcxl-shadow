object frmSetPassword: TfrmSetPassword
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  BorderStyle = bsSingle
  Caption = #23494#30721#35774#23450
  ClientHeight = 131
  ClientWidth = 314
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poDesktopCenter
  PixelsPerInch = 96
  TextHeight = 13
  object lblPasswordAgain: TRzLabel
    Left = 21
    Top = 59
    Width = 84
    Height = 13
    Caption = #20877#27425#30830#35748#23494#30721#65306
  end
  object lblNewPassword: TRzLabel
    Left = 21
    Top = 19
    Width = 84
    Height = 13
    Caption = #35831#36755#20837#26032#23494#30721#65306
  end
  object edtNewPassword: TRzEdit
    Left = 111
    Top = 16
    Width = 178
    Height = 21
    PasswordChar = '*'
    TabOrder = 0
    TextHint = #35831#36755#20837#26032#23494#30721
  end
  object edtPasswordAgain: TRzEdit
    Left = 111
    Top = 56
    Width = 178
    Height = 21
    PasswordChar = '*'
    TabOrder = 1
    TextHint = #24517#39035#21644#19978#38754#23494#30721#19968#33268
  end
  object btnOK: TRzButton
    Left = 133
    Top = 91
    Default = True
    Caption = #30830#23450'(&O)'
    TabOrder = 2
    OnClick = btnOKClick
  end
  object btnCancel: TRzButton
    Left = 214
    Top = 91
    Caption = #21462#28040'(&C)'
    TabOrder = 3
    OnClick = btnCancelClick
  end
end
