object frmMain: TfrmMain
  Left = 0
  Top = 0
  BorderIcons = []
  BorderStyle = bsSingle
  ClientHeight = 131
  ClientWidth = 372
  Color = clBtnFace
  Font.Charset = GB2312_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = #23435#20307
  Font.Style = []
  FormStyle = fsStayOnTop
  OldCreateOrder = False
  Position = poDesktopCenter
  PixelsPerInch = 96
  TextHeight = 12
  object lblMsg: TLabel
    Left = 136
    Top = 16
    Width = 36
    Height = 12
    Caption = 'lblMsg'
  end
  object pbSaveData: TProgressBar
    Left = 48
    Top = 82
    Width = 281
    Height = 17
    Smooth = True
    SmoothReverse = True
    TabOrder = 0
  end
  object tmrTest: TTimer
    Interval = 100
    OnTimer = tmrTestTimer
    Left = 312
    Top = 16
  end
end
