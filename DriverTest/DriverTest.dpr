program DriverTest;

uses
  Forms,
  untMain in 'untMain.pas' {frmMain},
  untFunc in 'untFunc.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TfrmMain, frmMain);
  Application.Run;
end.
