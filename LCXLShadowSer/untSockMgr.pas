unit untSockMgr;

interface
uses
  Windows, LCXLIOCPBase, LCXLIOCPCmd;

type
  TSerSockObj = class(TCmdSockObj)
  private
    FIsAuthed: Boolean;
    FPassMD5: string;
  public

    function SendSucc(LL_SUCC: Word; LL_COM: Word): Boolean;overload;
    function SendSucc(LL_SUCC: Word; LL_COM: Word; Data: Pointer; DataLen: Longword): Boolean;overload;
    ///	<param name="LL_Fail">
    ///	  ∑¢ÀÕ¥ÌŒÛ ±µƒ√¸¡Ó¬Î
    ///	</param>
    ///	<param name="LL_COM">
    ///	  ≥ˆœ÷¥ÌŒÛµƒ√¸¡Ó¬Î
    ///	</param>
    ///	<param name="ErrorCode">
    ///	  ¥ÌŒÛ¬Î
    ///	</param>
    function SendFail(LL_Fail: Word; LL_COM: Word; ErrorCode: DWORD): Boolean;

    property IsAuthed: Boolean read FIsAuthed write FIsAuthed;
    property PassMD5: string read FPassMD5 write FPassMD5;
  end;

  TSerSockLst = class(TCmdSockLst)
  protected
    procedure CreateSockObj(var SockObj: TSocketObj); override; // ∏≤∏«
  end;

  TIOCPSerList = class(TIOCPCMDList)

  end;
implementation

{ TSerSockLst }

procedure TSerSockLst.CreateSockObj(var SockObj: TSocketObj);
begin
  SockObj := TSerSockObj.Create;
end;

{ TSerSockObj }

function TSerSockObj.SendFail(LL_Fail, LL_COM: Word; ErrorCode: DWORD): Boolean;
begin
  Result := SendData(LL_Fail, [@LL_COM, @ErrorCode], [SizeOf(LL_COM), SizeOf(ErrorCode)]);
end;

function TSerSockObj.SendSucc(LL_SUCC, LL_COM: Word): Boolean;
begin
  Result := SendData(LL_SUCC, @LL_COM, SizeOf(LL_COM));
end;

function TSerSockObj.SendSucc(LL_SUCC, LL_COM: Word; Data: Pointer;
  DataLen: Longword): Boolean;
begin
  Result := SendData(LL_SUCC, [@LL_COM, Data], [SizeOf(LL_COM), DataLen]);
end;

end.
