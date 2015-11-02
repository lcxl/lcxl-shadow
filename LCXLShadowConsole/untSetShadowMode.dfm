object frmSetShadowMode: TfrmSetShadowMode
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  BorderStyle = bsSingle
  Caption = #35774#32622#24433#23376#27169#24335
  ClientHeight = 289
  ClientWidth = 350
  Color = clBtnFace
  DoubleBuffered = True
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poDesktopCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object rgpShadowMode: TRzRadioGroup
    AlignWithMargins = True
    Left = 3
    Top = 3
    Width = 344
    Height = 86
    Align = alTop
    Caption = #24433#23376#27169#24335
    Columns = 2
    GroupStyle = gsBanner
    ItemFrameColor = 8409372
    ItemHotTrack = True
    ItemHighlightColor = 2203937
    ItemHeight = 25
    ItemIndex = 3
    Items.Strings = (
      #19981#20445#25252
      #20840#30424#20445#25252
      #31995#32479#30424#20445#25252
      #33258#23450#20041#20445#25252)
    LightTextStyle = True
    SpaceEvenly = True
    StartXPos = 30
    StartYPos = 10
    TabOrder = 0
    OnChanging = rgpShadowModeChanging
  end
  object pnlOKCancel: TRzPanel
    Left = 0
    Top = 239
    Width = 350
    Height = 55
    Align = alTop
    BorderOuter = fsFlat
    BorderSides = [sdTop]
    TabOrder = 1
    DesignSize = (
      350
      55)
    object btnOK: TRzButton
      Left = 166
      Top = 16
      Default = True
      Anchors = [akTop, akRight]
      Caption = #30830#23450'(&O)'
      TabOrder = 0
      OnClick = btnOKClick
    end
    object btnCancel: TRzButton
      Left = 254
      Top = 16
      Anchors = [akTop, akRight]
      Caption = #21462#28040'(&C)'
      TabOrder = 1
      OnClick = btnCancelClick
    end
  end
  object chgCustomDiskList: TRzCheckGroup
    AlignWithMargins = True
    Left = 3
    Top = 95
    Width = 344
    Height = 141
    Align = alTop
    Caption = #33258#23450#20041#20445#25252
    Columns = 5
    GroupStyle = gsBanner
    ItemFrameColor = 8409372
    ItemHighlightColor = 2203937
    ItemHotTrack = True
    LightTextStyle = True
    SpaceEvenly = True
    TabOnEnter = True
    TabOrder = 2
  end
end
