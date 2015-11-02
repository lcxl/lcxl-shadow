unit untShadowing;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, Vcl.Graphics, Vcl.Controls, Vcl.Forms, Vcl.Dialogs,
  Vcl.StdCtrls, RzLabel, Vcl.Imaging.pngimage, Vcl.ExtCtrls;

type
  TfrmShadowing = class(TForm)
    imgLogo: TImage;
    lblTip: TRzLabel;
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  frmShadowing: TfrmShadowing;

implementation

{$R *.dfm}

end.
