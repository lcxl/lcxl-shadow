unit untVerifyPassword;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics,
  Controls, Forms, Dialogs, StdCtrls, Mask, RzEdit,
  RzButton, RzLabel, hash;

type
  TfrmVerifyPassword = class(TForm)
    edtPassword: TRzEdit;
    lblPassword: TRzLabel;
    btnOK: TRzButton;
    btnCancel: TRzButton;
    procedure btnOKClick(Sender: TObject);
    procedure btnCancelClick(Sender: TObject);
  private
    { Private declarations }
    FIsOK: Boolean;
  public
    { Public declarations }
    property IsOK: Boolean read FIsOK;
  end;

var
  frmVerifyPassword: TfrmVerifyPassword;

function VerifyPasswordByMsgBox(AOwner: TWinControl): Boolean;

implementation
uses
 untMain;
{$R *.dfm}


function VerifyPasswordByMsgBox(AOwner: TWinControl): Boolean;
var
  PassMD5: string;
begin
  Result := False;

    frmVerifyPassword := TfrmVerifyPassword.Create(AOwner);
    frmVerifyPassword.ShowModal;
    if frmVerifyPassword.IsOK then
    begin

      PassMD5 := frmVerifyPassword.edtPassword.Text;
      if PassMD5 <> '' then
      begin
        PassMD5 := MD5Print(MD5string(PassMD5));
      end;
      Result := frmMain.VerifyPassword(PassMD5);
    end;
    frmVerifyPassword.Free;
end;

procedure TfrmVerifyPassword.btnCancelClick(Sender: TObject);
begin
  Close();
end;

procedure TfrmVerifyPassword.btnOKClick(Sender: TObject);
begin
  FIsOK := True;
  Close();
end;

end.
