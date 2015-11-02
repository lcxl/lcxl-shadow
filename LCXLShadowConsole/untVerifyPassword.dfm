object frmVerifyPassword: TfrmVerifyPassword
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  BorderStyle = bsSingle
  Caption = #23494#30721#39564#35777
  ClientHeight = 139
  ClientWidth = 249
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
  object lblPassword: TRzLabel
    Left = 32
    Top = 19
    Width = 96
    Height = 12
    Caption = #35831#36755#20837#31649#29702#23494#30721#65306
  end
  object edtPassword: TRzEdit
    Left = 32
    Top = 45
    Width = 179
    Height = 20
    PasswordChar = '*'
    TabOrder = 0
    TextHint = #35831#36755#20837#23494#30721
  end
  object btnOK: TRzButton
    Left = 32
    Top = 88
    Default = True
    Caption = #30830#23450'(&O)'
    TabOrder = 1
    OnClick = btnOKClick
  end
  object btnCancel: TRzButton
    Left = 136
    Top = 88
    Caption = #21462#28040'(C)'
    TabOrder = 2
    OnClick = btnCancelClick
  end
end
