program LCXLMsg;

uses
  Forms,
  untMain in 'untMain.pas' {frmMain},
  ElAES in '..\ElAES.pas',
  untDllInterface in '..\untDllInterface.pas',
  untDllInterfaceEx in '..\untDllInterfaceEx.pas',
  untCommFunc in '..\untCommFunc.pas',
  untCommDataType in '..\untCommDataType.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TfrmMain, frmMain);
  Application.Run;
end.
