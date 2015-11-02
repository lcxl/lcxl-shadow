program LCXLShadowConsole;

uses
  Windows,
  SysUtils,
  Forms,
  untMain in 'untMain.pas' {frmMain},
  untResString in 'untResString.pas',
  untSetShadowMode in 'untSetShadowMode.pas' {frmSetShadowMode},
  untSystemShutdown in 'untSystemShutdown.pas' {frmSystemShutdown},
  uBitmapUtils in 'uBitmapUtils.pas',
  untSetPassword in 'untSetPassword.pas' {frmSetPassword},
  untVerifyPassword in 'untVerifyPassword.pas' {frmVerifyPassword},
  untShadowing in 'untShadowing.pas' {frmShadowing},
  ElAES in '..\ElAES.pas',
  untCommFunc in '..\untCommFunc.pas',
  untSerInterface in '..\untSerInterface.pas',
  untClientSockMgr in 'untClientSockMgr.pas',
  untCommDataType in '..\untCommDataType.pas';

{$R *.res}
var
  mymutex: THandle;
begin
  MyMsg := RegisterWindowMessage('LCXLShadow_Msg');
  mymutex := CreateMutex(nil, True, 'LCXLShadow_App');
  if GetLastError=ERROR_ALREADY_EXISTS then
  begin
    if not PostMessage(HWND_BROADCAST, MyMsg, 0, 0) then
    begin
      MessageBox(0, PChar(RS_CONSOLE_IS_RUNNING), nil, MB_ICONINFORMATION);
    end;
    if mymutex <> 0 then
    begin
      CloseHandle(mymutex);
    end;
    Exit;
  end;
  //如果找到Install参数
  if FindCmdLineSwitch('Install') then
  begin
    OutputDebugStr('App:Install模式');
    SetAutoRun(True);
    Exit;
  end
  else
  if FindCmdLineSwitch('Uninstall') then
  begin
    //卸载
     OutputDebugStr('App:Uninstall模式');
    SetAutoRun(False);
    Exit;
  end;

  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.Title := 'LCXLShadow 影子系统控制端';
  Application.CreateForm(TfrmMain, frmMain);
  Application.Run;
  if mymutex <> 0 then
  begin
    CloseHandle(mymutex);
  end;
end.
