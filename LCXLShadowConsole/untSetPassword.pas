unit untSetPassword;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, RzButton, RzLabel, Mask, RzEdit,

  untResString, Hash;

type
  TfrmSetPassword = class(TForm)
    edtNewPassword: TRzEdit;
    edtPasswordAgain: TRzEdit;
    lblPasswordAgain: TRzLabel;
    lblNewPassword: TRzLabel;
    btnOK: TRzButton;
    btnCancel: TRzButton;
    procedure btnOKClick(Sender: TObject);
    procedure btnCancelClick(Sender: TObject);
  private
    { Private declarations }
    FIsOK: Boolean;
    PassMD5: string;
  public
    { Public declarations }
    property IsOK: Boolean read FIsOK;
  end;

var
  frmSetPassword: TfrmSetPassword;

function SetMgrPasswordBox(AOwner: TComponent): Boolean;

implementation
uses
  untMain;
{$R *.dfm}


function SetMgrPasswordBox(AOwner: TComponent): Boolean;
begin
  frmSetPassword := TfrmSetPassword.Create(AOwner);
  frmSetPassword.ShowModal;
  Result := frmSetPassword.IsOK;
  if Result then
  begin
    Result := frmMain.SetPassword(frmSetPassword.PassMD5);
  end;
  frmSetPassword.Free;
end;

procedure TfrmSetPassword.btnCancelClick(Sender: TObject);
begin
  Close;
end;

procedure TfrmSetPassword.btnOKClick(Sender: TObject);
begin
  if edtNewPassword.Text <> edtPasswordAgain.Text then
  begin
    MessageBox(Handle, PChar(RS_PASSWORD_NOT_MATCH), PChar(RS_MSG_STOP_TITLE), MB_ICONSTOP);
    Exit;
  end;
  PassMD5 := edtNewPassword.Text;
  if PassMD5 = '' then
  begin
    if MessageBox(Handle, PChar(RS_NO_PASSWORD_WARNING),
      PChar(RS_MSG_WARNING_TITLE), MB_ICONWARNING or MB_OKCANCEL) <> MB_OK then
    begin
      Exit;
    end;
  end
  else
  begin
    PassMD5 := MD5Print(MD5string(PassMD5));
  end;
  FIsOK := True;
  Close;
end;

end.
