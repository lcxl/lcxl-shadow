object frmMain: TfrmMain
  Left = 0
  Top = 0
  Caption = 'LCXLShadow '#24433#23376#31995#32479#25511#21046#31471
  ClientHeight = 403
  ClientWidth = 608
  Color = clBtnFace
  DoubleBuffered = True
  Font.Charset = GB2312_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = #23435#20307
  Font.Style = []
  OldCreateOrder = False
  Position = poDesktopCenter
  OnClose = FormClose
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 12
  object pgcMain: TRzPageControl
    AlignWithMargins = True
    Left = 3
    Top = 3
    Width = 602
    Height = 378
    Hint = ''
    ActivePage = tsMain
    Align = alClient
    AllowTabDragging = True
    BoldCurrentTab = True
    Font.Charset = GB2312_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = #23435#20307
    Font.Style = []
    ParentFont = False
    TabHeight = 50
    TabIndex = 0
    TabOrder = 0
    TabOrientation = toLeft
    TabSequence = tsReverse
    TabStyle = tsRoundCorners
    FixedDimension = 47
    object tsMain: TRzTabSheet
      OnShow = tsMainShow
      Caption = #20027#31383#21475
      object lvDisk: TListView
        AlignWithMargins = True
        Left = 3
        Top = 55
        Width = 544
        Height = 187
        Align = alClient
        Columns = <
          item
            Caption = #29366#24577
            Width = 120
          end
          item
            Caption = #30913#30424
            Width = 120
          end
          item
            Caption = #21487#29992#23481#37327'/'#24635#23481#37327
            Width = 245
          end>
        GridLines = True
        Groups = <
          item
            Header = #24433#23376#27169#24335
            Footer = #24433#23376#27169#24335
            GroupID = 0
            State = [lgsNormal]
            HeaderAlign = taLeftJustify
            FooterAlign = taLeftJustify
            Subtitle = #24433#23376#27169#24335
            TitleImage = -1
          end
          item
            Header = #27491#24120#27169#24335
            Footer = #27491#24120#27169#24335
            GroupID = 1
            State = [lgsNormal]
            HeaderAlign = taLeftJustify
            FooterAlign = taLeftJustify
            Subtitle = #27491#24120#27169#24335
            TitleImage = -1
          end>
        IconOptions.AutoArrange = True
        OwnerData = True
        GroupView = True
        ReadOnly = True
        RowSelect = True
        ParentShowHint = False
        ShowWorkAreas = True
        ShowHint = False
        TabOrder = 0
        ViewStyle = vsReport
        OnClick = lvDiskClick
        OnData = lvDiskData
        OnDblClick = lvDiskDblClick
        OnInfoTip = lvDiskInfoTip
        OnSelectItem = lvDiskSelectItem
      end
      object gbDriverStatus: TRzGroupBox
        AlignWithMargins = True
        Left = 3
        Top = 3
        Width = 544
        Height = 46
        Align = alTop
        Caption = #31995#32479#29366#24577
        FrameController = frmctrlMain
        GroupStyle = gsBanner
        TabOrder = 1
        object grdpnlStatus: TRzGridPanel
          Left = 0
          Top = 20
          Width = 544
          Height = 26
          BorderOuter = fsNone
          Align = alClient
          ColumnCollection = <
            item
              SizeStyle = ssAbsolute
              Value = 90.000000000000000000
            end
            item
              Value = 50.000000000000000000
            end
            item
              SizeStyle = ssAbsolute
              Value = 90.000000000000000000
            end
            item
              Value = 50.000000000000000000
            end>
          ControlCollection = <
            item
              Column = 1
              Control = lblCurShadowMode
              Row = 0
            end
            item
              Column = 0
              Control = lblCurShadowModeLabel
              Row = 0
            end
            item
              Column = 2
              Control = lblNextShadowModeLabel
              Row = 0
            end
            item
              Column = 3
              Control = lblNextShadowMode
              Row = 0
            end>
          RowCollection = <
            item
              Value = 100.000000000000000000
            end>
          TabOrder = 0
          DesignSize = (
            544
            26)
          object lblCurShadowMode: TRzLabel
            Left = 178
            Top = 7
            Width = 6
            Height = 12
            Hint = #28857#20987#21487#36827#20837#24433#23376#27169#24335#65292#38548#32477#19968#20999#21361#38505#65281
            Anchors = []
            Font.Charset = GB2312_CHARSET
            Font.Color = clHighlight
            Font.Height = -12
            Font.Name = #23435#20307
            Font.Style = []
            ParentFont = False
            ParentShowHint = False
            ShowHint = True
            OnClick = lblCurShadowModeClick
            Blinking = True
            BlinkColor = clBlue
            FlyByColor = clBlue
            TextStyle = tsRaised
            ExplicitLeft = 63
            ExplicitTop = 14
          end
          object lblCurShadowModeLabel: TRzLabel
            Left = 3
            Top = 7
            Width = 84
            Height = 12
            Anchors = []
            Caption = #24403#21069#24433#23376#27169#24335#65306
            Font.Charset = GB2312_CHARSET
            Font.Color = clBlack
            Font.Height = -12
            Font.Name = #23435#20307
            Font.Style = []
            ParentFont = False
            ExplicitLeft = 18
            ExplicitTop = 14
          end
          object lblNextShadowModeLabel: TRzLabel
            Left = 275
            Top = 7
            Width = 84
            Height = 12
            Anchors = []
            Caption = #19979#27425#21551#21160#27169#24335#65306
            Font.Charset = GB2312_CHARSET
            Font.Color = clBlack
            Font.Height = -12
            Font.Name = #23435#20307
            Font.Style = []
            ParentFont = False
            ExplicitLeft = 212
            ExplicitTop = 14
          end
          object lblNextShadowMode: TRzLabel
            Left = 450
            Top = 7
            Width = 6
            Height = 12
            Anchors = []
            Font.Charset = GB2312_CHARSET
            Font.Color = clHighlight
            Font.Height = -12
            Font.Name = #23435#20307
            Font.Style = []
            ParentFont = False
            OnClick = lblNextShadowModeClick
            FlyByColor = clBlue
            FlyByEnabled = True
            TextStyle = tsRaised
            ExplicitLeft = 353
            ExplicitTop = 14
          end
        end
      end
      object gpnlLow: TRzGridPanel
        Left = 0
        Top = 285
        Width = 550
        Height = 91
        Align = alBottom
        ColumnCollection = <
          item
            Value = 50.000000000000000000
          end
          item
            Value = 50.000000000000000000
          end>
        ControlCollection = <
          item
            Column = 0
            Control = gbDiskStatus
            Row = 0
          end
          item
            Column = 1
            Control = gbSaveData
            Row = 0
          end>
        RowCollection = <
          item
            Value = 100.000000000000000000
          end>
        TabOrder = 2
        object gbDiskStatus: TRzGroupBox
          AlignWithMargins = True
          Left = 5
          Top = 5
          Width = 267
          Height = 81
          Align = alClient
          Caption = #30913#30424#29366#24577
          Enabled = False
          FrameController = frmctrlMain
          GroupStyle = gsBanner
          TabOrder = 0
          object lblShadowFreeSizeTitle: TRzLabel
            Left = 16
            Top = 58
            Width = 84
            Height = 12
            Caption = #24433#23376#21097#20313#31354#38388#65306
            Enabled = False
          end
          object lblShadowFreeSize: TRzLabel
            Left = 106
            Top = 58
            Width = 6
            Height = 12
            Enabled = False
            Font.Charset = GB2312_CHARSET
            Font.Color = clHighlight
            Font.Height = -12
            Font.Name = #23435#20307
            Font.Style = []
            ParentColor = False
            ParentFont = False
          end
          object lblIsProtectTitle: TRzLabel
            Left = 16
            Top = 34
            Width = 84
            Height = 12
            Caption = #24433#23376#36824#21407#27169#24335#65306
            Enabled = False
          end
          object lblIsProtect: TRzLabel
            Left = 106
            Top = 34
            Width = 6
            Height = 12
            Enabled = False
            Font.Charset = GB2312_CHARSET
            Font.Color = clHighlight
            Font.Height = -12
            Font.Name = #23435#20307
            Font.Style = []
            ParentColor = False
            ParentFont = False
          end
        end
        object gbSaveData: TRzGroupBox
          AlignWithMargins = True
          Left = 278
          Top = 5
          Width = 267
          Height = 81
          Align = alClient
          Caption = #24433#23376#25345#20037#21270
          Enabled = False
          GroupStyle = gsBanner
          TabOrder = 1
          object grdpnl1: TRzGridPanel
            Left = 0
            Top = 20
            Width = 267
            Height = 61
            BorderOuter = fsNone
            Align = alClient
            ColumnCollection = <
              item
                Value = 50.000000000000000000
              end
              item
                SizeStyle = ssAbsolute
                Value = 86.000000000000000000
              end
              item
                Value = 50.000000000000000000
              end>
            ControlCollection = <
              item
                Column = 1
                Control = smobtnSaveData
                Row = 1
              end>
            RowCollection = <
              item
                Value = 50.000000000000000000
              end
              item
                SizeStyle = ssAbsolute
                Value = 33.000000000000000000
              end
              item
                Value = 50.000000000000000000
              end>
            TabOrder = 0
            object smobtnSaveData: TAdvSmoothSlider
              Left = 90
              Top = 14
              Width = 86
              Height = 33
              Hint = 
                #24403#24744#21551#29992#24433#23376#27169#24335#21518#65292#24744#21448#24819#20445#23384#24433#23376#27169#24335#20013#30340#25968#25454#65288#22914#31995#32479#26356#26032#65289#65292#24744#21487#20197#23558#27492#24320#20851#32622#20026#8220#24320#8221#65292'LCXLShadow'#20250#22312#20851#26426#30340#26102#20505#33258#21160#20445 +
                #23384#24433#23376#25968#25454#12290
              AnimationFactor = 10
              AppearanceOn.Fill.Color = 16575452
              AppearanceOn.Fill.ColorTo = 16571329
              AppearanceOn.Fill.ColorMirror = clNone
              AppearanceOn.Fill.ColorMirrorTo = clNone
              AppearanceOn.Fill.GradientType = gtVertical
              AppearanceOn.Fill.GradientMirrorType = gtVertical
              AppearanceOn.Fill.BorderColor = 13542013
              AppearanceOn.Fill.Rounding = 4
              AppearanceOn.Fill.RoundingType = rtLeft
              AppearanceOn.Fill.ShadowOffset = 0
              AppearanceOn.Fill.Glow = gmNone
              AppearanceOn.FillDisabled.Color = 16765615
              AppearanceOn.FillDisabled.ColorTo = 16765615
              AppearanceOn.FillDisabled.ColorMirror = clNone
              AppearanceOn.FillDisabled.ColorMirrorTo = clNone
              AppearanceOn.FillDisabled.GradientType = gtVertical
              AppearanceOn.FillDisabled.GradientMirrorType = gtSolid
              AppearanceOn.FillDisabled.BorderColor = clNone
              AppearanceOn.FillDisabled.Rounding = 4
              AppearanceOn.FillDisabled.ShadowOffset = 0
              AppearanceOn.FillDisabled.Glow = gmNone
              AppearanceOn.Caption = #24320#21551
              AppearanceOn.Font.Charset = DEFAULT_CHARSET
              AppearanceOn.Font.Color = clWindowText
              AppearanceOn.Font.Height = -11
              AppearanceOn.Font.Name = 'Tahoma'
              AppearanceOn.Font.Style = []
              AppearanceOff.Fill.Color = 16645114
              AppearanceOff.Fill.ColorTo = 16643051
              AppearanceOff.Fill.ColorMirror = clNone
              AppearanceOff.Fill.ColorMirrorTo = clNone
              AppearanceOff.Fill.GradientType = gtVertical
              AppearanceOff.Fill.GradientMirrorType = gtVertical
              AppearanceOff.Fill.BorderColor = 16504504
              AppearanceOff.Fill.Rounding = 4
              AppearanceOff.Fill.RoundingType = rtRight
              AppearanceOff.Fill.ShadowOffset = 0
              AppearanceOff.Fill.Glow = gmNone
              AppearanceOff.FillDisabled.Color = 16765615
              AppearanceOff.FillDisabled.ColorTo = 16765615
              AppearanceOff.FillDisabled.ColorMirror = clNone
              AppearanceOff.FillDisabled.ColorMirrorTo = clNone
              AppearanceOff.FillDisabled.GradientType = gtVertical
              AppearanceOff.FillDisabled.GradientMirrorType = gtSolid
              AppearanceOff.FillDisabled.BorderColor = clNone
              AppearanceOff.FillDisabled.Rounding = 4
              AppearanceOff.FillDisabled.ShadowOffset = 0
              AppearanceOff.FillDisabled.Glow = gmNone
              AppearanceOff.Caption = #20851#38381
              AppearanceOff.Font.Charset = DEFAULT_CHARSET
              AppearanceOff.Font.Color = clWindowText
              AppearanceOff.Font.Height = -11
              AppearanceOff.Font.Name = 'Tahoma'
              AppearanceOff.Font.Style = []
              ButtonAppearance.Fill.Color = 15724527
              ButtonAppearance.Fill.ColorTo = 15724527
              ButtonAppearance.Fill.ColorMirror = clNone
              ButtonAppearance.Fill.ColorMirrorTo = clNone
              ButtonAppearance.Fill.GradientType = gtVertical
              ButtonAppearance.Fill.GradientMirrorType = gtSolid
              ButtonAppearance.Fill.BorderColor = 10786956
              ButtonAppearance.Fill.Rounding = 4
              ButtonAppearance.Fill.ShadowOffset = 0
              ButtonAppearance.Fill.Glow = gmNone
              ButtonAppearance.FillDisabled.Color = 16765615
              ButtonAppearance.FillDisabled.ColorTo = 16765615
              ButtonAppearance.FillDisabled.ColorMirror = clNone
              ButtonAppearance.FillDisabled.ColorMirrorTo = clNone
              ButtonAppearance.FillDisabled.GradientType = gtVertical
              ButtonAppearance.FillDisabled.GradientMirrorType = gtSolid
              ButtonAppearance.FillDisabled.BorderColor = clBlack
              ButtonAppearance.FillDisabled.Rounding = 4
              ButtonAppearance.FillDisabled.ShadowOffset = 0
              ButtonAppearance.FillDisabled.Glow = gmNone
              ValueOn = 1.000000000000000000
              State = ssOff
              Fill.Color = 16765615
              Fill.ColorTo = 16765615
              Fill.ColorMirror = clNone
              Fill.ColorMirrorTo = clNone
              Fill.GradientType = gtVertical
              Fill.GradientMirrorType = gtSolid
              Fill.BorderColor = clNone
              Fill.Rounding = 4
              Fill.ShadowOffset = 0
              Fill.Glow = gmNone
              FillDisabled.Color = 15921906
              FillDisabled.ColorTo = 11974326
              FillDisabled.ColorMirror = clNone
              FillDisabled.ColorMirrorTo = clNone
              FillDisabled.GradientType = gtVertical
              FillDisabled.GradientMirrorType = gtVertical
              FillDisabled.BorderColor = 9841920
              FillDisabled.Rounding = 4
              FillDisabled.ShadowOffset = 0
              FillDisabled.Glow = gmNone
              OnStateChanged = smobtnSaveDataStateChanged
              Align = alClient
              TabOrder = 0
              ParentShowHint = False
              ShowHint = True
            end
          end
        end
      end
      object pnlSet: TRzPanel
        AlignWithMargins = True
        Left = 3
        Top = 248
        Width = 544
        Height = 34
        Align = alBottom
        BorderOuter = fsFlatRounded
        TabOrder = 3
        DesignSize = (
          544
          34)
        object cbbShadowMode: TComboBox
          Left = 16
          Top = 8
          Width = 118
          Height = 20
          Style = csDropDownList
          Ctl3D = False
          ImeName = #20013#25991'('#31616#20307') - '#25628#29399#25340#38899#36755#20837#27861
          ItemIndex = 0
          ParentCtl3D = False
          TabOrder = 0
          Text = #26410#20445#25252
          OnChange = cbbShadowModeChange
          Items.Strings = (
            #26410#20445#25252
            #31995#32479#30424#20445#25252
            #20840#30424#20445#25252
            #33258#23450#20041#20445#25252)
        end
        object btnCurShadowMode: TButton
          Left = 140
          Top = 8
          Width = 104
          Height = 21
          Action = actCurShadowMode2
          TabOrder = 1
        end
        object btnNextShadowMode: TButton
          Left = 428
          Top = 8
          Width = 104
          Height = 21
          Action = actNextShaodowMode
          Anchors = [akTop, akRight]
          TabOrder = 2
        end
      end
    end
    object tsSetting: TRzTabSheet
      OnShow = tsSettingShow
      Caption = #35774#32622
      DesignSize = (
        550
        376)
      object lblPassword: TRzLabel
        Left = 32
        Top = 32
        Width = 336
        Height = 12
        Anchors = [akLeft, akTop, akRight]
        Caption = #20026#20102#36991#20813#20854#20182#29992#25143#30340#24847#22806#25805#20316#65292#24744#21487#20197#35774#32622#26412#36719#20214#30340#21551#21160#23494#30721#12290
        TextStyle = tsRaised
      end
      object lblReleasePassword: TRzLabel
        Left = 32
        Top = 71
        Width = 300
        Height = 12
        Anchors = [akLeft, akTop, akRight]
        Caption = #24403#24744#25191#34892#23436#31649#29702#25805#20316#65292#21487#20197#28857#20987#8220#37322#25918#31649#29702#26435#38480#8221#25353#38062#12290
        TextStyle = tsRaised
      end
      object btnPassword: TRzButton
        Left = 412
        Top = 26
        Width = 107
        Action = actPassword
        Anchors = [akTop, akRight]
        TabOrder = 0
      end
      object chkAutorun: TRzCheckBox
        Left = 32
        Top = 104
        Width = 201
        Height = 15
        Caption = #24320#26426#33258#21160#21551#21160
        FrameController = frmctrlMain
        HotTrack = True
        State = cbUnchecked
        TabOrder = 1
        OnClick = chkAutorunClick
      end
      object btnReleasePassword: TRzButton
        Left = 412
        Top = 65
        Width = 107
        Action = actReleasePassword
        Anchors = [akTop, akRight]
        TabOrder = 2
      end
      object chkEnableSaveData: TCheckBox
        Left = 32
        Top = 136
        Width = 336
        Height = 17
        Caption = #24320#21551#24433#23376#25345#20037#21270#36873#39033#65288#23454#39564#24615#65289
        Enabled = False
        TabOrder = 3
      end
    end
    object tsAbout: TRzTabSheet
      Caption = #20851#20110
      ExplicitLeft = 0
      ExplicitTop = 0
      ExplicitWidth = 0
      ExplicitHeight = 0
      object verinfostCopyright: TRzVersionInfoStatus
        Left = 40
        Top = 72
        Width = 116
        FillColor = clWindow
        FlatColorAdjustment = 0
        FrameController = frmctrlMain
        ParentFillColor = False
        Transparent = False
        Color = clBtnFace
        ParentColor = False
        FieldLabel = #29256#26435#65306
        AutoSize = True
        Field = vifCopyright
        VersionInfo = verinfoMain
      end
      object verinfostCompanyName: TRzVersionInfoStatus
        Left = 40
        Top = 38
        Width = 128
        FillColor = clWindow
        FlatColorAdjustment = 0
        FrameController = frmctrlMain
        ParentFillColor = False
        Transparent = False
        Color = clBtnFace
        ParentColor = False
        FieldLabel = #20316#32773#65306
        AutoSize = True
        Field = vifCompanyName
        VersionInfo = verinfoMain
      end
      object lblInfo: TRzLabel
        Left = 40
        Top = 182
        Width = 252
        Height = 12
        Caption = #22914#26524#26377#20219#20309#24847#35265#21644#24314#35758#65292#35831#21457#37038#20214#32473#25105#65292#35874#35874#65281
        Transparent = True
      end
      object lnklblEmail: TLinkLabel
        Left = 40
        Top = 212
        Width = 160
        Height = 16
        Caption = 
          'Email'#65306'<a href="mailto:lcx87654321@163.com">lcx87654321@163.com</' +
          'a>'
        TabOrder = 0
        OnLinkClick = lnklblEmailLinkClick
      end
    end
  end
  object sbMain: TRzStatusBar
    Left = 0
    Top = 384
    Width = 608
    Height = 19
    BorderInner = fsNone
    BorderOuter = fsNone
    BorderSides = [sdLeft, sdTop, sdRight, sdBottom]
    BorderWidth = 0
    Padding.Top = 1
    TabOrder = 1
    object verinfostProduct: TRzVersionInfoStatus
      Left = 0
      Top = 1
      Width = 170
      Height = 18
      FillColor = clWindow
      FlatColorAdjustment = 0
      FrameController = frmctrlMain
      ParentFillColor = False
      Transparent = False
      Align = alLeft
      Color = clBtnFace
      ParentColor = False
      FieldLabel = #20135#21697#29256#26412#65306
      AutoSize = True
      Field = vifProductVersion
      VersionInfo = verinfoMain
      ExplicitTop = 0
      ExplicitHeight = 20
    end
    object verinfostFile: TRzVersionInfoStatus
      Left = 170
      Top = 1
      Width = 152
      Height = 18
      FillColor = clWindow
      FlatColorAdjustment = 0
      FrameController = frmctrlMain
      ParentFillColor = False
      Transparent = False
      Align = alLeft
      Color = clBtnFace
      ParentColor = False
      FieldLabel = #25991#20214#29256#26412#65306
      AutoSize = True
      Field = vifFileVersion
      VersionInfo = verinfoMain
      ExplicitTop = 0
      ExplicitHeight = 20
    end
  end
  object frmctrlMain: TRzFrameController
    FrameVisible = True
    Left = 416
    Top = 176
  end
  object grpctrlMain: TRzGroupController
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    Left = 448
    Top = 176
  end
  object mnuctrlMain: TRzMenuController
    Left = 480
    Top = 176
  end
  object verinfoMain: TRzVersionInfo
    Left = 544
    Top = 136
  end
  object trycnMain: TTrayIcon
    PopupMenu = pmTray
    OnBalloonClick = trycnMainClick
    OnClick = trycnMainClick
    Left = 312
    Top = 176
  end
  object pmTray: TPopupMenu
    OwnerDraw = True
    OnPopup = pmTrayPopup
    Left = 344
    Top = 176
    object mniStartShadowMode: TMenuItem
      Action = actStartShadowMode
    end
    object mniN2: TMenuItem
      Caption = '-'
    end
    object mniReleasePassword: TMenuItem
      Action = actReleasePassword
    end
    object mniN1: TMenuItem
      Caption = '-'
    end
    object mniShutReset: TMenuItem
      Caption = #20851#26426'/'#37325#21551'(&S)'
      OnClick = mniShutResetClick
    end
    object mniExit: TMenuItem
      Caption = #36864#20986'(&E)'
      OnClick = mniExitClick
    end
  end
  object reginiMain: TRzRegIniFile
    FileEncoding = feUnicode
    Path = 'LCXLShadow.ini'
    Left = 544
    Top = 176
  end
  object actmgrMain: TActionManager
    Left = 480
    Top = 136
    StyleName = 'Platform Default'
    object actPassword: TAction
      Caption = #35774#32622#23494#30721
      OnExecute = actPasswordExecute
    end
    object actStartShadowMode: TAction
      Caption = #21551#21160#24433#23376#27169#24335'(&S)'
      OnExecute = actStartShadowModeExecute
    end
    object actReleasePassword: TAction
      Caption = #37322#25918#31649#29702#26435#38480'(&R)'
      OnExecute = actReleasePasswordExecute
    end
    object actNextShaodowMode: TAction
      Caption = #19979#27425#21551#21160#27169#24335'(&N)'
      OnExecute = actNextShaodowModeExecute
    end
    object actCurShadowMode2: TAction
      Caption = #24320#22987#20445#25252'(&S)'
      OnExecute = actCurShadowMode2Execute
    end
  end
  object advfchntMain: TAdvOfficeHint
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Segoe UI'
    Font.Style = []
    HintHelpText = #25353'F1'#33719#21462#26356#22810#24110#21161
    Version = '1.5.3.0'
    Left = 272
    Top = 176
  end
  object ilListState: TImageList
    Left = 448
    Top = 136
    Bitmap = {
      494C010102000800980010001000FFFFFFFFFF10FFFFFFFFFFFFFFFF424D3600
      0000000000003600000028000000400000001000000001002000000000000010
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000009999
      9900999999009999990099999900999999009999990099999900999999009999
      9900999999000000000000000000000000000000000000000000000000000099
      0000009900000099000000990000009900000099000000990000009900000099
      0000009900000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000099999900B2B2
      B200B2B2B200B2B2B200B2B2B200B2B2B20099999900B2B2B200999999009999
      99009999990099999900000000000000000000000000000000000099000000CC
      000000CC000000CC000000CC000000CC00000099000000CC0000009900000099
      0000009900000099000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000099999900CCCC
      CC00B2B2B200B2B2B200B2B2B200B2B2B200B2B2B20099999900B2B2B2009999
      99009999990099999900000000000000000000000000000000000099000000FF
      000000CC000000CC000000CC000000CC000000CC00000099000000CC00000099
      0000009900000099000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000099999900B2B2
      B200CCCCCC00B2B2B200B2B2B200B2B2B200B2B2B200B2B2B20099999900B2B2
      B2009999990099999900000000000000000000000000000000000099000000CC
      000000FF000000CC000000CC000000CC000000CC000000CC00000099000000CC
      0000009900000099000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000099999900CCCC
      CC00B2B2B200CCCCCC00B2B2B200B2B2B200B2B2B200B2B2B200B2B2B2009999
      9900B2B2B20099999900000000000000000000000000000000000099000000FF
      000000CC000000FF000000CC000000CC000000CC000000CC000000CC00000099
      000000CC00000099000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000099999900CCCC
      CC00CCCCCC00B2B2B200CCCCCC00B2B2B200B2B2B200B2B2B200B2B2B200B2B2
      B2009999990099999900000000000000000000000000000000000099000000FF
      000000FF000000CC000000FF000000CC000000CC000000CC000000CC000000CC
      0000009900000099000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000099999900E5E5
      E500CCCCCC00CCCCCC00B2B2B200CCCCCC00B2B2B200B2B2B200B2B2B200B2B2
      B200B2B2B20099999900000000000000000000000000000000000099000099FF
      990000FF000000FF000000CC000000FF000000CC000000CC000000CC000000CC
      000000CC00000099000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000000000000000000099999900E5E5
      E500E5E5E500CCCCCC00CCCCCC00B2B2B200CCCCCC00B2B2B200B2B2B200B2B2
      B200B2B2B20099999900000000000000000000000000000000000099000099FF
      990099FF990000FF000000FF000000CC000000FF000000CC000000CC000000CC
      000000CC00000099000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000009999
      9900999999009999990099999900999999009999990099999900999999009999
      9900999999000000000000000000000000000000000000000000000000000099
      0000009900000099000000990000009900000099000000990000009900000099
      0000009900000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      000000000000000000000000000000000000424D3E000000000000003E000000
      2800000040000000100000000100010000000000800000000000000000000000
      000000000000000000000000FFFFFF00FFFFFFFF00000000FFFFFFFF00000000
      FFFFFFFF00000000FFFFFFFF00000000E007E00700000000C003C00300000000
      C003C00300000000C003C00300000000C003C00300000000C003C00300000000
      C003C00300000000C003C00300000000E007E00700000000FFFFFFFF00000000
      FFFFFFFF00000000FFFFFFFF0000000000000000000000000000000000000000
      000000000000}
  end
end
