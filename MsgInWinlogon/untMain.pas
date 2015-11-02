unit untMain;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, ComCtrls, ExtCtrls, StdCtrls;

type
  TfrmMain = class(TForm)
    tmrTest: TTimer;
    pbSaveData: TProgressBar;
    lblMsg: TLabel;
    procedure tmrTestTimer(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  frmMain: TfrmMain;

implementation

{$R *.dfm}

procedure TfrmMain.tmrTestTimer(Sender: TObject);
begin
  if pbSaveData.Position = pbSaveData.Max then
  begin
    pbSaveData.Position := pbSaveData.Min;
  end
  else
  begin
    pbSaveData.Position := pbSaveData.Position+1;
  end;
end;

end.
