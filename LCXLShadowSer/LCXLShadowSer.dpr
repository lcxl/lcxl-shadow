program LCXLShadowSer;

//{$APPTYPE CONSOLE}

{$R *.res}

{$R 'DriverIcon.res' 'DriverIcon.rc'}

uses
  Windows,
  untSerCore in 'untSerCore.pas',
  untSerInterface in '..\untSerInterface.pas',
  ElAES in '..\ElAES.pas',
  untDllInterface in '..\untDllInterface.pas',
  untDllInterfaceEx in '..\untDllInterfaceEx.pas',
  untCommFunc in '..\untCommFunc.pas',
  untSockMgr in 'untSockMgr.pas',
  untCommDataType in '..\untCommDataType.pas';

begin
  SetErrorMode(SEM_FAILCRITICALERRORS);//使程序出现异常时不报错
  //Setup service table which define all services in this process
  SerCore := TSerCore.Create;
  SerCore.Run();
  SerCore.Free;
end.
