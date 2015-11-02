unit untClientSockMgr;

interface
uses
  Windows, System.SysUtils, LCXLIOCPBase, LCXLIOCPCmd;
type
  TClientSockLst = class(TCmdSockLst)
  protected
    procedure CreateSockObj(var SockObj: TSocketObj); override; // 覆盖
  end;

  TClientSockObj = class(TCmdSockObj)
  private
    //是否经过验证
    FIsAuthed: Boolean;
    FPassIsEmpty: Boolean;
    FPassIsVerified: Boolean;
  public
    property IsAuthed: Boolean read FIsAuthed write FIsAuthed;
    property PassIsEmpty: Boolean read FPassIsEmpty write FPassIsEmpty;
    property PassIsVerified: Boolean read FPassIsVerified write FPassIsVerified;
  end;

  TIOCPClientList = class(TIOCPCMDList)

  end;

implementation

{ TClientSockLst }

procedure TClientSockLst.CreateSockObj(var SockObj: TSocketObj);
begin
  SockObj := TClientSockObj.Create;

end;

end.
