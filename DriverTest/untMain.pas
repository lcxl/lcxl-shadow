unit untMain;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ComCtrls, WinSvc, ExtCtrls,
  untFunc;

type
  TfrmMain = class(TForm)
    lblFilePath: TLabel;
    edtFilePath: TEdit;
    lblStartType: TLabel;
    cbbStartType: TComboBox;
    btnOpenFile: TButton;
    lblSerName: TLabel;
    edtSerName: TEdit;
    btnCreateSer: TButton;
    btnStartSer: TButton;
    btnStopSer: TButton;
    btnDeleteSer: TButton;
    dlgOpenFile: TOpenDialog;
    statMain: TStatusBar;
    grpFilterOpt: TGroupBox;
    chkDisk: TCheckBox;
    chkDiskVol: TCheckBox;
    procedure btnOpenFileClick(Sender: TObject);
    procedure btnCreateSerClick(Sender: TObject);
    procedure btnDeleteSerClick(Sender: TObject);
    procedure btnStartSerClick(Sender: TObject);
    procedure btnStopSerClick(Sender: TObject);
    procedure chkDiskClick(Sender: TObject);
    procedure chkDiskVolClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  frmMain: TfrmMain;

implementation

{$R *.dfm}

procedure TfrmMain.btnCreateSerClick(Sender: TObject);
var
  hSCMgr: THandle;
  hSer: THandle;
  DriPath: string;
begin
  hSCMgr := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS );
  if (hSCMgr<> 0) then
  begin
    if chkDisk.Checked or chkDiskVol.Checked then
    begin
      DriPath := GetEnvironmentVariable('windir')+'\System32\Drivers\'+
        ExtractFileName(edtFilePath.Text);
      if CompareText(DriPath, edtFilePath.Text) <> 0 then
      begin
        DeleteFile(DriPath);
        if not CopyFile(PChar(edtFilePath.Text), PChar(DriPath), True) then
        begin
          statMain.SimpleText := 'CopyFile:'+SysErrorMessage(GetLastError());
          Exit;
        end;
      end;
      edtFilePath.Text := DriPath;
    end;
    hSer := CreateService(
      hSCMgr,
      PChar(edtSerName.Text),
      PChar(edtSerName.Text),
      SC_MANAGER_ALL_ACCESS,
      SERVICE_KERNEL_DRIVER,
      cbbStartType.ItemIndex,
      SERVICE_ERROR_CRITICAL,
      PChar(edtFilePath.Text),
      nil,
      nil,
      nil,
      nil,
      nil);
    if hSer <> 0 then
    begin
      if chkDisk.Checked then
      begin
        SetAsDiskFilter(edtSerName.Text);
      end;
      if chkDiskVol.Checked then
      begin
        SetAsDiskVolFilter(edtSerName.Text);
      end;
      statMain.SimpleText := 'CreateService:'+SysErrorMessage(GetLastError());
      CloseServiceHandle(hSer);
    end
    else
    begin
      statMain.SimpleText := 'CreateService:'+SysErrorMessage(GetLastError());
    end;
    CloseServiceHandle(hSCMgr);
  end
  else
  begin
    statMain.SimpleText := 'OpenSCManager'+SysErrorMessage(GetLastError());
  end;
end;

procedure TfrmMain.btnDeleteSerClick(Sender: TObject);
var
  hSCMgr: THandle;
  hSer: THandle;
begin
   hSCMgr := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS );
  if (hSCMgr<> 0) then
  begin

    hSer := OpenService(
      hSCMgr,
      PChar(edtSerName.Text),
      SERVICE_ALL_ACCESS);
    if hSer <> 0 then
    begin
      if DeleteService(hSer) then
      begin
        statMain.SimpleText := SysErrorMessage(GetLastError());
      end
      else
      begin
        statMain.SimpleText := SysErrorMessage(GetLastError());
      end;
      CloseServiceHandle(hSer);
    end
    else
    begin
      statMain.SimpleText := SysErrorMessage(GetLastError());
    end;
    CloseServiceHandle(hSCMgr);
  end
  else
  begin
    statMain.SimpleText := SysErrorMessage(GetLastError());
  end;
end;

procedure TfrmMain.btnOpenFileClick(Sender: TObject);
begin
  if dlgOpenFile.Execute(Handle) then
  begin
    edtFilePath.Text := dlgOpenFile.FileName;
    edtSerName.Text := ExtractFileName(edtFilePath.Text);
    edtSerName.Text := Copy(edtSerName.Text, 1, Pos('.', edtSerName.Text)-1);
  end;
end;

procedure TfrmMain.btnStartSerClick(Sender: TObject);
var
  hSCMgr: THandle;
  hSer: THandle;
  pArgc: PChar;
begin
  hSCMgr := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS );
  if (hSCMgr<> 0) then
  begin

    hSer := OpenService(
      hSCMgr,
      PChar(edtSerName.Text),
      SERVICE_ALL_ACCESS);
    if hSer <> 0 then
    begin
      pArgc := nil;
      if (StartService(hSer, 0, pArgc)) then
      begin
        statMain.SimpleText := SysErrorMessage(GetLastError());
      end
      else
      begin
        statMain.SimpleText := SysErrorMessage(GetLastError());
      end;
      CloseServiceHandle(hSer);
    end
    else
    begin
      statMain.SimpleText := SysErrorMessage(GetLastError());
    end;
    CloseServiceHandle(hSCMgr);
  end
  else
  begin
    statMain.SimpleText := SysErrorMessage(GetLastError());
  end;
end;

procedure TfrmMain.btnStopSerClick(Sender: TObject);
var
  hSCMgr: THandle;
  hSer: THandle;
  SerStatus: TServiceStatus;
begin
  hSCMgr := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS );
  if (hSCMgr<> 0) then
  begin

    hSer := OpenService(
      hSCMgr,
      PChar(edtSerName.Text),
      SERVICE_ALL_ACCESS);
    if hSer <> 0 then
    begin
      if (ControlService(hSer, SERVICE_CONTROL_STOP, SerStatus)) then
      begin
        statMain.SimpleText := SysErrorMessage(GetLastError());
      end
      else
      begin
        statMain.SimpleText := SysErrorMessage(GetLastError());
      end;
      CloseServiceHandle(hSer);
    end;
    CloseServiceHandle(hSCMgr);
  end;
end;

procedure TfrmMain.chkDiskClick(Sender: TObject);
begin
  if chkDisk.Checked then
  begin
    cbbStartType.ItemIndex := 0;
  end;
end;

procedure TfrmMain.chkDiskVolClick(Sender: TObject);
begin
  if chkDiskVol.Checked then
  begin
    cbbStartType.ItemIndex := 0;
  end;
end;

end.
