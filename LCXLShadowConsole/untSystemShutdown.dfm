object frmSystemShutdown: TfrmSystemShutdown
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  BorderStyle = bsNone
  Caption = 'LCXLShadow'#31995#32479#20851#26426#23545#35805#26694
  ClientHeight = 446
  ClientWidth = 734
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  FormStyle = fsStayOnTop
  OldCreateOrder = False
  Position = poDesktopCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object imgBackgrand: TImage
    Left = 0
    Top = 0
    Width = 64
    Height = 64
    AutoSize = True
  end
  object gpnl1: TRzGridPanel
    Left = 0
    Top = 0
    Width = 734
    Height = 446
    Transparent = True
    Align = alClient
    ColumnCollection = <
      item
        Value = 50.000000000000000000
      end
      item
        SizeStyle = ssAbsolute
        Value = 500.000000000000000000
      end
      item
        Value = 50.000000000000000000
      end>
    ControlCollection = <
      item
        Column = 1
        Control = pnl
        Row = 1
      end>
    RowCollection = <
      item
        Value = 50.000000000000000000
      end
      item
        SizeStyle = ssAbsolute
        Value = 200.000000000000000000
      end
      item
        Value = 50.000000000000000000
      end>
    TabOrder = 0
    object pnl: TRzPanel
      Left = 117
      Top = 123
      Width = 500
      Height = 200
      Align = alClient
      BorderInner = fsFlatRounded
      BorderOuter = fsFlatRounded
      BorderColor = clBlue
      BorderWidth = 1
      TabOrder = 0
      object lblLCXLShadow: TRzLabel
        AlignWithMargins = True
        Left = 8
        Top = 8
        Width = 484
        Height = 72
        Align = alTop
        Alignment = taCenter
        Caption = 'LCXLShadow'#24433#23376#36824#21407#31995#32479#13#10#20851#26426#23545#35805#26694
        Font.Charset = GB2312_CHARSET
        Font.Color = clWindowText
        Font.Height = -35
        Font.Name = #23435#20307
        Font.Style = [fsBold, fsItalic]
        ParentFont = False
        Transparent = True
        TextStyle = tsShadow
        ExplicitWidth = 408
      end
      object gpnlbtns: TRzGridPanel
        AlignWithMargins = True
        Left = 8
        Top = 127
        Width = 484
        Height = 65
        BorderOuter = fsNone
        Align = alClient
        ColumnCollection = <
          item
            Value = 33.333333333333340000
          end
          item
            Value = 33.333333333333340000
          end
          item
            Value = 33.333333333333340000
          end>
        ControlCollection = <
          item
            Column = 0
            Control = btnShutdown
            Row = 0
          end
          item
            Column = 1
            Control = btnReset
            Row = 0
          end
          item
            Column = 2
            Control = btnCancel
            Row = 0
          end>
        RowCollection = <
          item
            Value = 100.000000000000000000
          end>
        TabOrder = 0
        DesignSize = (
          484
          65)
        object btnShutdown: TRzButton
          Left = 30
          Top = 15
          Width = 100
          Height = 35
          Anchors = []
          Caption = #20851#26426'(&S)'
          HotTrack = True
          TabOrder = 0
          OnClick = btnShutdownClick
        end
        object btnReset: TRzButton
          Left = 191
          Top = 15
          Width = 100
          Height = 35
          Anchors = []
          Caption = #37325#21551'(&R)'
          HotTrack = True
          TabOrder = 1
          OnClick = btnResetClick
        end
        object btnCancel: TRzButton
          Left = 353
          Top = 15
          Width = 100
          Height = 35
          Anchors = []
          Caption = #21462#28040'(&C)'
          HotTrack = True
          TabOrder = 2
          OnClick = btnCancelClick
        end
      end
      object btnShadowSetting: TButton
        Left = 5
        Top = 83
        Width = 490
        Height = 41
        Align = alTop
        Style = bsCommandLink
        TabOrder = 1
        OnClick = btnShadowSettingClick
      end
    end
  end
  object tmrTrue: TTimer
    Interval = 100
    OnTimer = tmrTrueTimer
    Left = 65480
    Top = 65448
  end
end
