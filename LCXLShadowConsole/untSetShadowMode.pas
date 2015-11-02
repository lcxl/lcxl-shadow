unit untSetShadowMode;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, ExtCtrls, DateUtils, RzPanel, RzRadGrp, RzButton,

  untCommDataType, untVerifyPassword;

type
  TfrmSetShadowMode = class(TForm)
    rgpShadowMode: TRzRadioGroup;
    pnlOKCancel: TRzPanel;
    btnOK: TRzButton;
    btnCancel: TRzButton;
    chgCustomDiskList: TRzCheckGroup;
    procedure rgpShadowModeChanging(Sender: TObject; NewIndex: Integer;
      var AllowChange: Boolean);
    procedure FormCreate(Sender: TObject);
    procedure btnOKClick(Sender: TObject);
    procedure btnCancelClick(Sender: TObject);
  private
    { Private declarations }
    FShadowMode: LCXL_SHADOW_MODE;
    FCustomHeight: Integer;
    FIsOK: Boolean;
    procedure ShowHideCustom(IsShow: Boolean; ShowAnimation: Boolean = True);
  public
    { Public declarations }
  end;

var
  frmSetShadowMode: TfrmSetShadowMode;

//显示影子模式对话框
function ShowShadowModeBox(
  AOwner: TWinControl;
  var ShadowMode: LCXL_SHADOW_MODE
  ): Boolean;

implementation
uses
  untMain;
{$R *.dfm}

function ShowShadowModeBox(
  AOwner: TWinControl;
  var ShadowMode: LCXL_SHADOW_MODE
  ): Boolean;
var
  I: Integer;
  j: Integer;

  ItemIndex: Integer;

  DriverDiskList: array of TVolumeDiskInfoEx;
  DiskListLen: DWORD;

  CustomDiskList: array of TDeviceNameType;
  CustomDiskListLen: DWORD;
  SysDir: string;
  SysDirLen: DWORD;
begin

  SetLength(SysDir, MAX_PATH);
  SysDirLen := GetSystemDirectory(PChar(SysDir), MAX_PATH);
  SetLength(SysDir, SysDirLen);

  CustomDiskListLen := Length(frmMain.FCustomDiskList);
  SetLength(CustomDiskList, CustomDiskListLen);
  CopyMemory(@CustomDiskList[0], @frmMain.FCustomDiskList[0], SizeOf(TDeviceNameType)*CustomDiskListLen);

  DiskListLen := Length(frmMain.FDiskList);
  SetLength(DriverDiskList, DiskListLen);
  CopyMemory(@DriverDiskList[0], @frmMain.FDiskList[0], SizeOf(TVolumeDiskInfoEx)*DiskListLen);

  frmSetShadowMode := TfrmSetShadowMode.Create(AOwner);
  for I := 0 to DiskListLen-1 do
  begin
    ItemIndex := frmSetShadowMode.chgCustomDiskList.Items.Add(DriverDiskList[i].DiskInfo.DosName);
    //如果是系统盘
    if UpCase(DriverDiskList[i].DiskInfo.DosName[1]) = UpCase(SysDir[1]) then
    begin
      frmSetShadowMode.chgCustomDiskList.ItemChecked[ItemIndex] := True;
      frmSetShadowMode.chgCustomDiskList.ItemEnabled[ItemIndex] := False;
    end
    else
    begin
      for J := 0 to CustomDiskListLen-1 do
      begin
        if UpperCase(string(DriverDiskList[i].SysVolInfo.VolumeName)) = UpperCase(string(CustomDiskList[i])) then
        begin
          frmSetShadowMode.chgCustomDiskList.ItemChecked[ItemIndex] := True;
          Break;
        end;
      end;
    end;
  end;
  frmSetShadowMode.rgpShadowMode.ItemIndex := Integer(ShadowMode);
  frmSetShadowMode.ShowModal;
  Result := frmSetShadowMode.FIsOK;
  if Result then
  begin
    ShadowMode := LCXL_SHADOW_MODE(frmSetShadowMode.rgpShadowMode.ItemIndex);
    //如果是自定义
    if ShadowMode = SM_CUSTOM then
    begin

      CustomDiskListLen := 0;
      SetLength(CustomDiskList, CustomDiskListLen);
      for I := 0 to frmSetShadowMode.chgCustomDiskList.Items.Count-1 do
      begin
        if frmSetShadowMode.chgCustomDiskList.ItemChecked[I] then
        begin
          CustomDiskListLen := Length(CustomDiskList);

          SetLength(CustomDiskList, CustomDiskListLen+1);
          StrLCopy(CustomDiskList[CustomDiskListLen], DriverDiskList[I].SysVolInfo.VolumeName, MAX_DEVNAME_LENGTH);
        end;
      end;
      frmMain.SetCustomDiskList(@CustomDiskList[0], Length(CustomDiskList));
    end;
  end;
  frmSetShadowMode.Free;
end;

procedure TfrmSetShadowMode.btnCancelClick(Sender: TObject);
begin
  Close;
end;

procedure TfrmSetShadowMode.btnOKClick(Sender: TObject);
begin
  FIsOK := True;
  Close;
end;

procedure TfrmSetShadowMode.FormCreate(Sender: TObject);
begin
  FCustomHeight := chgCustomDiskList.Height;
  FIsOK := False;
end;

procedure TfrmSetShadowMode.rgpShadowModeChanging(Sender: TObject;
  NewIndex: Integer; var AllowChange: Boolean);
begin
  case NewIndex of
    0,1,2://未保护\系统盘保护\全盘保护
    begin
      ShowHideCustom(False);
      FShadowMode := LCXL_SHADOW_MODE(NewIndex);
    end;
    3://自定义保护
    begin
      ShowHideCustom(True);
      FShadowMode := LCXL_SHADOW_MODE(NewIndex);
    end;
  end;

end;

procedure TfrmSetShadowMode.ShowHideCustom(IsShow: Boolean; ShowAnimation: Boolean);
var
  PreTime: TDateTime;
  CurTime: TDateTime;
  I: Integer;
begin
  PreTime := Now;
  if IsShow then
  begin
    if ShowAnimation then
    begin
      while chgCustomDiskList.Height < FCustomHeight do
      begin
        CurTime := Now;
        I := Round(MilliSecondsBetween(CurTime, PreTime)*0.6);
        PreTime := CurTime;

        if FCustomHeight - chgCustomDiskList.Height>=I then
        begin
          chgCustomDiskList.Height := chgCustomDiskList.Height+I;
        end
        else
        begin
          Break;
        end;
        AutoSize := True;
        AutoSize := False;
        Refresh;
      end;
    end;
    chgCustomDiskList.Height := FCustomHeight;
    AutoSize := True;
    AutoSize := False;
    Refresh;
  end
  else
  begin
    if ShowAnimation then
    begin
      while chgCustomDiskList.Height > 0 do
      begin
        CurTime := Now;
        I := Round(MilliSecondsBetween(CurTime, PreTime)*0.6);
        PreTime := CurTime;
        if chgCustomDiskList.Height>=I then
        begin
          chgCustomDiskList.Height := chgCustomDiskList.Height - I;
        end
        else
        begin
          Break;
        end;
        AutoSize := True;
        AutoSize := False;
        Refresh;
      end;
    end;
    chgCustomDiskList.Height := 0;
    AutoSize := True;
    AutoSize := False;
    Refresh;
  end;
end;

end.
