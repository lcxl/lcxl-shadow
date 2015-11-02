unit untSystemShutdown;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, RzButton, StdCtrls, RzLabel, RzPanel, ExtCtrls, pngimage,

  uBitmapUtils, untCommDataType, untSetShadowMode, untResString, untSerInterface;
type
  TfrmSystemShutdown = class(TForm)
    gpnl1: TRzGridPanel;
    pnl: TRzPanel;
    lblLCXLShadow: TRzLabel;
    tmrTrue: TTimer;
    imgBackgrand: TImage;
    gpnlbtns: TRzGridPanel;
    btnShutdown: TRzButton;
    btnReset: TRzButton;
    btnCancel: TRzButton;
    btnShadowSetting: TButton;
    procedure FormCreate(Sender: TObject);
    procedure btnShutdownClick(Sender: TObject);
    procedure btnResetClick(Sender: TObject);
    procedure btnCancelClick(Sender: TObject);
    procedure tmrTrueTimer(Sender: TObject);
    procedure btnShadowSettingClick(Sender: TObject);
  private
    { Private declarations }
    FSSType: Byte;
    procedure ShowBackground();
    procedure UpdateShadowMode(ShadowMode: LCXL_SHADOW_MODE);
    procedure WMDisplayChange(var TheMsg:TMessage); message WM_DISPLAYCHANGE;
  public
    { Public declarations }
  end;

var
  frmSystemShutdown: TfrmSystemShutdown;

function ShowSystemShutdownBox(AOwner: TComponent): Byte;

implementation
uses
  untMain;
{$R *.dfm}

function ShowSystemShutdownBox(AOwner: TComponent): Byte;
begin
  frmSystemShutdown := TfrmSystemShutdown.Create(AOwner);
  frmSystemShutdown.ShowModal;
  result := frmSystemShutdown.FSSType;
  frmSystemShutdown.Free;
end;

procedure TfrmSystemShutdown.btnCancelClick(Sender: TObject);
begin
  Close;
end;

procedure TfrmSystemShutdown.btnResetClick(Sender: TObject);
begin
  FSSType := SS_REBOOT;
  Close;
end;

procedure TfrmSystemShutdown.btnShadowSettingClick(Sender: TObject);
var
  ShadowMode: LCXL_SHADOW_MODE;
begin
  ShadowMode := frmMain.FDriverSetting.NextShadowMode;

  if ShowShadowModeBox(Self, ShadowMode) then
  begin
    frmMain.SetNextRebootShadowMode(ShadowMode);

      UpdateShadowMode(ShadowMode);
  end;
end;

procedure TfrmSystemShutdown.btnShutdownClick(Sender: TObject);
begin
  FSSType := SS_SHUTDOWN;
  Close;

end;

procedure TfrmSystemShutdown.FormCreate(Sender: TObject);
begin
  FSSType := SS_NONE;
  ShowBackground;

  UpdateShadowMode(frmMain.FDriverSetting.NextShadowMode);

  Left := 0;
  Top := 0;
end;

procedure TfrmSystemShutdown.UpdateShadowMode(ShadowMode: LCXL_SHADOW_MODE);
var
  ShadowModeStr: string;
begin

    case ShadowMode of
    SM_NONE:
      begin
        ShadowModeStr := RS_SHADOW_MODE_NONE;
      end;
      SM_SYSTEM:
      begin
        ShadowModeStr := RS_SHADOW_MODE_SYSTEM;
      end;
      SM_ALL:
      begin
        ShadowModeStr := RS_SHADOW_MODE_ALL;
      end;
      SM_CUSTOM:
      begin
        ShadowModeStr := RS_SHADOW_MODE_CUSTOM;
      end;
    else
      OutputDebugString('FDriverSetting.NextShadowMode else'#10);
      ShadowModeStr := RS_SHADOW_MODE_NONE;
    end;

  btnShadowSetting.Caption := Format('%s: %s(%s)', [RS_NEXT_REBOOT_SHADOW_MODE, ShadowModeStr, RS_CLICK_TO_CHANGE]);
end;

procedure TfrmSystemShutdown.ShowBackground;
var
  Bitmap:TBitmap;
  FDC: HDC;
begin
  Width := Screen.Width;
  Height := Screen.Height;

  Bitmap:=TBitmap.Create;
  Bitmap.PixelFormat := pf24bit;
  Bitmap.Width := Screen.Width;
  Bitmap.Height := Screen.Height;
  FDC := GetDC(0);
  BitBlt(Bitmap.Canvas.Handle, 0, 0, Bitmap.Width, Bitmap.Height, FDC, 0,
    0, SRCCOPY);
  ReleaseDc(0, FDC);
  ToGray(Bitmap);
  imgBackgrand.Picture.Bitmap := Bitmap;
  Bitmap.Free;
end;

procedure TfrmSystemShutdown.tmrTrueTimer(Sender: TObject);
begin
  //if GetForegroundWindow() <> Handle  then
  //begin
    //SetForegroundWindow(Handle);
    //SetActiveWindow(Handle);
  //end;
end;

procedure TfrmSystemShutdown.WMDisplayChange(var TheMsg: TMessage);
begin
  Hide;
  Sleep(500);
  ShowBackground;
  Show;
end;

end.
